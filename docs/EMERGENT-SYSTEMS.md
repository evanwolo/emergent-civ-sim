# Emergent Systems Documentation

## Overview

This document describes the refactoring from hardcoded deterministic behavior to truly emergent dynamics. The simulation now generates outcomes from agent interactions and conditions rather than predetermined labels.

---

## Tier 1: System-Breaking Fixes (Critical)

### 1. Inequality from Wealth Distribution (not system labels)

**Before (hardcoded):**
```cpp
// Inequality was overwritten based on economic system label
if (region.system_type == "feudal") region.inequality = 0.75;
else if (region.system_type == "market") region.inequality = 0.50;
```

**After (emergent):**
```cpp
// Inequality computed from actual agent wealth distribution using Gini coefficient
std::sort(region_wealth.begin(), region_wealth.end());
double gini = computeGiniFromSortedWealth(region_wealth);
region.inequality = gini;  // TRUE inequality from data
```

### 2. Efficiency from Conditions (not system type)

**Before:** Efficiency was a lookup table by economic system label.

**After:** Efficiency emerges from:
- Regional development level
- Inequality (optimal around moderate levels)
- Infrastructure proxy (development × 0.3)
- Coordination costs (population density)

### 3. Income Distribution (no feudal elite bonuses)

**Before:** Feudal systems gave 2.5× income multiplier to wealthy agents.

**After:** Matthew effect emerges naturally:
- Capital returns: `wealth_return = log(1 + wealth) × 0.01`
- Network effects: Above-average wealth → slight income boost
- Capital penalty: Below-average wealth → slight income penalty
- No system-specific hardcoded bonuses

### 4. Leadership from Emergent Traits (not pre-selection)

**Before:**
```cpp
if (agent_id % 100 == 0) agents_[agent_id].assertiveness = 0.9;  // Every 100th agent
```

**After:** No pre-selection. Leaders emerge from:
- Natural trait variation (Gaussian distribution)
- Social network position
- Belief centrality within clusters

### 5. Belief Evolution (wealth influence, not determinism)

**Before:** Wealthy agents automatically pushed toward pro-authority/hierarchy beliefs.

**After:** Personality-modulated influence:
```cpp
double wealth_influence = (1.0 - agent.openness) * 0.5;  // Closed-minded more affected
double belief_shift = wealth_influence * std::log1p(relative_wealth) * 0.001;
```

### 6. Economic System Transitions (probabilistic, not magic thresholds)

**Before:** Systems changed instantly when metrics crossed fixed boundaries.

**After:** Probabilistic transitions based on multiple factors:
- Population pressure on system change
- Institutional inertia (existing systems harder to change)
- Crisis acceleration (high hardship increases transition probability)
- Gradual adoption (0.2-5% probability per tick when conditions met)

---

## Tier 2: Significant Constraint Fixes

### 7. Price Bounds (0.01-100.0, not 0.1-10.0)

**Before:** Artificial price floors/ceilings prevented realistic market dynamics.

**After:** Only numerical stability limits:
- Near-free goods (< 0.01) trigger demand spike
- Hyperinflation (> 100.0) triggers barter/alternatives
- Full supply/demand price discovery within bounds

### 8. Trade Network Topology (geography-based)

**Before:** Fixed ±5 region IDs as trade partners.

**After:** Partners determined by:
- Geographic distance (Euclidean from x,y coordinates)
- Number of partners: 2-15 based on `floor(2 + development × 10 + rand × 3)`
- Distance-weighted selection

### 9. Migration Age Restriction (age-weighted mobility)

**Before:** Only agents aged 18-35 could migrate.

**After:** Age-weighted mobility curve:
```cpp
age_factor = 1.0 - (age - 25)² / 2500.0;  // Peak at 25
// Elderly can migrate (lower probability), children unlikely
```

### 10. Migration Attractiveness Threshold (personality-based)

**Before:** Fixed 0.3 minimum attractiveness difference required.

**After:** Threshold = `0.15 + (1.0 - openness) × 0.3`
- Open agents: 0.15-0.30 threshold (move easily)
- Closed agents: 0.30-0.45 threshold (more rooted)

### 11. Network Retention on Migration (personality-based)

**Before:** Fixed 30% of old connections retained.

**After:** Retention = `0.2 + sociality × 0.4`
- Low sociality: Keep 20-40% of old network
- High sociality: Keep 40-60% of old network

### 12. Language Zone Boundaries (fuzzy distance-based)

**Before:** Hard quadrant boundaries:
```cpp
if (x < 7 && y < 7) return WESTERN;
```

**After:** Distance-based probability with overlap zones:
```cpp
double dist_to_western = sqrt((x-3.5)² + (y-3.5)²);
// Soft boundaries with regional variation
// Dialects encode fine-grained geographic origin
```

