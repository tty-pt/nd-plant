# IDEA.md — Plant Genome Data Model (v1.0) & Chrono-Deterministic Developmental Botany with Ephemeral Delta Decay

## 1. Core Architectural Principle

**A species definition is not an individual genome, and neither is the resulting plant.**
**The genome describes the mathematics of becoming a plant. The phenotype describes the plant that has become.**

We strictly distinguish four layers across two module boundaries:

| Structure / Layer | Owner | Meaning |
|---|---|---|
| `PlantSpeciesDef` | `external/nd-plant` | Species-level developmental defaults, algorithm selection (`PlantDevelopmentProgram`), population trait priors (`PlantTraitPrior`), and trait correlation matrices (`PlantCorrelationGroup`). |
| `PlantGenome` | `external/nd-plant` | Heritable developmental traits of one specific individual (`PlantGenomeTraits`: per-class `PlantAxisGenome`, leaf/branch `PlantPhyllotaxisGenome`, `PlantLeafGenome`, `PlantRootGenome`, `PlantAllometryGenome`, `PlantPhenologyGenome`, `PlantPlasticityGenome`, plus floral/fruit traits). Contains zero vertices, polygons, textures, or mesh data. |
| `PlantPhenotype` | `external/nd-plant` | The actual developed plant as an index-addressed Multiscale Tree Graph (MTG): typed dynamic/arena arrays of `PlantAxis`, `PlantGrowthUnit` (phytomers, root segments, reproductive units), `PlantBud` (dormant, active, developed, dead), and `PlantOrgan` (leaves, flowers, fruits, seeds, thorns). |
| `plant_geometry_build` | `mods/nd/ux/nd_seven_mesh3d.h` | Pure 3D/2D consumer of `PlantInstance` / `PlantPhenotype`. Never defines botanical structure; translates the developed MTG into renderable vertex/index buffers at the requested `detail_level`. |

```text
                 PlantSpeciesDef
                       |
              +--------+--------+
              |                 |
       Genetic priors     Development rules (PlantDevelopmentProgram)
       + Correlations     (Axis, Bud, Branching, Phyllotaxis, Tropism,
       (Cholesky L*L^T)    Da Vinci Allometry, Secondary Growth, Senescence)
              |                 |
              +--------+--------+
                       |
                       v
                  PlantGenome
                       |
              +--------+--------+
              |                 |
           Mutation          Inheritance (plant_genome_cross)
              |                 |
              +--------+--------+
                       |
                       v
                 PlantInstance
                       |
              +--------+--------+-----------------------+
              |                 |                       |
   PlantEnvironmentSampler   Growth history     Ephemeral Deltas
   (3D meristem position)    & Topo-Hash PRNG   (PlantDeltaRecord)
              |                 |                       |
              +--------+--------+-----------------------+
                       |
                       v
                GrowthSimulator
       (plant_simulate_step / plant_simulate_at_time)
                       |
                       v
                 PlantPhenotype (MTG)
         (PlantAxis, PlantGrowthUnit, PlantBud, PlantOrgan)
                       |
                       v
                GeometryGenerator (plant_geometry_build)
                       |
                       v
                 Renderable 3D / 2D
```

This architecture satisfies the four complementary elements of modern functional–structural plant modelling (FSPM) — **function, geometry, topology, and environment** — while simultaneously meeting NeverDark's real-time game world requirements:
1. **Continuous Growth Over Time**: Plants germinate, flush new phytomers, activate axillary buds, thicken via Da Vinci secondary growth, flower, fruit, senesce, and turn over across the world clock.
2. **Strict $\Delta t$-Invariant Chrono-Determinism**: Evaluating a plant at $T_{\text{now}} = t_0 + \Delta t$ produces the exact same `PlantPhenotype` graph whether simulated in a single analytical jump or stepped incrementally day-by-day.
3. **Zero-Memory Unvisited World & Ephemeral Self-Healing (Delta Decay)**: Unmodified wild areas consume **zero bytes** of RAM or persistent storage. Player perturbations (pruning branches, harvesting fruit, stripping leaves, felling stems) are stored as compact timestamped delta records (`PlantDeltaRecord`) that progressively regrow over $[t_{\text{event}}, t_{\text{event}} + t_{\text{heal}}]$ and are garbage-collected back to zero bytes once $\eta \ge 1.0$.

---

## 2. The C Data Model (`include/nd/plant_model.h`)

### 2.1 Fundamental Types & Intra-Individual Distributions

```c
#define PLANT_SCHEMA_VERSION         1u
#define PLANT_SPECIES_SCHEMA_VERSION 1u
#define PLANT_GENOME_SCHEMA_VERSION  1u
#define PLANT_GRAPH_SCHEMA_VERSION   1u

#define PLANT_NONE       UINT32_MAX
#define PLANT_INVALID_ID PLANT_NONE

typedef uint32_t PlantSpeciesId;
typedef uint32_t PlantAxisId;
typedef uint32_t PlantUnitId;
typedef uint32_t PlantBudId;
typedef uint32_t PlantOrganId;

typedef struct {
    float x, y, z;
} PlantVec3;

typedef struct {
    float mean;
    float sigma;
    float min;
    float max;
} PlantDistribution;
```

