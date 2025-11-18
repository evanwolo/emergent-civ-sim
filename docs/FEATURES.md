# Feature Documentation

Comprehensive guide to all implemented systems in the Grand Strategy Simulation Engine.

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
9. [Configuration](#configuration)

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
- `primaryLang`: Primary language (0-3, 4 base languages)
- `fluency`: Language skill level (0.0-1.0)

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
};
```

### Initialization
- All agents start as working-age adults (15-60 years)
- 50/50 male/female sex distribution
- Random regional assignment
- Lineage tracking begins with parent_a/parent_b = -1

### Mortality System

**Age-Specific Death Rates (annual):**
| Age Range | Annual Mortality | Description |
|-----------|-----------------|-------------|
| 0-5       | 1%              | Child mortality |
| 5-15      | 0.1%            | Youth |
| 15-50     | 0.2%            | Prime adult |
| 50-70     | 1%              | Middle age |
| 70-85     | 5%              | Elderly |
| 85-90     | 15%             | Very old |
| 90+       | 100%            | Hard cap |

**Implementation:**
- Converted to per-tick probability: `1 - (1 - annual)^(1/ticksPerYear)`
- Stochastic death check each tick
- Dead agents marked `alive=false`
- Removed from regionIndex and neighbor lists every 100 ticks

### Fertility System

**Age-Specific Birth Rates (annual, females only):**
| Age Range | Annual Fertility | Description |
|-----------|-----------------|-------------|
| <15       | 0%              | Pre-reproductive |
| 15-20     | 5%              | Teen fertility |
| 20-30     | 12%             | Peak fertility |
| 30-35     | 10%             | High fertility |
| 35-40     | 5%              | Declining |
| 40-45     | 2%              | Late fertility |
| 45+       | 0%              | Post-reproductive |

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

**Hardship Effects:**
```cpp
if (agent_econ.hardship > 0.5) {
    B[0] -= 0.001 * hardship  // Authority → Liberty
    B[2] -= 0.001 * hardship  // Hierarchy → Equality
}
```

**Inequality Effects:**
```cpp
if (regional_inequality > 0.4) {
    B[2] -= 0.001 * inequality  // Push toward equality
}
```

**Wealth Effects:**
```cpp
if (agent_econ.wealth > 2.0) {
    B[0] += 0.001 * wealth_factor  // Support authority
    B[2] += 0.001 * wealth_factor  // Support hierarchy
}
```

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

### Subsistence Requirements (per capita)
- Food: 0.7
- Energy: 0.35
- Tools: 0.2
- Services: 0.15
- Luxury: 0.0 (optional)

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

**Trade Network:**
- Each region trades with 5-10 neighbors (wrapping circular topology)
- Distance-based transport costs: 2% per hop

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
Price range: 0.5x to 2.0x base price

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

**Economic Systems:**
- **Mixed:** Default (most regions)
- **Cooperative:** High equality beliefs + low inequality
- **Market:** High liberty + low hardship
- **Feudal:** High hierarchy + high inequality
- **Planned:** High authority + high hardship

System-specific modifiers affect inequality and efficiency.

### Metrics
- **Welfare:** Weighted consumption satisfaction (0.0-2.0+)
- **Inequality:** Gini coefficient (0.0-1.0)
  - Efficient O(N log N) calculation using sorted wealth
- **Hardship:** Proportion of population below subsistence (0.0-1.0)
- **Development:** Cumulative surplus/tech advancement (0.0-1.0+)
- **Trade Volume:** Total goods traded per tick

**Typical Values @ 1000 ticks:**
- Trade Volume: 23,000-25,000
- Welfare: 0.9-1.0
- Inequality: 0.25-0.35
- Hardship: 0.06-0.08
- Development: 0.1-0.2

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

### Influences
- Economic hardship → stress
- Belief conflict → stress
- Social isolation → stress
- Stress → susceptibility to radical beliefs

---

## Health Module

### State Variables (per agent)
- `physical_health`: Body condition (0.0-1.0)
- `disease_exposure`: Infection risk
- `immunity`: Resistance to disease

### Dynamics
- Health affects productivity
- Disease spreads through networks
- Regional health infrastructure modulates outcomes

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
- `regionCapacity`: Lower = more migration pressure (future)
- `ticksPerYear`: Lower = faster aging, higher time resolution

**Network:**
- `avgConnections`: Higher = faster belief spread, more cohesion
- `rewireProb`: Higher = more long-range connections, less local clustering

---

## Performance Characteristics

### Computational Complexity
- **Belief Update:** O(N×k) per tick (N agents, k neighbors)
- **Economy Update:** O(R×G×T) every 10 ticks (R regions, G goods, T trade partners)
- **Clustering:** O(N×K×I) on demand (K clusters, I iterations)
- **Demographics:** O(N) per tick with compaction every 100 ticks

### Memory Footprint
- Agent: ~200 bytes each
- 50k agents: ~10 MB
- Economy data: ~5 MB
- Networks: ~3 MB
- **Total:** ~20 MB active data

### Parallelization
- OpenMP for belief updates (thread-safe parallel loops)
- SIMD-friendly tanh approximation
- Minimal synchronization points

### Optimization Strategies
1. **Cached Norms:** Pre-compute B_norm_sq for similarity
2. **Sparse Networks:** Neighbor lists, not adjacency matrix
3. **Batch Processing:** Economy every 10 ticks, demographics every tick
4. **Incremental Cleanup:** Dead agents removed every 100 ticks
5. **Skip Dead:** O(1) alive check before processing

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

*Last Updated: Phase 2.3+ (November 2025)*