---

## Tier 3: Variability-Reducing Constant Fixes

### 13. Regional Subsistence Needs (climate/development-based)

**Before:** Universal 0.7 food, 0.35 energy, etc. for all regions.

**After:** `RegionalNeeds` struct computed from:
- Climate proxy (latitude → food/energy needs)
- Development level (→ tools/services needs)
- Population density (→ services needs)

```cpp
struct RegionalNeeds {
    double food;     // 0.5-0.9 based on climate
    double energy;   // 0.25-0.45 based on climate
    double tools;    // 0.15-0.25 based on development
    double services; // 0.10-0.20 based on density
    double luxury;   // 0.0-0.15 based on development
};
```

### 14. Hardship Weights (development-scaled priorities)

**Before:** Fixed weights: food=3.0, energy=2.0, tools=1.5, services=1.0.

**After:** Development-scaled priorities:
```cpp
food_weight = 4.0 - development × 1.5;      // 2.5-4.0 (poor regions prioritize food)
energy_weight = 2.0 + development × 0.5;    // 2.0-2.5 (developed need more energy)
tools_weight = 1.0 + development × 1.0;     // 1.0-2.0 (developed need more tools)
services_weight = 0.5 + development × 1.5;  // 0.5-2.0 (developed need more services)
```

### 15. Stress Sensitivity (personality-based)

**Before:** Uniform stress coefficients for all agents.

**After:** `StressSensitivity` struct based on personality:
```cpp
struct StressSensitivity {
    double economic;      // (1.0 - openness) × (1.0 + conformity) × 0.5
    double media;         // conformity × (1.0 + sociality) × 0.4
    double institutional; // (1.0 - assertiveness) × (1.0 + conformity) × 0.4
    double disease;       // (1.0 - openness) × (1.0 - assertiveness) × 0.3
};
// Range: 0.0-0.5 per factor (more variation, some agents resilient)
```

### 16. Demographic Rates (development-modified)

**Before:** Fixed mortality/fertility rates regardless of region.

**After:**
- Infant mortality: Most affected by development (×0.3 to ×1.0 modifier)
- Child mortality: Significantly affected (×0.5 to ×1.0)
- Adult mortality: Modestly affected (×0.7 to ×1.0)
- Elderly mortality: Least affected (×0.9 to ×1.0)
- Fertility: Demographic transition effect (high development → lower fertility)

### 17. Infection Pressure Weights (adaptive by region type)

**Before:** Fixed weights for all health risk calculations.

**After:** Weights adapt to regional characteristics:
```cpp
// Urban areas (high density)
sanitation_weight = 0.3 + density × 0.1;
crowding_weight = 0.1 + density × 0.15;

// Rural areas (low density)
healthcare_weight = 0.25 + (1.0 - density) × 0.1;

// Poor regions
hardship_weight = 0.2 + hardship × 0.15;
```

---

## Emergent Outcomes Observed

### Economic Patterns

1. **Kuznets Curve:** Inequality rises then falls with development
   - Tick 0: 0.50 → Tick 250: 0.52 → Tick 500: 0.37

2. **Geographic Inequality:** Peripheral regions suffer, central hubs prosper
   - Luxury-producing regions can have hardship=1.0 (resource curse)

3. **Class Stratification:** Poorest concentrated in Luxury sector, middle class in Food

### Cultural Patterns

1. **Ethno-Regional Identity:** Only emerges in geographically isolated clusters
   - Example: Cluster with 60% Southern speakers in regions R68/R104

2. **Cosmopolitan Minorities:** Pro-Unity belief clusters emerge in trade hubs

3. **Language-Culture Decorrelation:** In connected regions, cultures transcend language

### Political Patterns

1. **Elite Minorities:** Small clusters with Anti-Equality beliefs (0.5% of population)

2. **Movement Formation:** Requires genuine hardship + coherent ideology concentration
   - High thresholds prevent trivial movement spawning

---

## Remaining Hardcoded Elements (Tier 4)

These remain as potential future improvements:

1. **Movement thresholds:** assertiveness > 0.7, coherence > 0.85
2. **Trade transport costs:** Fixed 2% per hop
3. **Agent consumption rate:** Fixed 80% of income

---

## Validation

Run simulation and observe:
```bash
echo "run 500 50`nstats`neconomy`ncluster kmeans 8`ncultures`nclasses" | docker run -i --rm civilizationsim:latest
```

Expected emergent patterns:
- Inequality varies by region (not uniform by system type)
- Prices span wide range (not clustered at 1.0)
- Leaders emerge naturally (not every 100th agent)
- Cultural clusters reflect geographic + network structure

---

*Last Updated: November 2025 (Emergent Systems Refactoring)*