**Two distinct levels of variation (never confused)**:
- **Variation within an individual (`PlantDistribution`)**: A single plant does not produce identical leaves or internodes. `PlantDistribution` (sampled via truncated normal draws addressed by the organ's or phytomer's topological coordinate hash) governs intra-individual variation across organs of one plant.
- **Variation between individuals (`PlantTraitPrior` & `PlantCorrelationGroup`)**: Genetically distinct individuals of the same species differ in their mean traits, sampled via Cholesky-correlated normal draws from the species' `PlantTraitPrior`s.

### 2.2 Botanical Classification Vocabulary

```c
typedef enum {
    PLANT_TREE = 0,
    PLANT_SHRUB,
    PLANT_SUBSHRUB,
    PLANT_HERB,
    PLANT_GRASS,
    PLANT_FERN,
    PLANT_VINE,
    PLANT_SUCCULENT
} PlantLifeForm;

typedef enum {
    PLANT_MONOPODIAL = 0,
    PLANT_SYMPODIAL,
    PLANT_DICHOTOMOUS,
    PLANT_ROSETTE,
    PLANT_OTHER_GROWTH
} PlantGrowthArchitecture;

typedef enum {
    PLANT_ORTHOTROPIC = 0,
    PLANT_PLAGIOTROPIC,
    PLANT_PROSTRATE,
    PLANT_CLIMBING,
    PLANT_VARIABLE_ORIENTATION
} PlantAxisOrientation;

typedef enum {
    PLANT_AXIS_PRIMARY = 0,
    PLANT_AXIS_LATERAL,
    PLANT_AXIS_SHORT_SHOOT,
    PLANT_AXIS_REPRODUCTIVE,
    PLANT_AXIS_TAPROOT,
    PLANT_AXIS_LATERAL_ROOT,
    PLANT_AXIS_CLASS_COUNT
} PlantAxisClass;

typedef enum {
    PLANT_PHYLLOTAXIS_ALTERNATE = 0,
    PLANT_PHYLLOTAXIS_OPPOSITE,
    PLANT_PHYLLOTAXIS_DECUSSATE,
    PLANT_PHYLLOTAXIS_WHORLED,
    PLANT_PHYLLOTAXIS_SPIRAL,
    PLANT_PHYLLOTAXIS_ROSETTE
} PlantPhyllotaxisType;
```

A plant can exhibit different architectural behaviours across its axis classes (`axes[PLANT_AXIS_CLASS_COUNT]`): for example, an orthotropic monopodial primary trunk (`PLANT_AXIS_PRIMARY`), plagiotropic lateral boughs (`PLANT_AXIS_LATERAL`), foliar short shoots (`PLANT_AXIS_SHORT_SHOOT`), determinate inflorescences (`PLANT_AXIS_REPRODUCTIVE`), a positively gravitropic taproot (`PLANT_AXIS_TAPROOT`), and hydrotropic lateral roots (`PLANT_AXIS_LATERAL_ROOT`).

### 2.3 Per-Class Axis Genome (`PlantAxisGenome`)

```c
typedef struct {
    PlantAxisOrientation orientation;
    uint8_t              determinate;
    uint8_t              max_branch_order;
    float                extension_rate_m_per_day;
    float                maximum_length_m;
    PlantDistribution    internode_length_m;
    PlantDistribution    branch_angle_rad;
    float                lateral_bud_probability;
    float                apical_dominance;
    float                branch_length_ratio;
    float                growth_rate_variation;
} PlantAxisGenome;
```

Rather than prescribing a fixed branch count, the developmental simulator determines how many internodes are produced from `extension_rate_m_per_day`, `maximum_length_m`, and `internode_length_m`, when axillary buds break dormancy via `apical_dominance` and `lateral_bud_probability`, and how child axes scale via `branch_length_ratio`.

### 2.4 Explicit Phyllotaxis (`PlantPhyllotaxisGenome`)

```c
typedef struct {
    PlantPhyllotaxisType arrangement;
    uint8_t              organs_per_node;
    float                divergence_angle_rad;
    float                whorl_rotation_rad;
    float                angular_variation_rad;
} PlantPhyllotaxisGenome;
```

The genome stores separate `leaf_phyllotaxis` and `branch_phyllotaxis` descriptors so foliar organ placement and lateral shoot arrangement can each follow their true species geometry (golden angle $\approx 2.39996\text{ rad}$, distichous $\pi\text{ rad}$, opposite/decussate $\pi/2\text{ rad}$ phase rotation, or conifer whorls).

### 2.5 Leaf Morphology (`PlantLeafGenome`)

