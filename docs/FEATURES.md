# Feature Documentation

Comprehensive guide to all implemented systems in the Emergent Civilization Simulation Engine.

---

## Table of Contents
1. [Agent System](#agent-system)
2. [Demographics](#demographics)
3. [Belief Dynamics](#belief-dynamics)
4. [Economic System](#economic-system)
5. [Culture & Clustering](#culture--clustering)
6. [Movements](#movements)
7. [Psychology Module](#psychology-module)
8. [Health Module](#health-module)
9. [Tuning Constants](#tuning-constants)
10. [Configuration](#configuration)

---

## Agent System

### Core Agent Structure
Each of the 50,000 agents has:

**Identity:**
- `id`: Unique identifier (0-49999)
- `region`: Geographic location (0-199)
- `alive`: Vital status flag
- `age`: Current age in years (0-90)
- `female`: Biological sex (50/50 distribution)

**Lineage:**
- `parent_a`, `parent_b`: Parent IDs (-1 if none)
- `lineage_id`: Family/clan identifier

**Language:**
- `primaryLang`: Language family (0-3: Western, Eastern, Northern, Southern)
- `dialect`: Regional dialect within family (0-255, encodes geographic origin)
- `fluency`: Language skill level (0.0-1.0)

**Language Assignment:**
- Distance-based probability to language zone centers
- Fuzzy boundaries allow overlap regions
- Dialects encode fine-grained regional identity

**Personality Traits (0.0-1.0):**
- `openness`: Receptiveness to new ideas
- `conformity`: Tendency to follow group norms
- `assertiveness`: Leadership/dominance
- `sociality`: Social connection strength

**Beliefs (4D space, -1.0 to 1.0):**
- `B[0]`: Authority ↔ Liberty
- `B[1]`: Tradition ↔ Progress
- `B[2]`: Hierarchy ↔ Equality  
- `B[3]`: Faith ↔ Reason

**Internal State:**
- `x[4]`: Unbounded internal belief state
- `B[4]`: Observable beliefs = tanh(x)
- `B_norm_sq`: Cached norm for similarity calculations

**Module Multipliers:**
- `m_comm`: Communication reach/speed
- `m_susceptibility`: Influence susceptibility
- `m_mobility`: Migration ease

**Network:**
- `neighbors`: Vector of connected agent IDs (small-world network)

**Module States:**
- `psych`: Psychological state (stress, satisfaction, etc.)
- `health`: Health metrics

---

## Demographics

### Overview
Fully integrated age-structured population with births, deaths, and multi-generational dynamics.

### Configuration
```cpp
struct KernelConfig {
    int ticksPerYear = 10;          // 1 year = 10 ticks
    int maxAgeYears = 90;           // Maximum lifespan
    double regionCapacity = 500.0;  // Target population per region
    bool demographyEnabled = true;  // Enable births/deaths
    uint32_t maxPopulation = 2000000; // Safety cap on total population
};
```

### Initialization
- All agents start as working-age adults (15-60 years)
- 50/50 male/female sex distribution
- Random regional assignment
- Lineage tracking begins with parent_a/parent_b = -1

### Mortality System

**Age-Specific Death Rates (annual, modified by development):**
| Age Range | Base Rate | Low Dev | High Dev | Description |
|-----------|-----------|---------|----------|-------------|
| 0-5       | 1%        | 1.0%    | 0.3%     | Infant mortality (most affected) |
| 5-15      | 0.1%      | 0.1%    | 0.05%    | Youth |
| 15-50     | 0.2%      | 0.2%    | 0.14%    | Prime adult |
| 50-70     | 1%        | 1.0%    | 0.8%     | Middle age |
| 70-85     | 5%        | 5.0%    | 4.5%     | Elderly (least affected) |
| 85-90     | 15%       | 15%     | 13.5%    | Very old |
| 90+       | 100%      | 100%    | 100%     | Hard cap |

**Development Effect:**
- Infant: `base_rate × (0.3 + 0.7 × (1-development))`
- Child: `base_rate × (0.5 + 0.5 × (1-development))`
- Adult: `base_rate × (0.7 + 0.3 × (1-development))`
- Elderly: `base_rate × (0.9 + 0.1 × (1-development))`

**Implementation:**
- Converted to per-tick probability: `1 - (1 - annual)^(1/ticksPerYear)`
- Stochastic death check each tick
- Dead agents marked `alive=false`
- Removed from regionIndex and neighbor lists every 100 ticks

### Fertility System

**Age-Specific Birth Rates (annual, females only):**
| Age Range | Base Rate | High Dev Rate | Description |
|-----------|-----------|---------------|-------------|
| <15       | 0%        | 0%            | Pre-reproductive |
| 15-20     | 5%        | 3%            | Teen fertility |
| 20-30     | 12%       | 8%            | Peak fertility |
| 30-35     | 10%       | 7%            | High fertility |
| 35-40     | 5%        | 4%            | Declining |
| 40-45     | 2%        | 1.5%          | Late fertility |
| 45+       | 0%        | 0%            | Post-reproductive |

**Demographic Transition Effect:**
```cpp
fertility *= (1.0 - development * 0.3);  // High development reduces fertility
```

**Modulation Factors:**
1. **Economic Hardship:**
   - `fertility *= (0.7 + 0.3 * (1 - hardship))`
   - High hardship reduces fertility 70-100%

2. **Carrying Capacity:**
   - If `regionPop > capacity`: `fertility /= (regionPop / capacity)`
   - Prevents runaway population growth

3. **Father Selection:**
   - Random choice from mother's neighbor network
   - Must be alive and male
   - If no valid father: mother's traits used (asexual reproduction)

### Inheritance

**Genetic Traits (from parents + 5% mutation):**
```cpp
child.trait = 0.5 * (mother.trait + father.trait) + N(0, 0.05)
// Applies to: openness, conformity, assertiveness, sociality
```

**Cultural Transmission (beliefs):**
```cpp
child.B[k] = 0.5 * (mother.B[k] + father.B[k]) + N(0, 0.2)
// 50% parent average + 20% cultural noise
```

**Network Inheritance:**
- Child automatically connected to mother
- Inherits 3 random neighbors from mother's network
- Builds family/kinship clusters

**Language:**
- Inherits mother's primary language
- Starts with 0.5 fluency (grows with age/exposure)

### Population Dynamics
- **Stable Equilibrium:** Fertility and mortality rates balanced for ~50k population
- **Regional Variation:** Harder regions have lower birth rates
- **Age Distribution:** Evolves from initial 15-60 to full 0-90 pyramid over time
- **Generational Drift:** Beliefs change across generations due to cultural noise

---

## Belief Dynamics

### 4D Belief Space
Agents navigate a 4-dimensional ideological space:

1. **Authority (-1) ↔ Liberty (+1)**
   - Centralized power vs individual freedom
   - Influences: Economy (hardship → liberty), movements

2. **Tradition (-1) ↔ Progress (+1)**
   - Conservative vs reformist
   - Influences: Age, development level

3. **Hierarchy (-1) ↔ Equality (+1)**
   - Social stratification vs egalitarianism
   - Influences: Wealth, inequality

4. **Faith (-1) ↔ Reason (+1)**
   - Religious vs secular worldview
   - Influences: Education, crises

### Update Mechanism

**Per Tick:**
```cpp
for each agent i:
    for each neighbor j:
        similarity = cosine_similarity(i.B, j.B)
        language_quality = language_overlap(i, j)
        weight = stepSize * similarity * language_quality * 
                 i.m_comm * j.m_comm * i.m_susceptibility
        
        dx[k] += weight * tanh(j.B[k] - i.B[k])
    
    i.x[k] += dx[k]
    i.B[k] = tanh(i.x[k])
```

**Key Features:**
- **Similarity Gate:** Agents only influenced by similar neighbors
- **Language Barrier:** Reduced influence across language groups
- **Susceptibility:** Varies with personality (openness) and hardship
- **Small-World Network:** Watts-Strogatz topology (k=8, p=0.05)

### Economic Feedback on Beliefs

**Personality-Modulated Wealth Influence:**
```cpp
// Wealth influence is modulated by openness (closed-minded more affected)
double wealth_influence = (1.0 - agent.openness) * 0.5;
double relative_wealth = agent.wealth / regional_avg_wealth;

if (relative_wealth > 1.5) {
    // Wealthy lean toward hierarchy/authority (personality-dependent)
    double shift = wealth_influence * log(1 + relative_wealth) * 0.001;
    B[0] += shift;  // Authority
    B[2] += shift;  // Hierarchy
}
```

**Hardship Effects (unchanged):**
```cpp
if (agent_econ.hardship > 0.5) {
    B[0] -= 0.001 * hardship  // Authority → Liberty
    B[2] -= 0.001 * hardship  // Hierarchy → Equality
}
```

**Key Change:** No deterministic wealth→belief forcing. Open-minded wealthy agents can maintain egalitarian beliefs.

---

## Economic System

### Overview
Five-good trade economy with dramatic regional specialization creating scarcity and interdependence.

### Goods
1. **Food** - Agriculture, sustenance
2. **Energy** - Fuel, power
3. **Tools** - Manufactured goods, technology
4. **Luxury** - Non-essential high-value goods
5. **Services** - Labor, expertise

### Regional Endowments

**Initialization Strategy:**
- **Primary Resource:** Each region abundant in 1 good (2.0-4.0 per capita)
- **Secondary Resource:** Adequate in 1 good (0.8-1.6 per capita)
- **Scarce Goods:** Desperately poor in 1-2 goods (0.05-0.15 per capita)
- **Other Goods:** Low baseline (0.2-0.4 per capita)

**Geographic Clustering:**
- 30% chance to inherit neighbor's abundance
- Simulates terrain features (fertile plains, mineral deposits, forests)
- Creates resource "zones" across regions

### Regional Subsistence Requirements

Subsistence needs now vary by region based on geography and development:

**Climate-Based Needs (per capita):**
| Need | Cold Climate | Temperate | Hot Climate |
|------|-------------|-----------|-------------|
| Food | 0.9 | 0.7 | 0.6 |
| Energy | 0.45 | 0.35 | 0.25 |

**Development-Based Needs:**
| Need | Low Development | High Development |
|------|-----------------|------------------|
| Tools | 0.15 | 0.25 |
| Services | 0.10 | 0.20 |
| Luxury | 0.0 | 0.15 |

### Production
```cpp
production[good] = endowment * population * specialization_bonus * 
                   tech_multiplier * efficiency * development_bonus * 
                   (1 - war_allocation)
```

**Factors:**
- `specialization_bonus`: 1.0 + specialization (-0.5 to +0.3)
- `tech_multiplier`: Good-specific technology level (0.8-1.2)
- `efficiency`: Institutional efficiency (0.8-1.0)
- `development_bonus`: 1.0 + development * 0.2
- `war_allocation`: Resources diverted to conflict (0.0-1.0)

### Trade System

**Trade Network (Geography-Based):**
- Partners determined by Euclidean distance from region coordinates (x, y)
- Number of partners: `2 + development × 10 + random(0-3)` (range: 2-15)
- Nearby regions prioritized over distant ones
- Transport costs: 2% per hop

**Trade Logic (per good, per tick):**
1. Calculate surplus: `production - (population * subsistence)`
2. If surplus > 0: Export to deficit neighbors
3. Export volume: min(available surplus, partner deficit * 0.5)
4. Transport loss: `volume * (1 - transport_cost)`

**Price Dynamics:**
```cpp
if (demand > supply):
    price *= (1 + PRICE_ADJUSTMENT_RATE)
else:
    price *= (1 - PRICE_ADJUSTMENT_RATE * 0.5)
```
**Emergent Price Range:** 0.01-100.0 (only numerical stability limits)
- Near-free goods attract demand surges
- Hyperinflation triggers barter alternatives

### Specialization Evolution
```cpp
if (surplus > 0):
    specialization[good] += SPECIALIZATION_RATE  // +0.001/tick
else if (deficit > 0):
    specialization[good] -= SPECIALIZATION_RATE * 0.5
```
Specialization range: -0.5 to +0.3

### Agent Economy

**Per Agent:**
- `wealth`: Accumulated savings
- `income`: Per-tick earnings
- `productivity`: Labor output (0.5-1.5, varies by traits)
- `hardship`: Unmet needs (0.0-1.0)
- `sector`: Employment sector (0-4 maps to goods)

**Income Distribution:**
```cpp
agent.income = (agent.productivity / region_total_productivity) * 
               regional_production[agent.sector]
```

**Economic Systems (Emergent Transitions):**
- **Mixed:** Default starting state
- **Cooperative:** Emerges from egalitarian beliefs + low actual inequality
- **Market:** Emerges from liberty beliefs + low hardship conditions
- **Feudal:** Emerges from hierarchy beliefs + high actual inequality
- **Planned:** Emerges from authority beliefs + crisis conditions

**Transition Dynamics:**
- Probabilistic transitions (0.2-5% per tick when conditions met)
- Institutional inertia slows change
- Crisis accelerates transitions
- No instant system flips

**Emergent Properties:**
- Inequality computed from actual Gini coefficient (not system label)
- Efficiency emerges from development + moderate inequality + coordination

### Metrics
- **Welfare:** Weighted consumption satisfaction (0.0-2.0+)
- **Inequality:** Gini coefficient (0.0-1.0)
  - Efficient O(N log N) calculation using sorted wealth
- **Hardship:** Proportion of population below subsistence (0.0-1.0)
- **Development:** Cumulative surplus/tech advancement (0.0-1.0+)
- **Trade Volume:** Total goods traded per tick

**Typical Values @ 500 ticks (Emergent Dynamics):**
- Trade Volume: 250,000-300,000
- Welfare: 1.3-1.7 (varies by region: 0.2-2.5)
- Inequality: 0.30-0.45 (Kuznets curve: rises then falls)
- Hardship: 0.4-0.7 (geographic variation: 0.0-1.0)
- Development: 0.2-0.4

**Regional Variation:**
- Periphery regions: Higher hardship, lower welfare
- Trade hubs: Lower hardship, higher welfare
- Resource curse: Luxury regions can have high hardship

---

## Culture & Clustering

### Overview
Cultural groups emerge through clustering of similar agents in belief space.

### Clustering Algorithms

**1. K-Means**
```
cluster kmeans K
```
- Partitions agents into K distinct cultures
- Iterative centroid refinement
- Fast, deterministic

**2. DBSCAN**
```
cluster dbscan epsilon minPts
```
- Density-based clustering
- Finds arbitrary-shaped clusters
- Handles noise (unclustered agents)
- Parameters:
  - `epsilon`: Maximum distance for neighborhood
  - `minPts`: Minimum agents for core point

### Cluster Characteristics
Each detected cluster has:
- **Centroid:** Average belief position in 4D space
- **Size:** Number of member agents
- **Coherence:** Average pairwise similarity (0.0-1.0)
- **Charisma Density:** Proportion of high-assertiveness members
- **Regional Distribution:** Geographic spread

### Charismatic Hubs
Special agents with:
- High assertiveness (>0.7)
- Many connections in cluster
- Central belief positions
- Potential movement leaders

---

## Movements

### Overview
Political/social movements form when coherent cultural clusters face pressure.

### Formation Triggers

**Required Conditions:**
1. **Size:** Cluster > 100 agents
2. **Coherence:** Average similarity > 0.7
3. **Charisma Density:** >60% assertive agents
4. **Economic Pressure:** Regional hardship > 0.5 OR inequality > 0.6

**Formation Check:**
```
cluster kmeans 5
detect_movements
```

### Movement Structure

**Core Properties:**
- `id`: Unique identifier
- `platform`: 4D belief vector (movement goals)
- `members`: List of agent IDs
- `leaders`: High-charisma core members
- `regions`: Geographic strongholds
- `stage`: Lifecycle phase

**Power Metrics:**
- `street_capacity`: Raw member count (0.0-1.0 normalized)
- `charisma`: Leadership quality (0.0-1.0)
- `coherence`: Internal unity (0.0-1.0)
- `power`: Combined metric = 0.5×street + 0.3×coherence + 0.2×charisma

**Regional Strength:**
- Per-region member density
- Influences local policy/conflict

### Lifecycle Stages
1. **Birth:** Initial formation from cluster
2. **Growth:** Gaining members/power
3. **Plateau:** Stable membership
4. **Schism:** Internal fracturing
5. **Decline:** Losing support
6. **Dead:** Disbanded

### Commands
```bash
detect_movements        # Form movements from last clustering
movements              # List active movements with stats
movement <ID>          # Detailed info for specific movement
```

---

## Psychology Module

### State Variables (per agent)
- `stress`: Accumulated pressure (0.0-1.0)
- `satisfaction`: Life contentment (0.0-1.0)
- `trauma`: Long-term psychological impact

### Stress Sensitivity (Personality-Based)
Each agent has unique stress coefficients based on personality:

```cpp
struct StressSensitivity {
    double economic;      // (1-openness) × (1+conformity) × 0.5
    double media;         // conformity × (1+sociality) × 0.4
    double institutional; // (1-assertiveness) × (1+conformity) × 0.4
    double disease;       // (1-openness) × (1-assertiveness) × 0.3
};
```

**Personality Effects:**
- Open agents: More resilient to economic/disease stress
- Conformist agents: More sensitive to media/institutional pressure
- Assertive agents: More resilient to institutional stress
- Range: 0.0-0.5 per factor (allows for resilient individuals)

### Influences
- Economic hardship → stress (weighted by personality)
- Belief conflict → stress
- Social isolation → stress (sociality-dependent)
- Stress → susceptibility to radical beliefs

---

## Health Module

### State Variables (per agent)
- `physical_health`: Body condition (0.0-1.0)
- `disease_exposure`: Infection risk
- `immunity`: Resistance to disease

### Adaptive Infection Pressure
Infection weights vary by regional characteristics:

**Urban Areas (high density):**
- Sanitation weight: 0.3 + density × 0.1
- Crowding weight: 0.1 + density × 0.15

**Rural Areas (low density):**
- Healthcare access weight: 0.25 + (1-density) × 0.1

**Poor Regions:**
- Hardship weight: 0.2 + hardship × 0.15

### Dynamics
- Health affects productivity
- Disease spreads through networks
- Regional health infrastructure modulates outcomes
- Development reduces baseline mortality rates

---

## Configuration

### KernelConfig Structure
```cpp
struct KernelConfig {
    // Population
    uint32_t population = 50000;
    uint32_t regions = 200;
    
    // Network
    uint32_t avgConnections = 8;      // k for Watts-Strogatz
    double rewireProb = 0.05;          // p for Watts-Strogatz
    
    // Belief Dynamics
    double stepSize = 0.15;            // eta (influence rate)
    double simFloor = 0.05;            // minimum similarity threshold
    
    // Demographics
    int ticksPerYear = 10;             // time granularity
    int maxAgeYears = 90;              // lifespan cap
    double regionCapacity = 500.0;     // population target
    bool demographyEnabled = true;     // births/deaths toggle
    
    // System
    uint64_t seed = 42;                // RNG seed for determinism
};
```

### Tuning Guidelines

**Population Scale:**
- 50k agents: Standard, well-tested
- 100k+: Requires more RAM, slower but more granular
- 10k: Faster testing, less emergent complexity

**Belief Dynamics:**
- `stepSize`: Higher = faster convergence, less stability
- `simFloor`: Higher = stronger echo chambers

**Demographics:**
- `regionCapacity`: Lower = more migration pressure
- `maxPopulation`: Hard cap to prevent unbounded growth
- `ticksPerYear`: Lower = faster aging, higher time resolution

**Network:**
- `avgConnections`: Higher = faster belief spread, more cohesion
- `rewireProb`: Higher = more long-range connections, less local clustering

---

## Tuning Constants

The `TuningConstants` namespace in `Kernel.h` provides centralized control over emergent behavior dynamics:

### Belief Dynamics
| Constant | Default | Description |
|----------|---------|-------------|
| `kHomophilyExponent` | 2.5 | Exponential homophily strength |
| `kHomophilyMinWeight` | 0.1 | Minimum neighbor influence weight |
| `kHomophilyMaxWeight` | 10.0 | Maximum neighbor influence weight |
| `kLanguageBonusMultiplier` | 1.5 | Shared language influence bonus |
| `kInnovationNoise` | 0.03 | Belief innovation std dev |

### Belief Anchoring
| Constant | Default | Description |
|----------|---------|-------------|
| `kAnchoringMaxAge` | 50.0 | Age at which anchoring maxes out |
| `kAnchoringBase` | 0.3 | Minimum anchoring (young agents) |
| `kAnchoringAgeWeight` | 0.4 | Age contribution to anchoring |
| `kAnchoringAssertWeight` | 0.2 | Assertiveness contribution |

### Network Dynamics
| Constant | Default | Description |
|----------|---------|-------------|
| `kReconnectInterval` | 5 | Ticks between reconnection passes |
| `kReconnectCapFraction` | 0.02 | Max fraction reconnected per tick |
| `kNeighborWeightMin` | 0.5 | Min neighbor vs regional influence |
| `kNeighborWeightMax` | 0.85 | Max neighbor vs regional influence |

### Migration
| Constant | Default | Description |
|----------|---------|-------------|
| `kHardshipPushWeight` | 2.0 | Hardship contribution to push |
| `kCrowdingPenaltyWeight` | 0.5 | Over-capacity crowding penalty |

### Economic Pressure
| Constant | Default | Description |
|----------|---------|-------------|
| `kBasePressureMultiplier` | 0.05 | Base pressure from economy |
| `kHardshipThreshold` | 0.3 | Hardship triggering belief response |
| `kWelfareThreshold` | 0.5 | Welfare triggering openness response |

---

## Performance Characteristics

### Computational Complexity
- **Belief Update:** O(N) with mean field (O(N×k) without)
- **Economy Update:** O(R²) matrix trade every 10 ticks
- **Clustering:** O(1) per agent with online K-means
- **Demographics:** O(C) cohort-based (C << N)

### Memory Footprint
- Agent: ~200 bytes each
- 50k agents: ~10 MB
- Economy data: ~5 MB
- Networks: ~3 MB
- Trade Laplacian: ~320 KB (200 regions)
- Cohorts: ~4 KB
- **Total:** ~20 MB active data

### Parallelization
- OpenMP for belief updates (thread-safe two-phase pattern)
- SIMD-friendly tanh approximation
- Minimal synchronization points

### Optimization Strategies
1. **Mean Field:** O(N) instead of O(N×k) for belief updates
2. **Cohort Demographics:** O(C) instead of O(N) for births/deaths
3. **Matrix Trade:** Laplacian diffusion replaces pairwise loops
4. **Online Clustering:** Incremental updates, no batch spikes
5. **Cached Norms:** Pre-compute B_norm_sq for similarity
6. **Sparse Networks:** Neighbor lists, not adjacency matrix
7. **Incremental Cleanup:** Dead agents compacted every 5 ticks

---

## Data Persistence

### Snapshot Format (JSON)
```json
{
  "generation": 1000,
  "agents": [
    {
      "id": 0,
      "region": 42,
      "alive": true,
      "age": 35,
      "female": false,
      "B": [0.2, -0.3, 0.1, -0.5],
      "openness": 0.65,
      "neighbors": [1, 5, 23, ...],
      ...
    }
  ]
}
```

### Metrics CSV
- Per-tick or per-interval metrics
- Columns: generation, population, welfare, inequality, hardship, trade_volume, etc.
- Location: `data/metrics.csv`

### Visualization
- External tools can parse JSON snapshots
- Network graphs, belief space plots, age pyramids
- Economic flow diagrams

---

## Future Extensions

### Planned Features
1. **Geography:** Terrain types, resource locations, coastlines
2. **Migration:** Population movement between regions
3. **Conflict:** War, violence, conquest
4. **Institutions:** Government types, laws, bureaucracy
5. **Technology:** Research trees, diffusion, productivity impacts
6. **Diplomacy:** Alliances, treaties, trade agreements
7. **Religion:** Organized faiths, syncretism, schisms
8. **Epidemics:** Disease outbreaks, public health responses

### Extensibility Points
- Module interface for new systems
- Event system for exogenous shocks
- Policy hooks for player/AI decisions
- Scenario loading for historical starts

---

*Last Updated: Phase 2.5 (November 2025)*