```c
typedef enum {
    PLANT_LEAF_SIMPLE = 0,
    PLANT_LEAF_COMPOUND_PINNATE,
    PLANT_LEAF_COMPOUND_BIPINNATE,
    PLANT_LEAF_COMPOUND_PALMATE,
    PLANT_LEAF_SCALE,
    PLANT_LEAF_NEEDLE
} PlantLeafType;

typedef enum {
    PLANT_LEAF_SHAPE_ELLIPTIC = 0,
    PLANT_LEAF_SHAPE_OVATE,
    PLANT_LEAF_SHAPE_OBOVATE,
    PLANT_LEAF_SHAPE_LANCEOLATE,
    PLANT_LEAF_SHAPE_LINEAR,
    PLANT_LEAF_SHAPE_CORDATE,
    PLANT_LEAF_SHAPE_ROUNDED,
    PLANT_LEAF_SHAPE_OTHER
} PlantLeafShape;

typedef struct {
    PlantLeafType     type;
    PlantLeafShape    shape;
    PlantDistribution blade_length_m;
    PlantDistribution width_ratio;
    PlantDistribution petiole_length_m;
    float             base_angle_ratio;
    float             apex_ratio;
    float             margin_amplitude;
    float             curvature;
    float             thickness_m;
    uint8_t           venation_type;
    uint8_t           venation_order;
    uint8_t           leaflet_count;
} PlantLeafGenome;
```

Encoding `width_ratio` separately from `blade_length_m` allows individual leaves to vary in size while preserving a recognisable species silhouette.

### 2.6 Root System (`PlantRootGenome`)

Roots share the exact same `PlantAxis` (`PLANT_AXIS_TAPROOT`, `PLANT_AXIS_LATERAL_ROOT`) and `PlantGrowthUnit` (`PLANT_UNIT_ROOT_SEGMENT`) graph representation as shoots:

```c
typedef enum {
    PLANT_ROOT_TAPROOT = 0,
    PLANT_ROOT_FIBROUS,
    PLANT_ROOT_ADVENTITIOUS,
    PLANT_ROOT_TUBEROUS
} PlantRootArchitecture;

typedef struct {
    PlantRootArchitecture architecture;
    float                 maximum_depth_m;
    float                 maximum_spread_m;
    float                 branching_probability;
    float                 branching_angle_rad;
    float                 diameter_taper_exponent;
    float                 gravitropism;
    float                 hydrotropism;
} PlantRootGenome;
```

### 2.7 Da Vinci Allometry with Soft Constraints (`PlantAllometryGenome`)

```c
typedef struct {
    float branch_radius_exponent;    /* Da Vinci alpha (e.g. 1.8 .. 2.5) */
    float branch_constraint_weight;  /* [0..1] blend between free growth and strict pipe conservation */
    float stem_taper_exponent;
    float branch_length_exponent;
    float branch_length_coefficient;
    float crown_width_height_ratio;
    float crown_asymmetry;
} PlantAllometryGenome;
```

At every branching junction, parent and child radii follow the configurable Da Vinci relationship:
$$r_p^\alpha \approx \sum_{i=1}^{n} r_i^\alpha$$
And when distributing a parent's conductive capacity across child branches of relative vigour $w_i$:
$$r_i^{\text{ideal}} = r_p \left( \frac{w_i}{\sum_j w_j} \right)^{1/\alpha}$$
Using `branch_constraint_weight` $\lambda \in [0, 1]$, actual radii blend between unconstrained local growth $r_i^{\text{free}}$ and strict allometric equilibrium:
$$r_i = (1 - \lambda)\, r_i^{\text{free}} + \lambda\, r_i^{\text{ideal}}$$
allowing damaged, pruned, shaded, or asymmetric trees to deviate naturally from rigid geometric ideals.

### 2.8 Physiological Phenology (`PlantPhenologyGenome`)

```c
typedef enum {
    PLANT_EVERGREEN = 0,
    PLANT_SEMIEVERGREEN,
    PLANT_DECIDUOUS
} PlantLeafPersistence;

typedef struct {
    PlantLeafPersistence persistence;
    float                base_temperature_C;
    float                budburst_degree_days;
    float                flowering_degree_days;
    float                senescence_degree_days;
    float                dormancy_chilling_hours;
    float                leaf_fall_daylength_hours;
    float                minimum_reproductive_age_days;
} PlantPhenologyGenome;
```

Phenology is driven by physiological triggers (`accumulated_degree_days` above `base_temperature_C`, chilling requirement, photoperiod, and `minimum_reproductive_age_days`) rather than hardcoded calendar dates.

### 2.9 Environmental Plasticity (`PlantPlasticityGenome`)

```c
typedef struct {
    float phototropism;
    float gravitropism;
    float hydrotropism;
    float shade_avoidance;
    float drought_sensitivity;
    float temperature_sensitivity;
    float wind_flexibility;
} PlantPlasticityGenome;
```

### 2.10 The Complete Individual Genome (`PlantGenome`)

```c
typedef struct {
    PlantLifeForm           life_form;
    PlantGrowthArchitecture architecture;
    PlantAxisGenome         axes[PLANT_AXIS_CLASS_COUNT];
    PlantPhyllotaxisGenome  leaf_phyllotaxis;
    PlantPhyllotaxisGenome  branch_phyllotaxis;
    PlantLeafGenome         leaves;
    PlantRootGenome         roots;
    PlantAllometryGenome    allometry;
    PlantPhenologyGenome    phenology;
    PlantPlasticityGenome   plasticity;
} PlantGenomeTraits;

typedef struct {
    uint32_t          schema_version;
    uint32_t          species_id;
    uint64_t          individual_id;
    uint64_t          parent_id[2];
    uint64_t          genetic_seed;
    PlantGenomeTraits traits;
    /* Legacy/convenience sub-genomes for direct floral, fruit & surface pigment access */
    PlantArchitectureGenome architecture;
    PlantPhyllotaxisGenome  phyllotaxis;
    PlantMeristemGenome     meristem;
    PlantLeafGenome         leaf;
    PlantFlowerGenome       flower;
    PlantFruitGenome        fruit;
    PlantPhysiologyGenome   physiology;
    PlantPlasticityGenome   plasticity;
    PlantAllometryGenome    allometry;
} PlantGenome;
```

---

## 3. Species Definitions, Trait Priors, Cholesky Correlations & Inheritance

```c
typedef enum {
    PLANT_TRAIT_PRIMARY_EXTENSION = 1,
    PLANT_TRAIT_LATERAL_EXTENSION = 2,
    PLANT_TRAIT_INTERNODE_LENGTH  = 3,
    PLANT_TRAIT_BRANCH_ANGLE      = 4,
    PLANT_TRAIT_BUD_ACTIVATION    = 5,
    PLANT_TRAIT_LEAF_LENGTH       = 6,
    PLANT_TRAIT_LEAF_WIDTH_RATIO  = 7,
    PLANT_TRAIT_ROOT_DEPTH        = 8,
    PLANT_TRAIT_BRANCH_EXPONENT   = 9
} PlantTraitId;

typedef struct {
    PlantTraitId      trait;
    PlantDistribution population_distribution;
    float             heritability;
    float             mutation_sigma;
} PlantTraitPrior;

typedef struct {
    uint16_t      trait_count;
    PlantTraitId *traits;
    /* Row-major trait_count x trait_count symmetric positive-semidefinite matrix */
    float        *correlation_matrix;
} PlantCorrelationGroup;

typedef enum {
    PLANT_ALG_AXIS_EXTENSION_V1     = 1,
    PLANT_ALG_BUD_ACTIVATION_V1     = 2,
    PLANT_ALG_PHYLLOTAXIS_V1        = 3,
    PLANT_ALG_BRANCHING_V1          = 4,
    PLANT_ALG_TROPISM_V1            = 5,
    PLANT_ALG_DA_VINCI_ALLOMETRY_V1 = 6,
    PLANT_ALG_SECONDARY_GROWTH_V1   = 7,
    PLANT_ALG_LEAF_EXPANSION_V1     = 8,
    PLANT_ALG_SENESCENCE_V1         = 9
} PlantAlgorithmId;

typedef struct {
    uint16_t algorithm_id;
    uint16_t algorithm_version;
} PlantAlgorithm;

typedef struct {
    PlantAlgorithm axis_growth;
    PlantAlgorithm bud_development;
    PlantAlgorithm branching;
    PlantAlgorithm phyllotaxis;
    PlantAlgorithm leaf_development;
    PlantAlgorithm root_development;
    PlantAlgorithm tropism;
    PlantAlgorithm secondary_growth;
    PlantAlgorithm senescence;
} PlantDevelopmentProgram;

typedef struct {
    uint32_t                schema_version;
    uint32_t                species_id;
    const char             *scientific_name;
    const char             *common_name;
    PlantGenomeTraits       defaults;
    PlantTraitPrior        *priors;
    size_t                  prior_count;
    PlantCorrelationGroup  *correlations;
    size_t                  correlation_count;
    PlantDevelopmentProgram development;
} PlantSpeciesDef;
```

### 3.1 Correlated Multi-Trait Sampling via Cholesky Decomposition
Independent random sampling of traits can produce biologically implausible individuals. For each `PlantCorrelationGroup` of size $K$:
1. `plant_species_validate()` verifies that `correlation_matrix` $\mathbf{\Sigma}$ is symmetric ($\Sigma_{ij} = \Sigma_{ji}$), has a unit diagonal ($\Sigma_{ii} = 1$), and is positive semidefinite by computing its lower-triangular Cholesky factor $\mathbf{L}$ ($\mathbf{\Sigma} = \mathbf{L}\mathbf{L}^T$).
2. During `plant_genome_sample()`, an independent standard normal vector $\mathbf{z} \sim \mathcal{N}(\mathbf{0}, \mathbf{I}_K)$ is transformed into correlated normal variates $\mathbf{y} = \mathbf{L}\mathbf{z}$, which then scale each trait's `population_distribution` ($\mu_i + y_i \sigma_i$, clamped to $[\text{min}_i, \text{max}_i]$).

### 3.2 Quantitative Inheritance (`plant_genome_cross`)
When crossing two parent genomes (`mother`, `father`) under `species`:
$$T_{\text{child}} = h^2 \left( \frac{T_{\text{mother}} + T_{\text{father}}}{2} \right) + (1 - h^2)\, \mu_{\text{species}} + \epsilon, \qquad \epsilon \sim \mathcal{N}(0, \sigma_{\text{mut}}^2)$$
where $h^2 \in [0, 1]$ is the trait's `heritability` and $\sigma_{\text{mut}}$ is its `mutation_sigma` from `PlantTraitPrior`, setting `out->parent_id[0] = mother->individual_id` and `out->parent_id[1] = father->individual_id`.

---

## 4. The Phenotype: Index-Addressed Multiscale Tree Graph (MTG)

```c
typedef struct {
    PlantAxisId    id;
    PlantAxisId    parent_axis;
    PlantBudId     origin_bud;
    PlantAxisClass axis_class;
    uint8_t        branching_order;
    uint8_t        alive;
    PlantUnitId    first_unit;
    PlantUnitId    last_unit;
    float          birth_day;
    float          age_days;
} PlantAxis;

typedef enum {
    PLANT_UNIT_PHYTOMER = 0,
    PLANT_UNIT_ROOT_SEGMENT,
    PLANT_UNIT_REPRODUCTIVE
} PlantUnitType;

typedef struct {
    PlantUnitId   id;
    PlantAxisId   axis_id;
    PlantUnitId   previous;
    PlantUnitId   next;
    PlantUnitType type;
    float         birth_day;
    float         length_m;
    float         radius_m;
    float         local_azimuth_rad;
    float         local_elevation_rad;
    PlantOrganId  first_organ;
    PlantBudId    first_bud;
} PlantGrowthUnit;

typedef enum {
    PLANT_BUD_DORMANT = 0,
    PLANT_BUD_ACTIVE,
    PLANT_BUD_DEVELOPED,
    PLANT_BUD_DEAD,
    PLANT_BUD_FLOWERING,
    PLANT_BUD_ABORTED,
    PLANT_BUD_SENESCENT
} PlantBudState;

typedef struct {
    PlantBudId     id;
    PlantUnitId    owner_unit;
    PlantBudId     next_on_unit;
    PlantAxisClass target_axis_class;
    PlantBudState  state;
    PlantAxisId    resulting_axis;
    float          activation_probability;
    float          birth_day;
    float          age_days;
} PlantBud;

typedef enum {
    PLANT_ORGAN_LEAF = 0,
    PLANT_ORGAN_FLOWER,
    PLANT_ORGAN_FRUIT,
    PLANT_ORGAN_SEED,
    PLANT_ORGAN_THORN,
    PLANT_ORGAN_OTHER,
    PLANT_ORGAN_ROOT_SEGMENT
} PlantOrganType;

typedef struct {
    PlantOrganId   id;
    PlantUnitId    owner_unit;
    PlantOrganId   next_on_unit;
    PlantOrganType type;
    float          birth_day;
    float          age_days;
    float          length_m;
    float          width_m;
    float          attachment_angle_rad;
    PlantVec3      local_position;
} PlantOrgan;

typedef struct {
    PlantAxis       *axes;
    size_t           axis_count;
    size_t           axis_capacity;

    PlantGrowthUnit *units;
    size_t           unit_count;
    size_t           unit_capacity;

    PlantBud        *buds;
    size_t           bud_count;
    size_t           bud_capacity;

    PlantOrgan      *organs;
    size_t           organ_count;
    size_t           organ_capacity;
} PlantPhenotype;
```

- All identifiers are integer array indices (`PLANT_NONE = UINT32_MAX` sentinel).
- Dormant buds remain in the `PlantPhenotype` graph (`PLANT_BUD_DORMANT`) so they can activate later in response to pruning, light opening, or seasonal flushes, and developed buds retain `resulting_axis` (`PLANT_BUD_DEVELOPED`).
- Both shoot systems (`PLANT_AXIS_PRIMARY`, `PLANT_AXIS_LATERAL`, `PLANT_AXIS_SHORT_SHOOT`, `PLANT_AXIS_REPRODUCTIVE`) and root systems (`PLANT_AXIS_TAPROOT`, `PLANT_AXIS_LATERAL_ROOT`) coexist in the same `PlantPhenotype` graph.

---

## 5. Spatial Environment Sampling, Chrono-Determinism & Ephemeral Delta Decay

### 5.1 Per-Meristem 3D Environment Sampler (`PlantEnvironmentSampler`)

```c
typedef struct {
    float     photosynthetic_daily_light_mol_m2;
    float     mean_air_temperature_C;
    float     soil_water_potential_MPa;
    float     soil_nutrient_availability;
    float     local_light_availability;
    float     wind_speed_m_s;
    PlantVec3 light_direction;
    PlantVec3 gravity_direction;
} PlantEnvironment;

typedef PlantEnvironment (*PlantEnvironmentSampler)(
    void     *context,
    PlantVec3 position,
    float     time_days
);

typedef struct {
    uint64_t       instance_id;
    PlantGenome    genome;
    PlantPhenotype phenotype;
    float          age_days;
    float          accumulated_degree_days;
    float          carbon_reserves_g;
    float          water_status;
    uint64_t       simulation_seed;
} PlantInstance;
```

Instead of assuming a uniform environment across the entire plant, every developing meristem samples `environment(context, meristem_pos_3d, time_days)` at its actual 3D world position. A branch on the sunlit side of a crown or near a light gap receives higher `local_light_availability` and phototropic pull than a shaded interior branch.

### 5.2 Topological Coordinate Hashing (`topo_hash`) for $\Delta t$-Invariance
Every `PlantGrowthUnit` $u$, `PlantBud` $b$, and `PlantOrgan` $o$ derives a deterministic 64-bit topological path hash $H_u$ from the collar seed:
- **Collar / Root Origin**: $H_0 = \text{SplitMix64}(\text{genetic\_seed} \oplus \text{0x504C414E545F3030ULL})$
- **Apical Continuation** (unit index $m$ along an axis): $H_{u'} = \text{SplitMix64}(H_u \oplus \text{0x41504943414C5F31ULL} \oplus m)$
- **Lateral Branching** (bud slot $k$ of order $d+1$): $H_{\text{child}} = \text{SplitMix64}(H_u \oplus \text{0x4C41544552414C32ULL} \oplus ((d + 1) \ll 16) \oplus k)$

Because every stochastic draw (internode length, branch angle, phyllotactic jitter, bud activation, organ size) is a pure function of $(H_u, \text{CHANNEL})$, `plant_simulate_step()` is **100% invariant to timestep size $\Delta t$**.

### 5.3 Ephemeral Delta Decay (`PlantDeltaRecord`)
Unvisited wild areas consume **0 bytes** in memory/storage. Player perturbations are stored as time-bounded deltas:
$$\eta(t) = \frac{t - t_{\text{event}}}{t_{\text{heal}}} \in [0, 1]$$
- At $\eta = 0$: A pruned axis (`PLANT_DELTA_PRUNE_AXIS`) terminates at `target_topo_hash`, releasing apical dominance on dormant lateral buds immediately below the cut.
- For $0 < \eta < 1$: The regrowing subtree develops at effective catch-up age $\eta \cdot A_{\text{canonical}}$.
- At $\eta \ge 1$: The delta expires (`plant_deltas_compact_expired`), consumes 0 bytes, and the plant mathematically converges back to its unaltered canonical phenotype.

---

## 6. Explicit Serialization, Schema Versioning & Validation

- **No Raw Struct Dumps**: `plant_genome_serialize` / `plant_genome_deserialize` and `plant_phenotype_serialize` / `plant_phenotype_deserialize` encode fields explicitly in little-endian portable binary format tagged with `PLANT_GENOME_SCHEMA_VERSION` and `PLANT_GRAPH_SCHEMA_VERSION`.
- **Schema Migration**: `plant_genome_migrate_v1_to_v2()` and `plant_species_migrate_v1_to_v2()` provide forward-compatible upgrade hooks.
- **Mandatory Validation (`plant_species_validate`, `plant_genome_validate`, `plant_phenotype_validate`)**:
  - Growth probabilities and heritabilities in $[0, 1]$
  - Distribution $\sigma \ge 0$ and $\text{min} \le \text{mean} \le \text{max}$
  - Positive branch radii, leaf dimensions, and allometric exponents
  - Finite angles in permitted ranges
  - Symmetric, unit-diagonal, positive-semidefinite correlation matrices (verified via Cholesky factorization)
  - Validated graph references (`owner_unit`, `parent_axis`, `origin_bud`, `first_unit`, `last_unit`, `resulting_axis`) and cycle-free axis parent topology
  - Supported `PlantAlgorithmId` and `algorithm_version` in `PlantDevelopmentProgram`

---

## 7. Canonical C API

```c
typedef enum {
    PLANT_OK = 0,
    PLANT_ERR_NULL_PTR,
    PLANT_ERR_SCHEMA_VERSION,
    PLANT_ERR_INVALID_PROBABILITY,
    PLANT_ERR_INVALID_DISTRIBUTION,
    PLANT_ERR_INVALID_DIMENSION,
    PLANT_ERR_INVALID_ANGLE,
    PLANT_ERR_INVALID_ALLOMETRY,
    PLANT_ERR_INVALID_HERITABILITY,
    PLANT_ERR_INVALID_CORRELATION_MATRIX,
    PLANT_ERR_UNSUPPORTED_ALGORITHM,
    PLANT_ERR_GRAPH_DANGLING_REF,
    PLANT_ERR_GRAPH_CYCLE,
    PLANT_ERR_CAPACITY_EXCEEDED,
    PLANT_ERR_SERIALIZE_BUFFER
} PlantStatus;

PlantStatus plant_species_validate(const PlantSpeciesDef *species);

PlantStatus plant_genome_sample(
    const PlantSpeciesDef *species,
    uint64_t seed,
    uint64_t individual_id,
    PlantGenome *out
);

PlantStatus plant_genome_cross(
    const PlantSpeciesDef *species,
    const PlantGenome *mother,
    const PlantGenome *father,
    uint64_t seed,
    PlantGenome *out
);

PlantStatus plant_instance_init(
    PlantInstance *plant,
    const PlantGenome *genome
);

void plant_instance_destroy(
    PlantInstance *plant
);

PlantStatus plant_simulate_step(
    PlantInstance *plant,
    PlantEnvironmentSampler environment,
    void *environment_context,
    float time_days,
    float delta_days
);

PlantStatus plant_geometry_build(
    const PlantInstance *plant,
    uint32_t detail_level,
    void *geometry_output
);
```

---

## 8. Stage 3 & Stage 4 Botanical Realism Plan (Skyrim-Style Flora & PBR Canopy Lighting)

### 8.1 Root Cause of Sparse / Low-Detail Phenotype Rendering

When `nd_m3d_build_plant_from_genes()` (`mods/nd/ux/nd_seven_mesh3d.h`) was wired to `plant_simulate_at_time()` (`external/nd-plant/include/nd/plant_model.h`) and `plant_geometry_build()` (`mods/nd/ux/nd_seven_mesh3d.h`), Stage 4 stopped calling the high-detail 3D botanical cluster primitives already defined in `nd_seven_mesh3d.h`:

1. **Conifers (`Pseudotsuga menziesii`, `Abies alba`, `Pinus sylvestris`)**: `plant_sim_develop_axis_recursive()` capped primary trunks at only `4` vertical nodes, and `plant_geometry_build()` only emitted a single 4-triangle frond spray per leaf organ—never calling `nd_m3d_add_conifer_tier()`.
2. **Broadleaf Trees (`Quercus robur`, `Acer pseudoplatanus`, `Betula pendula`, `Betula pubescens`, `Salix babylonica`, `Acacia senegal`, `Olea europaea`)**: `plant_geometry_build()` only emitted a single low-poly `5×2` sphere at branch tips instead of calling `nd_m3d_add_foliage_bough_cluster()` (which builds compound `8×3` central sub-crowns, 2 staggered satellite billows, and 3 radiating V-cupped leaf sprays with porous alpha-cutout fringes). Also, juvenile trees (`is_big == 0`) were simulated at `0.48 * full_horizon_days`, leaving them half-bare.
3. **Weeping Crowns & Understory Herbs**: Weeping willows and silver birches only emitted 1 tiny frond per tip instead of cascading weeping curtain streamers (`nd_m3d_add_arching_blade()`), and herbs (`Linum`, `Daucus`, `Solanum`, `Bellis`) lacked ground-hugging basal rosettes.
4. **Billboard Sprite Baker**: `nd_seven_mesh3d_bake_ortho_rgba()` quantized colors into 4 flat cartoon steps with a dark 1px pixel-art outline, clashing with the continuous 3D WebGL PBR/subsurface shader (`s_fs_mesh3d` in `mods/nd/ux/nd_seven_gl.c`).

### 8.2 Implementation Steps (Preserving 4-Stage `PlantSpeciesDef → PlantGenome → PlantPhenotype → plant_geometry_build` Architecture)

1. **Boreal & Alpine Conifers (Falkreath / Riverwood / Haafingar Pines & Firs)**
   - In `plant_sim_develop_axis_recursive()`, allow `PLANT_ARCH_MASSART` conifer trunks (`order == 0`) to develop **6 to 8 rhythmic vertical whorl tiers** from lower skirt to spire apex.
   - In `plant_geometry_build()`, drive `nd_m3d_add_conifer_tier()` directly from the MTG trunk whorl nodes (`u->branch_order == 0` on `PLANT_ARCH_MASSART`) so every tier gets 7–11 radiating plumose needle boughs, secondary drooping pectinate fanlets, sweeping skirt blades, and real inter-whorl sky gaps—plus upright seed cones on upper `Abies alba` whorls.
   - For `Pinus sylvestris` (Scots Pine), emit high self-pruned flaky ochre-red upper bark and multi-layered `nd_m3d_add_foliage_bough_cluster()` needle pads at every upper scaffold fork and crown umbrella apex.

2. **Billowing Broadleaf Canopies, Rift-Style Birches & Weeping Willows**
   - In `nd_m3d_build_plant_from_genes()`, develop `is_big == 0` trees to `0.74f * full_horizon_days` so young trees have full architectural branching scaled to their smaller metric dimensions (`w_wu, h_wu`).
   - In `plant_geometry_build()`, replace the single `5×2` lobe with `nd_m3d_add_foliage_bough_cluster()` at every terminal and sub-terminal foliated shoot (`org->sub_index == 0`), shaped by the genome's `orthotropy` and `blade_length_m` (deep billowing domes for `Quercus` and `Acer`, airy tiered clouds over chalk-white bark with dark diamond basal fissures for `Betula`, twisted Mediterranean pads for `Olea`, and wide tabular pads for `Acacia`).
   - For weeping genomes (`g->architecture.orthotropy < -0.25f`: `Salix babylonica` and `Betula pendula`), emit cascading 3D weeping curtain streamers via `nd_m3d_add_arching_blade()` draping from the outer crown boughs.

3. **Lush Tundra/Meadow Understory Herbs, Wildflowers & Wetland Reeds**
   - In `plant_geometry_build()`, emit a full ground-hugging **basal rosette** of 6–8 radiating 3D arching leaf blades (`nd_m3d_add_arching_blade()`) around the root collar of herbaceous species (`Linum`, `Daucus`, `Solanum`, `Bellis`), dense sweeping sword-blade clumps + cigar-brown spadix spikes for `Typha latifolia`, and multi-star umbels/blossoms/berries on terminal floral organs—while keeping total vertices across all 128 plant templates + 51 stone templates well inside the `262,144` VBO vertex budget.

4. **Skyrim Nordic Palette & Continuous PBR Lighting in the Ortho Baker and 3D Shader**
   - Tune species foliage and bark palettes in `plant_species_init_preset()` for authentic Nordic forest tones (deep evergreen spruce/pine blue-greens, sunlit Rift-style golden-lime birch foliage, weathered mossy slate-brown oak/maple bark).
   - Upgrade `nd_seven_mesh3d_bake_ortho_rgba()` to use smooth physical directional sun lighting, subsurface chlorophyll rim-scatter, and deep sub-canopy ambient occlusion (removing the harsh 4-step cartoon quantization and dark cartoon outline) so distant/midground baked sprites blend seamlessly with `s_fs_mesh3d`.

### 8.3 SpeedTree / Creation Engine (Skyrim)–Style 3D Branchlet-Card + UV Botanical Silhouette & Dome-Normal Pipeline

To eliminate stylized low-poly "cotton-ball spheres + isotropic Perlin sponge holes" and achieve genuine Skyrim/SpeedTree botanical realism without changing the 48-byte `nd_mesh3d_vert_t` VBO stride:

1. **Zero-Overhead UV & Botanical Archetype Encoding inside the 48-Byte Vertex (`nd_mesh3d_vert_t`)**:
   - **`a_vpos.w` (joint Wind Weight & Longitudinal Card $u \in [0, 1]$)**:
     - Pack `a_vpos.w = floor(u * 250.0f + 0.5f) + clamp(wind_w, 0.01f, 0.99f)` on branchlet cards ($u = 0$ at twig attachment base to $u = 1$ at distal tip).
     - Unpack in `s_vs_mesh3d` and `nd_seven_mesh3d_bake_ortho_rgba()`: `wgt = (a_vpos.w > 1.0f) ? fract(a_vpos.w) : a_vpos.w; cardU = (a_vpos.w > 1.0f) ? (floor(a_vpos.w) / 250.0f) : 0.0f;`.
   - **`a_vcol.a` (Material Class + Botanical Leaf Archetype + Lateral Card $v \in [0, 1]$)**:
     - `0.200 .. 0.238` (`ND_MAT3D_FOL_CONIFER`): Pectinate conifer needle bough (`Pseudotsuga`, `Abies`, `Pinus`), with $v = (\text{mat} - 0.200) / 0.038 \in [0, 1]$ ($v = 0.5$ on central woody rachis, $v \to 0, 1$ at left/right needle comb tips).
     - `0.240 .. 0.278` (`ND_MAT3D_FOL_OVATE`): Serrated birch / willow / olive / acacia twig-and-leaf spray (`Betula`, `Salix`, `Olea`, `Acacia`), with $v = (\text{mat} - 0.240) / 0.038 \in [0, 1]$.
     - `0.280 .. 0.318` (`ND_MAT3D_FOL_LOBED`): Lobed oak / palmate maple sub-crown branchlet (`Quercus`, `Acer`), with $v = (\text{mat} - 0.280) / 0.038 \in [0, 1]$.
     - `0.320 .. 0.350` (`ND_MAT3D_FOL_BLADE`): Parallel-veined sword/rosette leaf blade (`Typha`, `Linum`, `Daucus`, `Solanum`, `Bellis`), with $v = (\text{mat} - 0.320) / 0.030 \in [0, 1]$.
     - `0.185` (`ND_MAT3D_FOLIAGE_CORE`): Solid succulent apex cap (`Arthrocereus`).

2. **Intersecting 3D Branchlet Cards (`nd_m3d_add_branchlet_card`) with Outward Dome-Transferred Normals**:
   - Retire closed ellipsoid spheres (`nd_m3d_add_foliage_lobe`) from all tree crowns.
   - Construct tree crowns from multi-layered, intersecting, cambered 3D branchlet sheets (`nd_m3d_add_branchlet_card`) with explicit axial roll ($\pm 32^\circ\text{--}45^\circ$), gravitropic bow, V-cupped dihedral wings, and **outward dome-transferred normals** (`70%` spherical sub-crown dome vector $\mathbf{p} - \mathbf{c}_{\text{crown}}$, `30%` cambered sheet normal) with unbaked physical chlorophyll albedo.
   - Add root-collar buttress spurs on mature tree trunks, dead self-pruned lower conifer branch stubs on `Pinus` and `Pseudotsuga`, and smooth `tanh` radial crown clamping so branches never kink.

3. **UV-Space Procedural Botanical Silhouette & Venation Synthesizer (`s_fs_mesh3d` & Ortho Baker)**:
   - **`ND_MAT3D_FOL_CONIFER`**: Central resinous woody twig rachis ($|v - 0.5| < 0.045(1 - 0.65u)$) + high-frequency angled pectinate needle comb fascicles ($\theta \approx 42^\circ$, 18–22 needles per bough + secondary sub-shoot V-forks), discarding inter-needle gaps and perturbing normals across each cylindrical needle.
   - **`ND_MAT3D_FOL_OVATE` & `ND_MAT3D_FOL_LOBED`**: Central forking shoot stem + 7 distinct overlapping leaves in alternate/sub-opposite phyllotaxis, each evaluated in its local leaf frame $(\xi, \eta)$ with pointed serrated (`Betula`/`Salix`/`Olea`) or sinuate-lobed (`Quercus`/`Acer`) margin SDF cutouts and raised pinnate/palmate venation normals.
   - **`ND_MAT3D_FOL_BLADE`**: Tapered lanceolate/sword blade silhouette in $(u, v)$ with parallel longitudinal vascular ridges and sharp pointed apex.
   - **Nordic Bark & Basal Moss (`isBark`)**: Anisotropic vertical bark plates, deep cambium fissures, birch horizontal lenticels, and subtle basal forest-floor moss weathering.


