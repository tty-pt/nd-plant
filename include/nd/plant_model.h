/*
 * plant_model.h — Pure-C Botanical Genetics, Multi-Scale Tree Graph (MTG)
 * Phenotype, Chrono-Deterministic Developmental Simulator, and Ephemeral
 * Delta Decay Engine.
 *
 * Implements the 4-layer architecture documented in external/nd-plant/IDEA.md:
 *   1. PlantSpeciesDef  (Taxonomic metadata, architectural model, trait priors)
 *   2. PlantGenome      (9 modular sub-genomes sampled or bred per individual)
 *   3. PlantPhenotype   (Index-addressed MTG: Axes, GrowthUnits, Buds, Organs)
 *   4. Chrono-Deterministic Simulator with Coordinate-Addressed PRNG (topo_hash)
 *      and Ephemeral Delta Decay (self-healing branch regrowth & zero storage).
 *
 * Pure C99/C11 header-inline implementation so it compiles identically in:
 *   - external/nd-plant (libnd-plant.so server module)
 *   - mods/nd/ux (both native SSR .so and browser wasm32 via plant.h)
 *   - standalone unit tests
 */

#ifndef ND_PLANT_MODEL_H
#define ND_PLANT_MODEL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * 1. Fundamental Botanical & Math Primitives
 * ============================================================================ */

#define PLANT_SCHEMA_VERSION         1u
#define PLANT_SPECIES_SCHEMA_VERSION 1u
#define PLANT_GENOME_SCHEMA_VERSION  1u
#define PLANT_GRAPH_SCHEMA_VERSION   1u

#define PLANT_NONE       0xFFFFFFFFu
#define PLANT_INVALID_ID PLANT_NONE

typedef uint32_t PlantSpeciesId;
typedef uint32_t PlantAxisId;
typedef uint32_t PlantUnitId;
typedef uint32_t PlantBudId;
typedef uint32_t PlantOrganId;

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

typedef struct { float x, y, z; }       PlantVec3;
typedef struct { float x, y, z, w; }    PlantQuat;
typedef struct { float r, g, b, a; }    PlantColor4;

/* Intra-individual trait distribution (variation across organs of ONE plant) */
typedef struct {
	float mean;
	float sigma;
	float min;
	float max;
} PlantDistribution;

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

/* Inter-individual species prior (variation across individuals of a SPECIES) */
typedef struct {
	float             mean_prior;
	float             mean_sigma;
	float             intra_sigma;
	float             hard_min;
	float             hard_max;
	/* v1.0 canonical trait-indexed prior fields */
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

/* ============================================================================
 * 2. Botanical Taxonomic & Morphological Enumerations
 * ============================================================================ */

typedef enum {
	PLANT_TREE = 0,
	PLANT_SHRUB = 1,
	PLANT_SUBSHRUB = 2,
	PLANT_HERB = 3,
	PLANT_GRASS = 4,
	PLANT_FERN = 5,
	PLANT_VINE = 6,
	PLANT_SUCCULENT = 7,
	/* Aliases */
	PLANT_LIFEFORM_TREE      = PLANT_TREE,
	PLANT_LIFEFORM_SHRUB     = PLANT_SHRUB,
	PLANT_LIFEFORM_HERB      = PLANT_HERB,
	PLANT_LIFEFORM_GRAMINOID = PLANT_GRASS,
	PLANT_LIFEFORM_VINE      = PLANT_VINE,
	PLANT_LIFEFORM_FERN      = PLANT_FERN,
	PLANT_LIFEFORM_SUCCULENT = PLANT_SUCCULENT
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

/* Hallé & Oldeman (1970) tropical & temperate tree architectural models */
typedef enum {
	PLANT_ARCH_CORNER = 0,   /* Unbranched monocaul / palm / columnar stem */
	PLANT_ARCH_RAUH,         /* Monopodial rhythmic trunk & rhythmic tiers (Pinus, Quercus, Acer) */
	PLANT_ARCH_MASSART,      /* Monopodial orthotropic trunk + plagiotropic whorled tiers (Abies, Pseudotsuga) */
	PLANT_ARCH_LEEUWENBERG,  /* Sympodial 3D equal forks (dichasial/trichasial umbrella / shrubs) */
	PLANT_ARCH_TROLL,        /* Plagiotropic axes with secondary erection / weeping arches (Salix, Acacia) */
	PLANT_ARCH_SCHOUTE,      /* Dichotomous / basal clump branching */
	PLANT_ARCH_HOLTTUM,      /* Monocaul determinate monocarpic */
	PLANT_ARCH_CHAMBERLAIN,  /* Linear sympodial relay */
	PLANT_ARCH_MCCLURE       /* Basitonic culm / rhizomatous clump (Typha, Linum, grasses) */
} PlantArchitecturalModel;

typedef enum {
	PLANT_PHYLLOTAXIS_ALTERNATE = 0,
	PLANT_PHYLLOTAXIS_OPPOSITE = 1,
	PLANT_PHYLLOTAXIS_DECUSSATE = 2,
	PLANT_PHYLLOTAXIS_WHORLED = 3,
	PLANT_PHYLLOTAXIS_SPIRAL = 4,
	PLANT_PHYLLOTAXIS_ROSETTE = 5,
	/* Aliases */
	PLANT_PHYLLO_ALTERNATE = PLANT_PHYLLOTAXIS_ALTERNATE,
	PLANT_PHYLLO_OPPOSITE  = PLANT_PHYLLOTAXIS_OPPOSITE,
	PLANT_PHYLLO_DECUSSATE = PLANT_PHYLLOTAXIS_DECUSSATE,
	PLANT_PHYLLO_WHORLED   = PLANT_PHYLLOTAXIS_WHORLED,
	PLANT_PHYLLO_SPIRAL    = PLANT_PHYLLOTAXIS_SPIRAL
} PlantPhyllotaxisType;
typedef PlantPhyllotaxisType PlantPhyllotaxisMode;

typedef enum {
	PLANT_LEAF_SIMPLE = 0,
	PLANT_LEAF_COMPOUND_PINNATE = 1,
	PLANT_LEAF_COMPOUND_BIPINNATE = 2,
	PLANT_LEAF_COMPOUND_PALMATE = 3,
	PLANT_LEAF_TRIFOLIATE = 4,
	PLANT_LEAF_NEEDLE = 5,
	PLANT_LEAF_SCALE = 6,
	/* Aliases */
	PLANT_LEAF_PINNATE   = PLANT_LEAF_COMPOUND_PINNATE,
	PLANT_LEAF_BIPINNATE = PLANT_LEAF_COMPOUND_BIPINNATE,
	PLANT_LEAF_PALMATE   = PLANT_LEAF_COMPOUND_PALMATE
} PlantLeafType;
typedef PlantLeafType PlantLeafComplexity;

typedef enum {
	PLANT_LEAF_SHAPE_LINEAR = 0,
	PLANT_LEAF_SHAPE_LANCEOLATE,
	PLANT_LEAF_SHAPE_ELLIPTIC,
	PLANT_LEAF_SHAPE_OVATE,
	PLANT_LEAF_SHAPE_OBOVATE,
	PLANT_LEAF_SHAPE_CORDATE,
	PLANT_LEAF_SHAPE_RENIFORM,
	PLANT_LEAF_SHAPE_ROUNDED,
	PLANT_LEAF_SHAPE_PALMATELY_LOBED,
	PLANT_LEAF_SHAPE_PINNATELY_LOBED,
	PLANT_LEAF_SHAPE_OTHER
} PlantLeafShape;

typedef enum {
	PLANT_MARGIN_ENTIRE = 0,
	PLANT_MARGIN_SERRATE,
	PLANT_MARGIN_DENTATE,
	PLANT_MARGIN_CRENATE,
	PLANT_MARGIN_LOBED,
	PLANT_MARGIN_UNDULATE,
	PLANT_MARGIN_SPINY
} PlantLeafMargin;

typedef enum {
	PLANT_VENATION_PARALLELODROMOUS = 0,
	PLANT_VENATION_PINNATE,
	PLANT_VENATION_PALMATE,
	PLANT_VENATION_ARCUATE,
	PLANT_VENATION_RETICULATE
} PlantVenationType;

typedef enum {
	PLANT_INFLORESCENCE_SOLITARY = 0,
	PLANT_INFLORESCENCE_SPIKE,
	PLANT_INFLORESCENCE_RACEME,
	PLANT_INFLORESCENCE_PANICLE,
	PLANT_INFLORESCENCE_UMBEL,
	PLANT_INFLORESCENCE_CAPITULUM,
	PLANT_INFLORESCENCE_CATKIN
} PlantInflorescenceType;

typedef enum {
	PLANT_FLOWER_ACTINOMORPHIC = 0,
	PLANT_FLOWER_ZYGOMORPHIC
} PlantFlowerSymmetry;

typedef enum {
	PLANT_COROLLA_FREE_PETALS = 0,
	PLANT_COROLLA_CAMPANULATE,
	PLANT_COROLLA_TUBULAR,
	PLANT_COROLLA_FUNNELFORM,
	PLANT_COROLLA_PAPILIONACEOUS,
	PLANT_COROLLA_LIGULATE
} PlantCorollaType;

typedef enum {
	PLANT_FRUIT_NONE = 0,
	PLANT_FRUIT_BERRY,
	PLANT_FRUIT_DRUPE,
	PLANT_FRUIT_POME,
	PLANT_FRUIT_CAPSULE,
	PLANT_FRUIT_LEGUME,
	PLANT_FRUIT_SAMARA,
	PLANT_FRUIT_ACHENE,
	PLANT_FRUIT_NUT,
	PLANT_FRUIT_CONE
} PlantFruitType;

typedef enum {
	PLANT_ROOT_TAPROOT = 0,
	PLANT_ROOT_FIBROUS,
	PLANT_ROOT_ADVENTITIOUS,
	PLANT_ROOT_TUBEROUS
} PlantRootArchitecture;

typedef enum {
	PLANT_EVERGREEN = 0,
	PLANT_SEMIEVERGREEN,
	PLANT_DECIDUOUS
} PlantLeafPersistence;

/* ============================================================================
 * 3. Modular Individual Genome (PlantGenome)
 * ============================================================================ */

/* Per-axis-class developmental genome (IDEA.md §2.3) */
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

typedef struct {
	PlantArchitecturalModel model;
	uint8_t                 max_branch_order;
	uint8_t                 basal_stem_count;      /* 1 = single bole, >1 = multi-stem clump */
	PlantDistribution       internode_length_m;
	PlantDistribution       branch_angle_deg;      /* insertion angle from parent axis (0=parallel, 90=perp) */
	PlantDistribution       roll_variation_deg;
	float                   apical_dominance;      /* [0..1] inhibition of lateral buds by active apex */
	float                   acrotony;              /* [0..1] 1=distal/upper branch bias, 0=mesotonic */
	float                   basitony;              /* [0..1] 1=basal shrub/clump sprout bias */
	float                   clear_bole_fraction;   /* [0..1] lower trunk self-pruning / branch-free zone */
	float                   fork_probability;      /* [0..1] sympodial apical bifurcation probability */
	float                   orthotropy;            /* [-1..1] +1=erect skyward, 0=plagiotropic, -1=pendulous/weeping */
	float                   tortuosity;            /* [0..1] per-internode gnarl / helical twist */
	float                   branch_shed_threshold; /* [0..1] shade self-pruning sensitivity */
	float                   crown_flatness;        /* [0.15..1.50] vertical crown aspect (0.26=tabular umbrella, 0.75=dome, 1.35=columnar) */
} PlantArchitectureGenome;

typedef struct {
	union { PlantPhyllotaxisType arrangement; PlantPhyllotaxisMode mode; };
	union { uint8_t organs_per_node;          uint8_t buds_per_node;     };
	float   divergence_angle_rad;
	float   whorl_rotation_rad;
	float   angular_variation_rad;
	float   divergence_angle_deg; /* e.g., 137.50776f (golden angle), 180.0f, 90.0f */
	float   divergence_jitter_deg;
	bool    decussate_phase_flip;
} PlantPhyllotaxisGenome;

typedef struct {
	float plastochron_days;          /* thermal days between successive phytomer emissions */
	float lateral_bud_probability;   /* base probability an axillary bud breaks dormancy */
	float flower_bud_probability;    /* probability a reproductive bud forms on mature shoots */
	float dormancy_release_threshold;
	float mortality_probability;
	float sylleptic_branch_ratio;    /* [0..1] immediate vs proleptic (delayed next-flush) branching */
	float vigor_decay_per_order;     /* [0.25..0.90] plastochron/length scaling per branch order */
} PlantMeristemGenome;

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

typedef struct {
	union { float branch_radius_exponent; float pipe_model_exponent; }; /* Da Vinci alpha (1.8..2.6) */
	float branch_constraint_weight;  /* [0..1] soft-constraint weight between free & ideal pipe radius */
	float stem_taper_exponent;
	float branch_length_exponent;
	float branch_length_coefficient;
	float crown_width_height_ratio;
	float crown_asymmetry;
	float basal_radius_ratio;        /* basal trunk radius relative to crown width (e.g. 0.034=birch, 0.072=oak) */
	float tip_min_radius_m;          /* minimum radius of a newly formed terminal twig */
	float radius_taper;              /* passive conductive taper along internodes */
	float wood_density;
	float stiffness_young_modulus;   /* resistance to gravitational bending / drooping */
	float root_flare_strength;       /* basal buttress swelling factor at ground collar */
	float bark_roughness;            /* [0..1] fissure depth / periderm fluting */
	float bark_thickness_ratio;      /* outer bark thickness fraction */
	uint8_t bark_class;              /* ND_PLANT_BARK_* surface shader / pattern class */
	PlantColor4 bark_dark;
	PlantColor4 bark_mid;
	PlantColor4 bark_light;
} PlantAllometryGenome;

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

typedef struct {
	union { PlantLeafType type; PlantLeafComplexity complexity; };
	PlantLeafShape      shape;
	PlantLeafMargin     margin;
	PlantVenationType   venation_type;
	uint8_t             venation_order;
	PlantDistribution   blade_length_m;
	PlantDistribution   width_ratio;
	PlantDistribution   petiole_length_m;
	float               base_angle_ratio;
	float               apex_ratio;
	float               margin_amplitude;
	float               thickness_m;
	PlantDistribution   thickness_mm;
	PlantDistribution   curvature;        /* [-1..1] longitudinal midrib droop/cup */
	PlantDistribution   serration_depth;
	PlantDistribution   lobe_depth;
	uint8_t             leaflet_count;    /* for PINNATE / PALMATE / NEEDLE fascicles */
	uint8_t             leaves_per_node;  /* foliar organs per phytomer node */
	float               chlorophyll_index;
	float               anthocyanin_index;
	float               carotenoid_index;
	float               canopy_porosity;  /* [0.15..0.75] sky gap / foliar porosity fraction */
	float               retention_order_threshold; /* max branch order that retains foliage (e.g. twigs only) */
	PlantColor4         adaxial_deep;
	PlantColor4         adaxial_mid;
	PlantColor4         adaxial_lit;
	PlantColor4         abaxial_high;
} PlantLeafGenome;

typedef struct {
	bool                   has_flowers;
	PlantInflorescenceType inflorescence;
	PlantFlowerSymmetry    symmetry;
	PlantCorollaType       corolla_type;
	uint8_t                sepal_count;
	uint8_t                petal_count;
	uint8_t                stamen_count;
	uint8_t                carpel_count;
	PlantDistribution      flower_radius_m;
	PlantDistribution      pedicel_length_m;
	PlantDistribution      corolla_depth_m;
	PlantColor4            primary_color;
	PlantColor4            secondary_color;
	float                  bloom_onset_maturity; /* [0..1] fraction of mature age when flowering starts */
} PlantFlowerGenome;

typedef struct {
	PlantFruitType    fruit_type;
	PlantDistribution length_m;
	PlantDistribution diameter_m;
	PlantDistribution mass_g;
	PlantColor4       immature_color;
	PlantColor4       mature_color;
	float             ripening_rate;
	float             retention_strength;
} PlantFruitGenome;

typedef struct {
	float light_optimum;
	float light_tolerance;
	float water_optimum;
	float drought_tolerance;
	float temperature_optimum_c;
	float cold_tolerance_c;
	float heat_tolerance_c;
	float nutrient_demand;
	float wind_tolerance;
	float growth_rate;
	float maintenance_cost;
	float max_age_years;
	float mature_height_m;
	float mature_crown_width_m;
} PlantPhysiologyGenome;

typedef struct {
	union { float phototropism; float phototropism_strength; };
	union { float gravitropism; float gravitropism_strength; };
	float hydrotropism;
	float shade_avoidance;
	float drought_sensitivity;
	float temperature_sensitivity;
	union { float wind_flexibility; float wind_thigmomorphogenesis; };
	float drought_root_shoot_shift;
} PlantPlasticityGenome;

/* Complete quantitative trait bundle (IDEA.md §2.10) */
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
	uint32_t                schema_version;
	PlantSpeciesId          species_id;
	uint64_t                individual_id;
	uint64_t                parent_id[2];
	uint64_t                genetic_seed;
	PlantGenomeTraits       traits;

	/* Direct sub-genomes for developmental & 3D mesher access */
	PlantArchitectureGenome architecture;
	PlantPhyllotaxisGenome  phyllotaxis;
	PlantMeristemGenome     meristem;
	PlantAllometryGenome    allometry;
	PlantLeafGenome         leaf;
	PlantRootGenome         roots;
	PlantPhenologyGenome    phenology;
	PlantFlowerGenome       flower;
	PlantFruitGenome        fruit;
	PlantPhysiologyGenome   physiology;
	PlantPlasticityGenome   plasticity;
} PlantGenome;

/* ============================================================================
 * 4. Species Definition & Trait Priors (PlantSpeciesDef)
 * ============================================================================ */

#define PLANT_SPECIES_MAX_INLINE_PRIORS 10u
#define PLANT_SPECIES_MAX_CORR_TRAITS   4u

typedef struct {
	uint32_t                schema_version;
	union { PlantSpeciesId species_id; PlantSpeciesId id; };
	const char             *scientific_name;
	const char             *common_name;
	const char             *slug;
	PlantLifeForm           life_form;
	PlantArchitecturalModel architectural_model;

	/* IDEA.md §3 canonical species defaults, prior table, correlation groups & program */
	PlantGenomeTraits       defaults;
	PlantTraitPrior        *priors;
	size_t                  prior_count;
	PlantCorrelationGroup  *correlations;
	size_t                  correlation_count;
	PlantDevelopmentProgram development;

	/* Inline storage so preset species definitions require zero heap allocations */
	PlantTraitPrior         inline_priors[PLANT_SPECIES_MAX_INLINE_PRIORS];
	PlantTraitId            inline_corr_traits[PLANT_SPECIES_MAX_CORR_TRAITS];
	float                   inline_corr_matrix[PLANT_SPECIES_MAX_CORR_TRAITS * PLANT_SPECIES_MAX_CORR_TRAITS];
	PlantCorrelationGroup   inline_corr_group;

	/* Named population-level priors for continuous sampling across individuals */
	PlantTraitPrior         internode_length_m;
	PlantTraitPrior         branch_angle_deg;
	PlantTraitPrior         apical_dominance;
	PlantTraitPrior         pipe_model_exponent;
	PlantTraitPrior         leaf_length_m;
	PlantTraitPrior         leaf_width_ratio;
	PlantTraitPrior         flower_radius_m;
	PlantTraitPrior         fruit_length_m;
	PlantTraitPrior         growth_rate;
	PlantTraitPrior         max_age_years;

	/* Canonical baseline genome template for this species */
	PlantGenome             canonical_genome;
} PlantSpeciesDef;

/* ============================================================================
 * 5. Multi-Scale Tree Graph Phenotype (PlantPhenotype)
 * ============================================================================ */

typedef enum {
	PLANT_BUD_DORMANT = 0,
	PLANT_BUD_ACTIVE,
	PLANT_BUD_DEVELOPED,
	PLANT_BUD_DEAD,
	PLANT_BUD_FLOWERING,
	PLANT_BUD_ABORTED,
	PLANT_BUD_SENESCENT
} PlantBudState;

typedef enum {
	PLANT_BUD_VEGETATIVE = 0,
	PLANT_BUD_REPRODUCTIVE,
	PLANT_BUD_MIXED
} PlantBudFate;

typedef enum {
	PLANT_UNIT_PHYTOMER = 0,
	PLANT_UNIT_ROOT_SEGMENT,
	PLANT_UNIT_REPRODUCTIVE
} PlantUnitType;

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
	PlantBudId     id;
	PlantAxisId    parent_axis;
	union { PlantUnitId owner_unit; PlantUnitId parent_unit; };
	PlantBudId     next_on_unit;
	PlantAxisClass target_axis_class;
	PlantBudState  state;
	PlantBudFate   fate;
	PlantAxisId    resulting_axis;
	float          activation_probability;
	float          birth_day;
	float          age_days;
	PlantVec3      position;
	PlantVec3      direction;
	float          vigor;
	float          inhibitor_level;
	uint64_t       topo_hash; /* deterministic topological address of this meristem */
} PlantBud;

typedef struct {
	PlantOrganId   id;
	PlantOrganType type;
	union { PlantUnitId owner_unit; PlantUnitId parent_unit; };
	PlantOrganId   next_on_unit;
	float          birth_day;
	float          age_days;
	float          length_m;
	float          width_m;
	float          thickness_m;
	float          attachment_angle_rad;
	union { PlantVec3 local_position; PlantVec3 attachment_pos; };
	PlantQuat      orientation;
	PlantVec3      direction;     /* outward unit axis of petiole/pedicel */
	PlantVec3      normal;        /* adaxial surface normal */
	float          curvature;
	float          maturity;      /* [0..1] unfolding / ripening progress */
	float          health;        /* [0..1] */
	uint8_t        branch_order;
	uint8_t        sub_index;     /* whorl / fascicle / leaflet index */
	uint64_t       topo_hash;
} PlantOrgan;

typedef struct {
	PlantUnitId    id;
	PlantAxisId    axis_id;
	union { PlantUnitId previous; PlantUnitId prev_in_axis; };
	union { PlantUnitId next;     PlantUnitId next_in_axis; };
	PlantUnitId    parent_unit;   /* unit on parent axis if this is first_unit of a lateral axis */
	PlantUnitType  type;
	PlantVec3      base_pos;
	PlantVec3      tip_pos;
	PlantQuat      orientation;
	float          length_m;
	float          base_radius_m;
	float          tip_radius_m;
	float          radius_m;      /* mean conductive + bark radius */
	float          local_azimuth_rad;
	float          local_elevation_rad;
	float          leaf_area_m2;  /* cumulative downstream supported foliage area */
	float          birth_day;
	float          maturity;      /* [0..1] elongation progress of this internode */
	union { uint8_t branching_order; uint8_t branch_order; };
	uint8_t        node_index;    /* 0-based phytomer index along its axis */
	uint64_t       topo_hash;     /* immutable topological coordinate hash H_u */
	PlantBudId     first_bud;
	uint32_t       bud_count;
	PlantOrganId   first_organ;
	uint32_t       organ_count;
} PlantGrowthUnit;

typedef struct {
	PlantAxisId    id;
	PlantAxisId    parent_axis;
	PlantUnitId    parent_unit;
	PlantBudId     origin_bud;
	PlantAxisClass axis_class;
	union { uint8_t branching_order; uint8_t order; };
	uint8_t        alive;
	bool           is_root_axis;
	PlantUnitId    first_unit;
	PlantUnitId    last_unit;
	uint32_t       unit_count;
	float          birth_day;
	float          age_days;
	float          length_m;
	float          base_radius_m;
	uint64_t       topo_hash;
} PlantAxis;

typedef struct {
	PlantAxis       *axes;
	uint32_t         axis_count;
	uint32_t         axis_capacity;

	PlantGrowthUnit *units;
	uint32_t         unit_count;
	uint32_t         unit_capacity;

	PlantBud        *buds;
	uint32_t         bud_count;
	uint32_t         bud_capacity;

	PlantOrgan      *organs;
	uint32_t         organ_count;
	uint32_t         organ_capacity;

	/* Computed macroscopic bounds of the developed phenotype (in meters) */
	float            height_m;
	float            crown_radius_m;
	float            trunk_base_radius_m;
	float            total_leaf_area_m2;
} PlantPhenotype;

/* Fixed-buffer backing arena so callers can evaluate plants on the stack or static BSS
 * with zero malloc/free overhead in tight rendering or simulation loops. */
#define PLANT_MAX_AXES   192u
#define PLANT_MAX_UNITS  768u
#define PLANT_MAX_BUDS   768u
#define PLANT_MAX_ORGANS 1536u

typedef struct {
	PlantAxis       axes[PLANT_MAX_AXES];
	PlantGrowthUnit units[PLANT_MAX_UNITS];
	PlantBud        buds[PLANT_MAX_BUDS];
	PlantOrgan      organs[PLANT_MAX_ORGANS];
} PlantPhenotypeStorage;

/* ============================================================================
 * 6. Environment, Ephemeral Deltas & Plant Instance
 * ============================================================================ */

typedef struct {
	/* IDEA.md §5 physical environment observables */
	float     photosynthetic_daily_light_mol_m2;
	union { float mean_air_temperature_C; float temperature_c; };
	float     soil_water_potential_MPa;
	union { float soil_nutrient_availability; float soil_nutrients; };
	union { float local_light_availability;   float sunlight;       };
	float     wind_speed_m_s;
	union { PlantVec3 light_direction;        PlantVec3 mean_light_dir; };
	PlantVec3 gravity_direction;

	/* Additional convenient normalized moisture & 3D wind vector */
	float     soil_moisture;      /* [0..1] volumetric root-zone moisture */
	PlantVec3 wind_vector;        /* m/s prevailing wind vector */
} PlantEnvironment;

/* Per-meristem 3D spatial environment sampler callback (IDEA.md §7) */
typedef PlantEnvironment (*PlantEnvironmentSampler)(
	void     *context,
	PlantVec3 position,
	float     time_days
);

/* Ephemeral Delta Decay record: represents a temporary player/world perturbation
 * that self-heals over [t_event_days, t_event_days + t_heal_days] and can be
 * garbage-collected to 0 bytes once age_days >= t_event_days + t_heal_days. */
typedef enum {
	PLANT_DELTA_NONE = 0,
	PLANT_DELTA_PRUNE_AXIS,     /* Sever axis/subtree at target_topo_hash */
	PLANT_DELTA_HARVEST_FRUIT,  /* Strip fruit/cones on subtree rooted at target_topo_hash (0=all) */
	PLANT_DELTA_STRIP_FOLIAGE,  /* Strip leaves on subtree rooted at target_topo_hash (0=all) */
	PLANT_DELTA_FELL_PLANT      /* Cut entire above-ground plant at basal collar */
} PlantDeltaKind;

typedef struct {
	uint64_t       target_topo_hash; /* 0 = whole plant, or specific PlantGrowthUnit/Axis topo_hash */
	PlantDeltaKind kind;
	float          t_event_days;     /* plant age (in days) when perturbation occurred */
	float          t_heal_days;      /* duration (in days) until 100% healed & GC-eligible */
} PlantDeltaRecord;

typedef struct {
	uint64_t       instance_id;
	PlantGenome    genome;
	PlantPhenotype phenotype;
	float          age_days;
	float          accumulated_degree_days;
	float          carbon_reserves_g;
	float          water_status;
	uint64_t       simulation_seed;
	float          biomass_kg;
	float          carbohydrate_reserve;
	float          water_potential;
	float          stress_index;
	bool           owns_heap_storage;
	/* Growth-time 3D Beer-Lambert light competition voxel grid (12^3) */
	float          light_vox_lai[12][12][12];
	float          light_vox_val[12][12][12];
	float          light_vox_bounds_r;
	float          light_vox_bounds_h;
	bool           light_vox_valid;
} PlantInstance;

/* ============================================================================
 * 7. Topological Coordinate Hashing (SplitMix64) & Math Helpers
 * ============================================================================ */

#define PLANT_TOPO_ROOT_SALT    0x504C414E545F3030ULL
#define PLANT_TOPO_APICAL_SALT  0x41504943414C5F31ULL
#define PLANT_TOPO_LATERAL_SALT 0x4C41544552414C32ULL
#define PLANT_TOPO_ORGAN_SALT   0x4F5247414E5F5F33ULL

enum {
	PLANT_CH_INTERNODE_LEN = 1,
	PLANT_CH_BRANCH_ANGLE  = 2,
	PLANT_CH_ROLL_VAR      = 3,
	PLANT_CH_PHYLLO_JITTER = 4,
	PLANT_CH_BUD_BREAK     = 5,
	PLANT_CH_FLOWER_FATE   = 6,
	PLANT_CH_TORTUOSITY_X  = 7,
	PLANT_CH_TORTUOSITY_Y  = 8,
	PLANT_CH_TORTUOSITY_Z  = 9,
	PLANT_CH_LEAF_LEN      = 10,
	PLANT_CH_LEAF_WIDTH    = 11,
	PLANT_CH_PETIOLE_LEN   = 12,
	PLANT_CH_LEAF_CURVE    = 13,
	PLANT_CH_FLOWER_RAD    = 14,
	PLANT_CH_FRUIT_LEN     = 15,
	PLANT_CH_FRUIT_DIAM    = 16,
	PLANT_CH_FORK_TEST     = 17,
	PLANT_CH_MORTALITY     = 18
};

static inline uint64_t
plant_mix64(uint64_t x)
{
	x += 0x9E3779B97F4A7C15ULL;
	x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ULL;
	x = (x ^ (x >> 27)) * 0x94D049BB133111EBULL;
	return x ^ (x >> 31);
}

static inline uint64_t
plant_topo_step_apical(uint64_t parent_hash, uint32_t node_idx)
{
	return plant_mix64(parent_hash ^ PLANT_TOPO_APICAL_SALT ^ (uint64_t)node_idx);
}

static inline uint64_t
plant_topo_step_lateral(uint64_t unit_hash, uint32_t child_order, uint32_t bud_slot)
{
	return plant_mix64(unit_hash ^ PLANT_TOPO_LATERAL_SALT ^
	                   (((uint64_t)child_order) << 16) ^ (uint64_t)bud_slot);
}

static inline uint64_t
plant_topo_step_organ(uint64_t unit_hash, uint32_t organ_kind, uint32_t sub_idx)
{
	return plant_mix64(unit_hash ^ PLANT_TOPO_ORGAN_SALT ^
	                   (((uint64_t)organ_kind) << 16) ^ (uint64_t)sub_idx);
}

/* Uniform [0, 1] float from (topo_hash, channel) */
static inline float
plant_hash_u01(uint64_t topo_hash, uint32_t channel)
{
	uint64_t h = plant_mix64(topo_hash ^ ((uint64_t)channel * 0xD1B54A32D192ED03ULL));
	return (float)((h >> 40) & 0xFFFFFFULL) / 16777215.0f;
}

/* Approximate standard normal N(0, 1) via Irwin-Hall sum of 4 independent uniform draws */
static inline float
plant_hash_normal01(uint64_t topo_hash, uint32_t channel)
{
	uint64_t h = plant_mix64(topo_hash ^ ((uint64_t)channel * 0x94D049BB133111EBULL));
	float u0 = (float)((h >>  0) & 0xFFFFULL) / 65535.0f;
	float u1 = (float)((h >> 16) & 0xFFFFULL) / 65535.0f;
	float u2 = (float)((h >> 32) & 0xFFFFULL) / 65535.0f;
	float u3 = (float)((h >> 48) & 0xFFFFULL) / 65535.0f;
	/* Sum of 4 U(0,1) has mean 2.0, variance 4/12 = 1/3 -> multiply by sqrt(3) = 1.7320508f */
	return ((u0 + u1 + u2 + u3) - 2.0f) * 1.7320508f;
}

static inline float
plant_clampf(float v, float lo, float hi)
{
	return (v < lo) ? lo : ((v > hi) ? hi : v);
}

static inline float
plant_sample_distribution(const PlantDistribution *d, uint64_t topo_hash, uint32_t channel)
{
	if (!d)
		return 0.0f;
	float z = plant_hash_normal01(topo_hash, channel);
	float v = d->mean + z * d->sigma;
	return plant_clampf(v, d->min, d->max);
}

static inline PlantDistribution
plant_sample_prior(const PlantTraitPrior *p, uint64_t genetic_seed, uint32_t channel)
{
	PlantDistribution d = { 0.0f, 0.0f, 0.0f, 0.0f };
	if (!p)
		return d;
	float z = plant_hash_normal01(genetic_seed, channel);
	d.mean  = plant_clampf(p->mean_prior + z * p->mean_sigma, p->hard_min, p->hard_max);
	d.sigma = p->intra_sigma;
	d.min   = p->hard_min;
	d.max   = p->hard_max;
	return d;
}

static inline PlantVec3
plant_v3(float x, float y, float z)
{
	PlantVec3 v = { x, y, z };
	return v;
}

static inline PlantVec3
plant_v3_add(PlantVec3 a, PlantVec3 b)
{
	return plant_v3(a.x + b.x, a.y + b.y, a.z + b.z);
}

static inline PlantVec3
plant_v3_sub(PlantVec3 a, PlantVec3 b)
{
	return plant_v3(a.x - b.x, a.y - b.y, a.z - b.z);
}

static inline PlantVec3
plant_v3_scale(PlantVec3 a, float s)
{
	return plant_v3(a.x * s, a.y * s, a.z * s);
}

static inline float
plant_v3_dot(PlantVec3 a, PlantVec3 b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

static inline PlantVec3
plant_v3_cross(PlantVec3 a, PlantVec3 b)
{
	return plant_v3(
		a.y * b.z - a.z * b.y,
		a.z * b.x - a.x * b.z,
		a.x * b.y - a.y * b.x);
}

static inline PlantVec3
plant_v3_norm(PlantVec3 v)
{
	float l2 = plant_v3_dot(v, v);
	if (l2 > 1e-12f) {
		float inv = 1.0f / sqrtf(l2);
		return plant_v3_scale(v, inv);
	}
	return plant_v3(0.0f, 0.0f, 1.0f);
}

/* Rotate vector v around unit axis k by angle_rad (Rodrigues' rotation formula) */
static inline PlantVec3
plant_v3_rotate_axis(PlantVec3 v, PlantVec3 k, float angle_rad)
{
	float c = cosf(angle_rad);
	float s = sinf(angle_rad);
	PlantVec3 kxv = plant_v3_cross(k, v);
	float kdv = plant_v3_dot(k, v);
	return plant_v3(
		v.x * c + kxv.x * s + k.x * kdv * (1.0f - c),
		v.y * c + kxv.y * s + k.y * kdv * (1.0f - c),
		v.z * c + kxv.z * s + k.z * kdv * (1.0f - c));
}

/* Build an orthonormal basis (right, forward_perp) perpendicular to unit tangent T */
static inline void
plant_v3_ortho_basis(PlantVec3 T, PlantVec3 *out_U, PlantVec3 *out_V)
{
	PlantVec3 ref = (fabsf(T.z) < 0.92f) ? plant_v3(0.0f, 0.0f, 1.0f) : plant_v3(1.0f, 0.0f, 0.0f);
	PlantVec3 U = plant_v3_norm(plant_v3_cross(T, ref));
	PlantVec3 V = plant_v3_norm(plant_v3_cross(T, U));
	if (out_U) *out_U = U;
	if (out_V) *out_V = V;
}

static inline PlantQuat
plant_quat_from_tangent(PlantVec3 T, float roll_rad)
{
	PlantVec3 z = plant_v3(0.0f, 0.0f, 1.0f);
	float d = plant_v3_dot(z, T);
	PlantQuat q;
	if (d > 0.9999f) {
		q.x = 0.0f; q.y = 0.0f; q.z = sinf(0.5f * roll_rad); q.w = cosf(0.5f * roll_rad);
		return q;
	}
	if (d < -0.9999f) {
		q.x = 1.0f; q.y = 0.0f; q.z = 0.0f; q.w = 0.0f;
		return q;
	}
	PlantVec3 axis = plant_v3_norm(plant_v3_cross(z, T));
	float angle = acosf(plant_clampf(d, -1.0f, 1.0f));
	float s = sinf(0.5f * angle);
	q.x = axis.x * s;
	q.y = axis.y * s;
	q.z = axis.z * s;
	q.w = cosf(0.5f * angle);
	return q;
}

static inline PlantColor4
plant_rgba_u8(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	PlantColor4 c = { (float)r / 255.0f, (float)g / 255.0f, (float)b / 255.0f, (float)a / 255.0f };
	return c;
}

/* ============================================================================
 * 8. Validation, Cholesky Correlated Sampling & Quantitative Inheritance
 * ============================================================================ */

static inline bool
plant_prior_valid(const PlantTraitPrior *p)
{
	if (!p) return false;
	if (!isfinite(p->hard_min) || !isfinite(p->hard_max) || p->hard_min > p->hard_max)
		return false;
	if (!isfinite(p->mean_prior) || p->mean_prior < p->hard_min || p->mean_prior > p->hard_max)
		return false;
	if (!isfinite(p->mean_sigma) || !isfinite(p->intra_sigma) ||
	    p->mean_sigma < 0.0f || p->intra_sigma < 0.0f)
		return false;
	if (p->heritability < 0.0f || p->heritability > 1.0f || !isfinite(p->heritability))
		return false;
	if (p->mutation_sigma < 0.0f || !isfinite(p->mutation_sigma))
		return false;
	return true;
}

static inline bool
plant_dist_valid(const PlantDistribution *d)
{
	if (!d) return false;
	if (!isfinite(d->min) || !isfinite(d->max) || d->min > d->max)
		return false;
	if (!isfinite(d->mean) || d->mean < d->min || d->mean > d->max)
		return false;
	if (!isfinite(d->sigma) || d->sigma < 0.0f)
		return false;
	return true;
}

/* Compute lower-triangular Cholesky factor L (n x n) of symmetric positive-semidefinite
 * correlation matrix A (A = L * L^T). Also validates symmetry and unit diagonal (A_ii == 1). */
#define PLANT_CHOLESKY_MAX_DIM 16u

static inline bool
plant_correlation_cholesky(const float *corr, uint16_t n, float *out_L)
{
	if (!corr || n == 0u || n > PLANT_CHOLESKY_MAX_DIM)
		return false;
	float L[PLANT_CHOLESKY_MAX_DIM * PLANT_CHOLESKY_MAX_DIM];
	memset(L, 0, sizeof(L));

	for (uint16_t i = 0u; i < n; i++) {
		/* Diagonal of a correlation matrix must be 1.0 */
		if (!isfinite(corr[i * n + i]) || fabsf(corr[i * n + i] - 1.0f) > 1e-4f)
			return false;
		for (uint16_t j = 0u; j <= i; j++) {
			float a_ij = corr[i * n + j];
			float a_ji = corr[j * n + i];
			if (!isfinite(a_ij) || !isfinite(a_ji) || fabsf(a_ij - a_ji) > 1e-4f)
				return false;
			if (i != j && (a_ij < -1.0f || a_ij > 1.0f))
				return false;

			float sum = a_ij;
			for (uint16_t k = 0u; k < j; k++)
				sum -= L[i * n + k] * L[j * n + k];

			if (i == j) {
				if (sum < -1e-5f)
					return false; /* Not positive semidefinite! */
				L[i * n + i] = (sum > 0.0f) ? sqrtf(sum) : 0.0f;
			} else {
				L[i * n + j] = (L[j * n + j] > 1e-6f) ? (sum / L[j * n + j]) : 0.0f;
			}
		}
	}
	if (out_L)
		memcpy(out_L, L, (size_t)n * (size_t)n * sizeof(float));
	return true;
}

static inline bool
plant_algorithm_supported(PlantAlgorithm alg, uint16_t expected_id)
{
	if (alg.algorithm_id == 0u && alg.algorithm_version == 0u)
		return true; /* default unset -> V1 */
	return (alg.algorithm_id == expected_id && alg.algorithm_version == 1u);
}

/* Synchronize IDEA.md §2.10 `genome->traits` with convenience sub-genomes */
static inline void
plant_genome_sync_traits(PlantGenome *g)
{
	if (!g) return;
	if (g->schema_version == 0u)
		g->schema_version = PLANT_GENOME_SCHEMA_VERSION;
	const float deg2rad = 3.1415926535f / 180.0f;

	g->traits.life_form = (g->physiology.mature_height_m >= 1.8f) ? PLANT_TREE : PLANT_HERB;
	switch (g->architecture.model) {
	case PLANT_ARCH_LEEUWENBERG:
	case PLANT_ARCH_CHAMBERLAIN:
		g->traits.architecture = PLANT_SYMPODIAL;
		break;
	case PLANT_ARCH_SCHOUTE:
		g->traits.architecture = PLANT_DICHOTOMOUS;
		break;
	case PLANT_ARCH_HOLTTUM:
		g->traits.architecture = PLANT_ROSETTE;
		break;
	default:
		g->traits.architecture = PLANT_MONOPODIAL;
		break;
	}

	PlantDistribution ang_rad = {
		g->architecture.branch_angle_deg.mean * deg2rad,
		g->architecture.branch_angle_deg.sigma * deg2rad,
		g->architecture.branch_angle_deg.min * deg2rad,
		g->architecture.branch_angle_deg.max * deg2rad
	};
	float tau = fmaxf(1.0f, g->meristem.plastochron_days);

	/* Primary shoot axis */
	PlantAxisGenome *ax_prim = &g->traits.axes[PLANT_AXIS_PRIMARY];
	ax_prim->orientation = (g->architecture.orthotropy >= 0.15f) ? PLANT_ORTHOTROPIC : PLANT_PLAGIOTROPIC;
	ax_prim->determinate = (g->architecture.model == PLANT_ARCH_HOLTTUM || g->architecture.model == PLANT_ARCH_CORNER) ? 1u : 0u;
	ax_prim->max_branch_order = g->architecture.max_branch_order;
	ax_prim->extension_rate_m_per_day = g->architecture.internode_length_m.mean / tau;
	ax_prim->maximum_length_m = g->physiology.mature_height_m;
	ax_prim->internode_length_m = g->architecture.internode_length_m;
	ax_prim->branch_angle_rad = ang_rad;
	ax_prim->lateral_bud_probability = g->meristem.lateral_bud_probability;
	ax_prim->apical_dominance = g->architecture.apical_dominance;
	ax_prim->branch_length_ratio = g->meristem.vigor_decay_per_order;
	ax_prim->growth_rate_variation = 0.12f;

	/* Lateral shoot axis */
	PlantAxisGenome *ax_lat = &g->traits.axes[PLANT_AXIS_LATERAL];
	*ax_lat = *ax_prim;
	ax_lat->orientation = (g->architecture.orthotropy > 0.65f) ? PLANT_ORTHOTROPIC : PLANT_PLAGIOTROPIC;
	ax_lat->extension_rate_m_per_day *= g->meristem.vigor_decay_per_order;
	ax_lat->maximum_length_m = g->physiology.mature_crown_width_m * 0.6f;

	/* Short shoot & reproductive axes */
	g->traits.axes[PLANT_AXIS_SHORT_SHOOT] = *ax_lat;
	g->traits.axes[PLANT_AXIS_SHORT_SHOOT].determinate = 1u;
	g->traits.axes[PLANT_AXIS_SHORT_SHOOT].maximum_length_m *= 0.25f;

	g->traits.axes[PLANT_AXIS_REPRODUCTIVE] = *ax_lat;
	g->traits.axes[PLANT_AXIS_REPRODUCTIVE].determinate = 1u;
	g->traits.axes[PLANT_AXIS_REPRODUCTIVE].maximum_length_m = fmaxf(0.05f, g->flower.pedicel_length_m.mean * 3.0f);

	/* Root axes */
	if (g->roots.maximum_depth_m <= 0.0f) {
		g->roots.architecture = (g->architecture.basal_stem_count > 3u) ? PLANT_ROOT_FIBROUS : PLANT_ROOT_TAPROOT;
		g->roots.maximum_depth_m = fmaxf(0.15f, g->physiology.mature_height_m * 0.45f);
		g->roots.maximum_spread_m = fmaxf(0.20f, g->physiology.mature_crown_width_m * 0.85f);
		g->roots.branching_probability = 0.55f;
		g->roots.branching_angle_rad = 0.95f;
		g->roots.diameter_taper_exponent = 1.4f;
		g->roots.gravitropism = 0.82f;
		g->roots.hydrotropism = 0.65f;
	}
	g->traits.axes[PLANT_AXIS_TAPROOT] = *ax_prim;
	g->traits.axes[PLANT_AXIS_TAPROOT].orientation = PLANT_ORTHOTROPIC;
	g->traits.axes[PLANT_AXIS_TAPROOT].maximum_length_m = g->roots.maximum_depth_m;
	g->traits.axes[PLANT_AXIS_LATERAL_ROOT] = *ax_lat;
	g->traits.axes[PLANT_AXIS_LATERAL_ROOT].orientation = PLANT_PLAGIOTROPIC;
	g->traits.axes[PLANT_AXIS_LATERAL_ROOT].maximum_length_m = g->roots.maximum_spread_m;

	/* Phyllotaxis */
	g->phyllotaxis.divergence_angle_rad = g->phyllotaxis.divergence_angle_deg * deg2rad;
	g->phyllotaxis.angular_variation_rad = g->phyllotaxis.divergence_jitter_deg * deg2rad;
	g->phyllotaxis.whorl_rotation_rad = g->phyllotaxis.decussate_phase_flip ? (1.5707963f) : 0.0f;
	g->traits.branch_phyllotaxis = g->phyllotaxis;
	g->traits.leaf_phyllotaxis = g->phyllotaxis;
	if (g->leaf.leaves_per_node > 0u)
		g->traits.leaf_phyllotaxis.organs_per_node = g->leaf.leaves_per_node;

	/* Leaf, root, allometry, phenology, plasticity */
	g->leaf.thickness_m = fmaxf(0.0001f, g->leaf.thickness_mm.mean * 0.001f);
	if (g->leaf.base_angle_ratio <= 0.0f) g->leaf.base_angle_ratio = 0.45f;
	if (g->leaf.apex_ratio <= 0.0f)       g->leaf.apex_ratio = 0.35f;
	g->traits.leaves = g->leaf;
	g->traits.roots  = g->roots;

	if (g->allometry.branch_constraint_weight <= 0.0f)
		g->allometry.branch_constraint_weight = 0.85f;
	if (g->allometry.stem_taper_exponent <= 0.0f)
		g->allometry.stem_taper_exponent = 0.80f;
	if (g->allometry.branch_length_exponent <= 0.0f)
		g->allometry.branch_length_exponent = 0.74f;
	if (g->allometry.branch_length_coefficient <= 0.0f)
		g->allometry.branch_length_coefficient = 0.68f;
	if (g->physiology.mature_height_m > 1e-3f)
		g->allometry.crown_width_height_ratio = g->physiology.mature_crown_width_m / g->physiology.mature_height_m;
	g->traits.allometry = g->allometry;

	if (g->phenology.budburst_degree_days <= 0.0f) {
		g->phenology.persistence = (g->leaf.complexity == PLANT_LEAF_NEEDLE || g->leaf.complexity == PLANT_LEAF_SCALE)
			? PLANT_EVERGREEN : PLANT_DECIDUOUS;
		g->phenology.base_temperature_C = 5.0f;
		g->phenology.budburst_degree_days = 120.0f;
		g->phenology.flowering_degree_days = 450.0f;
		g->phenology.senescence_degree_days = 1800.0f;
		g->phenology.dormancy_chilling_hours = 400.0f;
		g->phenology.leaf_fall_daylength_hours = 10.5f;
		g->phenology.minimum_reproductive_age_days = 60.0f;
	}
	g->traits.phenology = g->phenology;
	g->traits.plasticity = g->plasticity;
}

/* Detailed IDEA.md §10 species validation returning PlantStatus */
static inline PlantStatus
plant_species_validate_status(const PlantSpeciesDef *species)
{
	if (!species || !species->scientific_name || !species->scientific_name[0])
		return PLANT_ERR_NULL_PTR;
	if (species->schema_version != 0u && species->schema_version != PLANT_SPECIES_SCHEMA_VERSION)
		return PLANT_ERR_SCHEMA_VERSION;

	if (!plant_prior_valid(&species->internode_length_m) ||
	    !plant_prior_valid(&species->branch_angle_deg) ||
	    !plant_prior_valid(&species->apical_dominance) ||
	    !plant_prior_valid(&species->pipe_model_exponent) ||
	    !plant_prior_valid(&species->leaf_length_m) ||
	    !plant_prior_valid(&species->leaf_width_ratio) ||
	    !plant_prior_valid(&species->flower_radius_m) ||
	    !plant_prior_valid(&species->fruit_length_m) ||
	    !plant_prior_valid(&species->growth_rate) ||
	    !plant_prior_valid(&species->max_age_years))
		return PLANT_ERR_INVALID_DISTRIBUTION;

	for (size_t i = 0; i < species->prior_count; i++) {
		if (!species->priors)
			return PLANT_ERR_NULL_PTR;
		const PlantTraitPrior *tp = &species->priors[i];
		if (!plant_dist_valid(&tp->population_distribution))
			return PLANT_ERR_INVALID_DISTRIBUTION;
		if (tp->heritability < 0.0f || tp->heritability > 1.0f || !isfinite(tp->heritability))
			return PLANT_ERR_INVALID_HERITABILITY;
		if (tp->mutation_sigma < 0.0f || !isfinite(tp->mutation_sigma))
			return PLANT_ERR_INVALID_DISTRIBUTION;
	}

	/* Validate correlation matrices (symmetric, unit diagonal, positive semidefinite) */
	for (size_t c = 0; c < species->correlation_count; c++) {
		if (!species->correlations)
			return PLANT_ERR_NULL_PTR;
		const PlantCorrelationGroup *cg = &species->correlations[c];
		if (cg->trait_count == 0u || !cg->traits || !cg->correlation_matrix)
			return PLANT_ERR_INVALID_CORRELATION_MATRIX;
		if (!plant_correlation_cholesky(cg->correlation_matrix, cg->trait_count, NULL))
			return PLANT_ERR_INVALID_CORRELATION_MATRIX;
	}

	/* Validate development algorithms */
	const PlantDevelopmentProgram *dp = &species->development;
	if (!plant_algorithm_supported(dp->axis_growth,      PLANT_ALG_AXIS_EXTENSION_V1) ||
	    !plant_algorithm_supported(dp->bud_development,  PLANT_ALG_BUD_ACTIVATION_V1) ||
	    !plant_algorithm_supported(dp->phyllotaxis,      PLANT_ALG_PHYLLOTAXIS_V1) ||
	    !plant_algorithm_supported(dp->branching,        PLANT_ALG_BRANCHING_V1) ||
	    !plant_algorithm_supported(dp->tropism,          PLANT_ALG_TROPISM_V1) ||
	    !plant_algorithm_supported(dp->secondary_growth, PLANT_ALG_SECONDARY_GROWTH_V1) ||
	    !plant_algorithm_supported(dp->leaf_development, PLANT_ALG_LEAF_EXPANSION_V1) ||
	    !plant_algorithm_supported(dp->senescence,       PLANT_ALG_SENESCENCE_V1))
		return PLANT_ERR_UNSUPPORTED_ALGORITHM;

	const PlantGenome *g = &species->canonical_genome;
	if (g->architecture.max_branch_order > 6u)
		return PLANT_ERR_INVALID_DIMENSION;
	if (g->phyllotaxis.buds_per_node == 0u || g->phyllotaxis.buds_per_node > 12u)
		return PLANT_ERR_INVALID_DIMENSION;
	if (g->meristem.plastochron_days <= 0.0f)
		return PLANT_ERR_INVALID_DIMENSION;
	if (g->meristem.lateral_bud_probability < 0.0f || g->meristem.lateral_bud_probability > 1.0f ||
	    g->architecture.apical_dominance < 0.0f || g->architecture.apical_dominance > 1.0f)
		return PLANT_ERR_INVALID_PROBABILITY;
	if (g->allometry.pipe_model_exponent < 1.1f || g->allometry.pipe_model_exponent > 3.5f)
		return PLANT_ERR_INVALID_ALLOMETRY;
	if (g->allometry.tip_min_radius_m <= 0.0f || g->leaf.blade_length_m.mean <= 0.0f)
		return PLANT_ERR_INVALID_DIMENSION;
	if (!isfinite(g->phyllotaxis.divergence_angle_deg) ||
	    g->architecture.branch_angle_deg.min < 0.0f ||
	    g->architecture.branch_angle_deg.max > 180.0f)
		return PLANT_ERR_INVALID_ANGLE;
	if (!plant_dist_valid(&g->architecture.internode_length_m) ||
	    !plant_dist_valid(&g->architecture.branch_angle_deg) ||
	    !plant_dist_valid(&g->leaf.blade_length_m))
		return PLANT_ERR_INVALID_DISTRIBUTION;

	return PLANT_OK;
}

static inline bool
plant_species_validate(const PlantSpeciesDef *species)
{
	return plant_species_validate_status(species) == PLANT_OK;
}

/* Validate developed PlantPhenotype graph topology (no dangling IDs, no parent_axis cycles) */
static inline PlantStatus
plant_phenotype_validate(const PlantPhenotype *pheno)
{
	if (!pheno)
		return PLANT_ERR_NULL_PTR;
	for (uint32_t a = 0u; a < pheno->axis_count; a++) {
		const PlantAxis *ax = &pheno->axes[a];
		if (ax->parent_axis != PLANT_NONE && ax->parent_axis >= pheno->axis_count)
			return PLANT_ERR_GRAPH_DANGLING_REF;
		if (ax->origin_bud != PLANT_NONE && ax->origin_bud >= pheno->bud_count)
			return PLANT_ERR_GRAPH_DANGLING_REF;
		if (ax->first_unit != PLANT_NONE && ax->first_unit >= pheno->unit_count)
			return PLANT_ERR_GRAPH_DANGLING_REF;
		if (ax->last_unit != PLANT_NONE && ax->last_unit >= pheno->unit_count)
			return PLANT_ERR_GRAPH_DANGLING_REF;

		/* Cycle detection in parent_axis chain */
		PlantAxisId cur = ax->parent_axis;
		uint32_t hops = 0u;
		while (cur != PLANT_NONE) {
			if (cur >= pheno->axis_count)
				return PLANT_ERR_GRAPH_DANGLING_REF;
			if (cur == a || ++hops > pheno->axis_count)
				return PLANT_ERR_GRAPH_CYCLE;
			cur = pheno->axes[cur].parent_axis;
		}
	}
	for (uint32_t u = 0u; u < pheno->unit_count; u++) {
		const PlantGrowthUnit *un = &pheno->units[u];
		if (un->axis_id >= pheno->axis_count)
			return PLANT_ERR_GRAPH_DANGLING_REF;
		if (un->previous != PLANT_NONE && un->previous >= pheno->unit_count)
			return PLANT_ERR_GRAPH_DANGLING_REF;
		if (un->next != PLANT_NONE && un->next >= pheno->unit_count)
			return PLANT_ERR_GRAPH_DANGLING_REF;
		if (un->length_m < 0.0f || un->radius_m <= 0.0f)
			return PLANT_ERR_INVALID_DIMENSION;
	}
	for (uint32_t b = 0u; b < pheno->bud_count; b++) {
		const PlantBud *bd = &pheno->buds[b];
		if (bd->owner_unit >= pheno->unit_count)
			return PLANT_ERR_GRAPH_DANGLING_REF;
		if (bd->resulting_axis != PLANT_NONE && bd->resulting_axis >= pheno->axis_count)
			return PLANT_ERR_GRAPH_DANGLING_REF;
	}
	for (uint32_t o = 0u; o < pheno->organ_count; o++) {
		const PlantOrgan *org = &pheno->organs[o];
		if (org->owner_unit >= pheno->unit_count)
			return PLANT_ERR_GRAPH_DANGLING_REF;
		if (org->next_on_unit != PLANT_NONE && org->next_on_unit >= pheno->organ_count)
			return PLANT_ERR_GRAPH_DANGLING_REF;
	}
	return PLANT_OK;
}

static inline PlantGenome
plant_genome_sample(const PlantSpeciesDef *species, uint64_t seed)
{
	PlantGenome g;
	if (!species) {
		memset(&g, 0, sizeof(g));
		return g;
	}
	g = species->canonical_genome;
	g.schema_version = PLANT_GENOME_SCHEMA_VERSION;
	g.species_id = species->id;
	g.genetic_seed = seed ? seed : 0xCAFEBABE12345678ULL;
	g.individual_id = plant_mix64(g.genetic_seed ^ ((uint64_t)species->id << 32));
	g.parent_id[0] = 0ULL;
	g.parent_id[1] = 0ULL;

	/* Sample individual quantitative traits from species priors */
	g.architecture.internode_length_m = plant_sample_prior(&species->internode_length_m, g.genetic_seed, 1u);
	g.architecture.branch_angle_deg   = plant_sample_prior(&species->branch_angle_deg,   g.genetic_seed, 2u);
	g.architecture.apical_dominance   = plant_sample_prior(&species->apical_dominance,   g.genetic_seed, 3u).mean;
	g.allometry.pipe_model_exponent   = plant_sample_prior(&species->pipe_model_exponent,g.genetic_seed, 4u).mean;
	g.leaf.blade_length_m             = plant_sample_prior(&species->leaf_length_m,      g.genetic_seed, 5u);
	g.leaf.width_ratio                = plant_sample_prior(&species->leaf_width_ratio,   g.genetic_seed, 6u);
	g.flower.flower_radius_m          = plant_sample_prior(&species->flower_radius_m,    g.genetic_seed, 7u);
	g.fruit.length_m                  = plant_sample_prior(&species->fruit_length_m,     g.genetic_seed, 8u);
	g.physiology.growth_rate          = plant_sample_prior(&species->growth_rate,        g.genetic_seed, 9u).mean;
	g.physiology.max_age_years        = plant_sample_prior(&species->max_age_years,      g.genetic_seed, 10u).mean;

	/* Apply Cholesky-correlated multi-trait sampling across correlation groups (IDEA.md §3.1) */
	for (size_t c = 0; c < species->correlation_count && species->correlations; c++) {
		const PlantCorrelationGroup *cg = &species->correlations[c];
		float L[PLANT_CHOLESKY_MAX_DIM * PLANT_CHOLESKY_MAX_DIM];
		if (cg->trait_count > 0u && cg->trait_count <= PLANT_CHOLESKY_MAX_DIM &&
		    plant_correlation_cholesky(cg->correlation_matrix, cg->trait_count, L)) {
			float z[PLANT_CHOLESKY_MAX_DIM];
			float y[PLANT_CHOLESKY_MAX_DIM];
			for (uint16_t i = 0u; i < cg->trait_count; i++)
				z[i] = plant_hash_normal01(g.genetic_seed, 50u + (uint32_t)c * 16u + i);
			for (uint16_t i = 0u; i < cg->trait_count; i++) {
				float acc = 0.0f;
				for (uint16_t k = 0u; k <= i; k++)
					acc += L[i * cg->trait_count + k] * z[k];
				y[i] = acc;
			}
			for (uint16_t i = 0u; i < cg->trait_count; i++) {
				switch (cg->traits[i]) {
				case PLANT_TRAIT_INTERNODE_LENGTH: {
					const PlantTraitPrior *p = &species->internode_length_m;
					g.architecture.internode_length_m.mean =
						plant_clampf(p->mean_prior + y[i] * p->mean_sigma, p->hard_min, p->hard_max);
					break;
				}
				case PLANT_TRAIT_LEAF_LENGTH: {
					const PlantTraitPrior *p = &species->leaf_length_m;
					g.leaf.blade_length_m.mean =
						plant_clampf(p->mean_prior + y[i] * p->mean_sigma, p->hard_min, p->hard_max);
					break;
				}
				case PLANT_TRAIT_PRIMARY_EXTENSION:
				case PLANT_TRAIT_LATERAL_EXTENSION: {
					const PlantTraitPrior *p = &species->growth_rate;
					g.physiology.growth_rate =
						plant_clampf(p->mean_prior + y[i] * p->mean_sigma, p->hard_min, p->hard_max);
					break;
				}
				case PLANT_TRAIT_BRANCH_ANGLE: {
					const PlantTraitPrior *p = &species->branch_angle_deg;
					g.architecture.branch_angle_deg.mean =
						plant_clampf(p->mean_prior + y[i] * p->mean_sigma, p->hard_min, p->hard_max);
					break;
				}
				case PLANT_TRAIT_BRANCH_EXPONENT: {
					const PlantTraitPrior *p = &species->pipe_model_exponent;
					g.allometry.pipe_model_exponent =
						plant_clampf(p->mean_prior + y[i] * p->mean_sigma, p->hard_min, p->hard_max);
					break;
				}
				default:
					break;
				}
			}
		}
	}

	/* Correlate crown expansion & mature height with vigour */
	float vigor_mod = g.physiology.growth_rate / fmaxf(0.1f, species->growth_rate.mean_prior);
	float h_mod = 0.5f * (1.0f + vigor_mod) + 0.04f * plant_hash_normal01(g.genetic_seed, 11u);
	float w_mod = 0.5f * (1.0f + vigor_mod) + 0.05f * plant_hash_normal01(g.genetic_seed, 12u);
	g.physiology.mature_height_m      *= plant_clampf(h_mod, 0.82f, 1.22f);
	g.physiology.mature_crown_width_m *= plant_clampf(w_mod, 0.82f, 1.22f);
	g.architecture.tortuosity = plant_clampf(
		g.architecture.tortuosity + 0.04f * plant_hash_normal01(g.genetic_seed, 13u),
		0.0f, 1.0f);

	plant_genome_sync_traits(&g);
	return g;
}

/* Canonical IDEA.md §11 4-argument plant_genome_sample_into */
static inline PlantStatus
plant_genome_sample_into(const PlantSpeciesDef *species,
                         uint64_t seed,
                         uint64_t individual_id,
                         PlantGenome *out)
{
	if (!species || !out)
		return PLANT_ERR_NULL_PTR;
	PlantStatus st = plant_species_validate_status(species);
	if (st != PLANT_OK)
		return st;
	*out = plant_genome_sample(species, seed);
	out->individual_id = individual_id;
	return PLANT_OK;
}

static inline PlantDistribution
plant_dist_cross(const PlantDistribution *a, const PlantDistribution *b, uint64_t seed, uint32_t ch)
{
	PlantDistribution out = *a;
	/* Quantitative inheritance (IDEA.md §3): T_child = (T_mother + T_father)/2 + epsilon */
	float mid = 0.5f * (a->mean + b->mean);
	float mut = 0.03f * (a->max - a->min) * plant_hash_normal01(seed, ch + 100u);
	out.min   = (a->min < b->min) ? a->min : b->min;
	out.max   = (a->max > b->max) ? a->max : b->max;
	out.mean  = plant_clampf(mid + mut, out.min, out.max);
	out.sigma = 0.5f * (a->sigma + b->sigma);
	return out;
}

static inline float
plant_float_cross(float a, float b, float lo, float hi, uint64_t seed, uint32_t ch)
{
	float mid = 0.5f * (a + b);
	float mut = 0.025f * (hi - lo) * plant_hash_normal01(seed, ch + 200u);
	return plant_clampf(mid + mut, lo, hi);
}

static inline PlantGenome
plant_genome_cross(const PlantGenome *parent_a, const PlantGenome *parent_b, uint64_t seed)
{
	if (!parent_a && !parent_b) {
		PlantGenome empty;
		memset(&empty, 0, sizeof(empty));
		return empty;
	}
	if (!parent_a) return *parent_b;
	if (!parent_b) return *parent_a;

	PlantGenome child = (plant_hash_u01(seed, 0u) < 0.5f) ? *parent_a : *parent_b;
	child.schema_version = PLANT_GENOME_SCHEMA_VERSION;
	child.genetic_seed = plant_mix64(parent_a->genetic_seed ^ parent_b->genetic_seed ^ seed);
	child.individual_id = plant_mix64(child.genetic_seed ^ 0xC4055ULL);
	child.parent_id[0] = parent_a->individual_id;
	child.parent_id[1] = parent_b->individual_id;

	child.architecture.internode_length_m = plant_dist_cross(
		&parent_a->architecture.internode_length_m, &parent_b->architecture.internode_length_m, child.genetic_seed, 1u);
	child.architecture.branch_angle_deg = plant_dist_cross(
		&parent_a->architecture.branch_angle_deg, &parent_b->architecture.branch_angle_deg, child.genetic_seed, 2u);
	child.architecture.apical_dominance = plant_float_cross(
		parent_a->architecture.apical_dominance, parent_b->architecture.apical_dominance, 0.0f, 1.0f, child.genetic_seed, 3u);
	child.architecture.orthotropy = plant_float_cross(
		parent_a->architecture.orthotropy, parent_b->architecture.orthotropy, -1.0f, 1.0f, child.genetic_seed, 4u);
	child.architecture.tortuosity = plant_float_cross(
		parent_a->architecture.tortuosity, parent_b->architecture.tortuosity, 0.0f, 1.0f, child.genetic_seed, 5u);

	child.allometry.pipe_model_exponent = plant_float_cross(
		parent_a->allometry.pipe_model_exponent, parent_b->allometry.pipe_model_exponent, 1.4f, 3.0f, child.genetic_seed, 6u);
	child.allometry.root_flare_strength = plant_float_cross(
		parent_a->allometry.root_flare_strength, parent_b->allometry.root_flare_strength, 0.0f, 1.5f, child.genetic_seed, 7u);

	child.leaf.blade_length_m = plant_dist_cross(
		&parent_a->leaf.blade_length_m, &parent_b->leaf.blade_length_m, child.genetic_seed, 8u);
	child.leaf.width_ratio = plant_dist_cross(
		&parent_a->leaf.width_ratio, &parent_b->leaf.width_ratio, child.genetic_seed, 9u);
	child.leaf.petiole_length_m = plant_dist_cross(
		&parent_a->leaf.petiole_length_m, &parent_b->leaf.petiole_length_m, child.genetic_seed, 10u);

	child.flower.flower_radius_m = plant_dist_cross(
		&parent_a->flower.flower_radius_m, &parent_b->flower.flower_radius_m, child.genetic_seed, 11u);
	child.fruit.length_m = plant_dist_cross(
		&parent_a->fruit.length_m, &parent_b->fruit.length_m, child.genetic_seed, 12u);
	child.fruit.diameter_m = plant_dist_cross(
		&parent_a->fruit.diameter_m, &parent_b->fruit.diameter_m, child.genetic_seed, 13u);

	child.physiology.growth_rate = plant_float_cross(
		parent_a->physiology.growth_rate, parent_b->physiology.growth_rate, 0.1f, 3.0f, child.genetic_seed, 14u);
	child.physiology.drought_tolerance = plant_float_cross(
		parent_a->physiology.drought_tolerance, parent_b->physiology.drought_tolerance, 0.0f, 1.0f, child.genetic_seed, 15u);
	child.physiology.cold_tolerance_c = plant_float_cross(
		parent_a->physiology.cold_tolerance_c, parent_b->physiology.cold_tolerance_c, -60.0f, 30.0f, child.genetic_seed, 16u);

	plant_genome_sync_traits(&child);
	return child;
}

/* Canonical IDEA.md §11 5-argument plant_genome_cross_into */
static inline PlantStatus
plant_genome_cross_into(const PlantSpeciesDef *species,
                        const PlantGenome *mother,
                        const PlantGenome *father,
                        uint64_t seed,
                        PlantGenome *out)
{
	if (!mother || !father || !out)
		return PLANT_ERR_NULL_PTR;
	if (species) {
		PlantStatus st = plant_species_validate_status(species);
		if (st != PLANT_OK)
			return st;
	}
	*out = plant_genome_cross(mother, father, seed);
	if (species)
		out->species_id = species->id;
	return PLANT_OK;
}

/* ============================================================================
 * 9. Canonical 16-Species Biological Definitions (Matching plants.dsv)
 * ============================================================================ */

static inline PlantEnvironment
plant_environment_default(void)
{
	PlantEnvironment env;
	memset(&env, 0, sizeof(env));
	env.photosynthetic_daily_light_mol_m2 = 35.0f;
	env.sunlight = 0.85f;
	env.soil_moisture = 0.65f;
	env.soil_water_potential_MPa = -0.35f;
	env.temperature_c = 18.0f;
	env.soil_nutrients = 0.75f;
	env.wind_speed_m_s = 0.4272f;
	env.wind_vector = plant_v3(0.4f, 0.15f, 0.0f);
	env.mean_light_dir = plant_v3(0.08f, -0.10f, 0.99f);
	env.gravity_direction = plant_v3(0.0f, 0.0f, -1.0f);
	return env;
}

/* Build biologically accurate PlantSpeciesDef for any of the 16 NeverDark species (0..15) */
static inline void
plant_species_init_preset(PlantSpeciesDef *sp, uint32_t spec_idx)
{
	if (!sp) return;
	memset(sp, 0, sizeof(*sp));
	sp->schema_version = PLANT_SPECIES_SCHEMA_VERSION;
	sp->id = spec_idx;

	/* Default priors & baseline physiological parameters */
	PlantGenome *g = &sp->canonical_genome;
	g->schema_version = PLANT_GENOME_SCHEMA_VERSION;
	g->species_id = spec_idx;
	g->genetic_seed = 0x10000ULL + spec_idx;

	g->phyllotaxis.mode = PLANT_PHYLLO_SPIRAL;
	g->phyllotaxis.divergence_angle_deg = 137.50776f;
	g->phyllotaxis.divergence_jitter_deg = 7.5f;
	g->phyllotaxis.buds_per_node = 1u;
	g->phyllotaxis.decussate_phase_flip = false;

	g->meristem.plastochron_days = 15.0f;
	g->meristem.lateral_bud_probability = 0.68f;
	g->meristem.flower_bud_probability = 0.35f;
	g->meristem.dormancy_release_threshold = 0.25f;
	g->meristem.mortality_probability = 0.02f;
	g->meristem.sylleptic_branch_ratio = 0.25f;
	g->meristem.vigor_decay_per_order = 0.68f;

	g->allometry.pipe_model_exponent = 2.10f;
	g->allometry.basal_radius_ratio = 0.048f;
	g->allometry.tip_min_radius_m = 0.012f;
	g->allometry.radius_taper = 0.015f;
	g->allometry.wood_density = 0.65f;
	g->allometry.stiffness_young_modulus = 0.72f;
	g->allometry.root_flare_strength = 0.38f;
	g->allometry.bark_roughness = 0.55f;
	g->allometry.bark_thickness_ratio = 0.08f;

	g->architecture.crown_flatness = 0.75f;
	g->architecture.branch_shed_threshold = 0.30f;

	g->leaf.complexity = PLANT_LEAF_SIMPLE;
	g->leaf.shape = PLANT_LEAF_SHAPE_ELLIPTIC;
	g->leaf.margin = PLANT_MARGIN_ENTIRE;
	g->leaf.venation_type = PLANT_VENATION_PINNATE;
	g->leaf.blade_length_m = (PlantDistribution){ 0.28f, 0.04f, 0.14f, 0.45f };
	g->leaf.width_ratio = (PlantDistribution){ 0.52f, 0.06f, 0.30f, 0.80f };
	g->leaf.petiole_length_m = (PlantDistribution){ 0.05f, 0.01f, 0.01f, 0.12f };
	g->leaf.thickness_mm = (PlantDistribution){ 0.35f, 0.05f, 0.15f, 0.80f };
	g->leaf.curvature = (PlantDistribution){ 0.22f, 0.05f, -0.30f, 0.75f };
	g->leaf.serration_depth = (PlantDistribution){ 0.05f, 0.01f, 0.0f, 0.25f };
	g->leaf.lobe_depth = (PlantDistribution){ 0.0f, 0.0f, 0.0f, 0.65f };
	g->leaf.leaflet_count = 1u;
	g->leaf.leaves_per_node = 3u;
	g->leaf.chlorophyll_index = 0.80f;
	g->leaf.canopy_porosity = 0.46f;
	g->leaf.retention_order_threshold = 1.0f; /* leaves on terminal & sub-terminal twigs */

	g->flower.has_flowers = false;
	g->flower.inflorescence = PLANT_INFLORESCENCE_SOLITARY;
	g->flower.symmetry = PLANT_FLOWER_ACTINOMORPHIC;
	g->flower.corolla_type = PLANT_COROLLA_FREE_PETALS;
	g->flower.sepal_count = 5u;
	g->flower.petal_count = 5u;
	g->flower.stamen_count = 10u;
	g->flower.carpel_count = 1u;
	g->flower.flower_radius_m = (PlantDistribution){ 0.06f, 0.01f, 0.02f, 0.15f };
	g->flower.pedicel_length_m = (PlantDistribution){ 0.04f, 0.01f, 0.01f, 0.10f };
	g->flower.corolla_depth_m = (PlantDistribution){ 0.03f, 0.01f, 0.01f, 0.08f };
	g->flower.bloom_onset_maturity = 0.45f;

	g->fruit.fruit_type = PLANT_FRUIT_NONE;
	g->fruit.length_m = (PlantDistribution){ 0.08f, 0.015f, 0.02f, 0.20f };
	g->fruit.diameter_m = (PlantDistribution){ 0.05f, 0.010f, 0.01f, 0.16f };
	g->fruit.mass_g = (PlantDistribution){ 15.0f, 3.0f, 1.0f, 150.0f };
	g->fruit.ripening_rate = 0.5f;
	g->fruit.retention_strength = 0.6f;

	g->physiology.light_optimum = 0.80f;
	g->physiology.light_tolerance = 0.45f;
	g->physiology.water_optimum = 0.60f;
	g->physiology.drought_tolerance = 0.50f;
	g->physiology.temperature_optimum_c = 18.0f;
	g->physiology.cold_tolerance_c = -15.0f;
	g->physiology.heat_tolerance_c = 38.0f;
	g->physiology.nutrient_demand = 0.50f;
	g->physiology.wind_tolerance = 0.70f;
	g->physiology.growth_rate = 1.0f;
	g->physiology.maintenance_cost = 0.20f;
	g->physiology.max_age_years = 120.0f;
	g->physiology.mature_height_m = 3.20f;
	g->physiology.mature_crown_width_m = 2.60f;

	g->plasticity.phototropism_strength = 0.32f;
	g->plasticity.gravitropism_strength = 0.45f;
	g->plasticity.shade_avoidance = 0.35f;
	g->plasticity.wind_thigmomorphogenesis = 0.25f;
	g->plasticity.drought_root_shoot_shift = 0.20f;

	switch (spec_idx) {
	case 0: /* Pinus sylvestris (Scots pine) */
		sp->scientific_name = "Pinus sylvestris";
		sp->common_name = "Scots pine";
		sp->slug = "pinus_sylvestris";
		sp->life_form = PLANT_LIFEFORM_TREE;
		sp->architectural_model = PLANT_ARCH_RAUH;
		g->architecture.model = PLANT_ARCH_RAUH;
		g->architecture.max_branch_order = 3u;
		g->architecture.basal_stem_count = 1u;
		g->architecture.internode_length_m = (PlantDistribution){ 0.88f, 0.12f, 0.46f, 1.40f };
		g->architecture.branch_angle_deg = (PlantDistribution){ 68.0f, 7.0f, 45.0f, 88.0f };
		g->architecture.roll_variation_deg = (PlantDistribution){ 12.0f, 4.0f, 0.0f, 30.0f };
		g->architecture.apical_dominance = 0.78f;
		g->architecture.acrotony = 0.88f;
		g->architecture.basitony = 0.02f;
		g->architecture.clear_bole_fraction = 0.52f;
		g->architecture.fork_probability = 0.28f;
		g->architecture.orthotropy = 0.30f;
		g->architecture.tortuosity = 0.38f;
		g->architecture.crown_flatness = 0.46f;
		g->architecture.branch_shed_threshold = 0.68f;
		g->phyllotaxis.mode = PLANT_PHYLLO_WHORLED;
		g->phyllotaxis.buds_per_node = 3u;
		g->phyllotaxis.divergence_angle_deg = 120.0f;
		g->meristem.lateral_bud_probability = 0.86f;
		g->allometry.pipe_model_exponent = 2.08f;
		g->allometry.basal_radius_ratio = 0.044f;
		g->allometry.tip_min_radius_m = 0.026f;
		g->allometry.root_flare_strength = 0.44f;
		g->allometry.bark_class = 1u; /* ND_PLANT_BARK_SCOTS */
		g->allometry.bark_dark  = plant_rgba_u8(54, 34, 24, 255);
		g->allometry.bark_mid   = plant_rgba_u8(124, 68, 42, 255);
		g->allometry.bark_light = plant_rgba_u8(196, 114, 66, 255);
		g->leaf.complexity = PLANT_LEAF_NEEDLE;
		g->leaf.shape = PLANT_LEAF_SHAPE_LINEAR;
		g->leaf.blade_length_m = (PlantDistribution){ 0.72f, 0.09f, 0.42f, 1.05f };
		g->leaf.width_ratio = (PlantDistribution){ 0.45f, 0.06f, 0.28f, 0.65f };
		g->leaf.leaflet_count = 7u;
		g->leaf.leaves_per_node = 4u;
		g->leaf.canopy_porosity = 0.48f;
		g->leaf.adaxial_deep = plant_rgba_u8(16, 40, 28, 255);
		g->leaf.adaxial_mid  = plant_rgba_u8(30, 68, 46, 255);
		g->leaf.adaxial_lit  = plant_rgba_u8(52, 102, 66, 255);
		g->leaf.abaxial_high = plant_rgba_u8(84, 140, 92, 255);
		g->physiology.mature_height_m = 10.00f;
		g->physiology.mature_crown_width_m = 5.78f;
		g->physiology.max_age_years = 250.0f;
		break;

	case 1: /* Pseudotsuga menziesii (Douglas fir) */
		sp->scientific_name = "Pseudotsuga menziesii";
		sp->common_name = "Douglas fir";
		sp->slug = "pseudotsuga_menziesii";
		sp->life_form = PLANT_LIFEFORM_TREE;
		sp->architectural_model = PLANT_ARCH_MASSART;
		g->architecture.model = PLANT_ARCH_MASSART;
		g->architecture.max_branch_order = 3u;
		g->architecture.basal_stem_count = 1u;
		g->architecture.internode_length_m = (PlantDistribution){ 1.02f, 0.12f, 0.56f, 1.50f };
		g->architecture.branch_angle_deg = (PlantDistribution){ 74.0f, 5.0f, 58.0f, 88.0f };
		g->architecture.roll_variation_deg = (PlantDistribution){ 6.0f, 2.0f, 0.0f, 15.0f };
		g->architecture.apical_dominance = 0.96f;
		g->architecture.acrotony = 0.52f;
		g->architecture.basitony = 0.05f;
		g->architecture.clear_bole_fraction = 0.18f;
		g->architecture.fork_probability = 0.01f;
		g->architecture.orthotropy = 0.85f;
		g->architecture.tortuosity = 0.08f;
		g->architecture.crown_flatness = 0.45f;
		g->architecture.branch_shed_threshold = 0.54f;
		g->phyllotaxis.mode = PLANT_PHYLLO_WHORLED;
		g->phyllotaxis.buds_per_node = 4u;
		g->phyllotaxis.divergence_angle_deg = 90.0f;
		g->meristem.lateral_bud_probability = 0.92f;
		g->allometry.pipe_model_exponent = 2.18f;
		g->allometry.basal_radius_ratio = 0.052f;
		g->allometry.tip_min_radius_m = 0.025f;
		g->allometry.root_flare_strength = 0.50f;
		g->allometry.bark_class = 0u;
		g->allometry.bark_dark  = plant_rgba_u8(48, 30, 20, 255);
		g->allometry.bark_mid   = plant_rgba_u8(78, 50, 34, 255);
		g->allometry.bark_light = plant_rgba_u8(112, 74, 52, 255);
		g->leaf.complexity = PLANT_LEAF_NEEDLE;
		g->leaf.shape = PLANT_LEAF_SHAPE_LINEAR;
		g->leaf.blade_length_m = (PlantDistribution){ 0.78f, 0.09f, 0.44f, 1.12f };
		g->leaf.width_ratio = (PlantDistribution){ 0.42f, 0.05f, 0.25f, 0.60f };
		g->leaf.leaflet_count = 8u;
		g->leaf.leaves_per_node = 4u;
		g->leaf.canopy_porosity = 0.52f;
		g->leaf.adaxial_deep = plant_rgba_u8(12, 36, 24, 255);
		g->leaf.adaxial_mid  = plant_rgba_u8(24, 62, 40, 255);
		g->leaf.adaxial_lit  = plant_rgba_u8(44, 94, 58, 255);
		g->leaf.abaxial_high = plant_rgba_u8(74, 134, 84, 255);
		g->physiology.mature_height_m = 14.40f;
		g->physiology.mature_crown_width_m = 6.89f;
		g->physiology.max_age_years = 400.0f;
		break;

	case 2: /* Betula pendula (Silver birch) */
		sp->scientific_name = "Betula pendula";
		sp->common_name = "Silver birch";
		sp->slug = "betula_pendula";
		sp->life_form = PLANT_LIFEFORM_TREE;
		sp->architectural_model = PLANT_ARCH_TROLL;
		g->architecture.model = PLANT_ARCH_TROLL;
		g->architecture.max_branch_order = 3u;
		g->architecture.basal_stem_count = 2u;
		g->architecture.internode_length_m = (PlantDistribution){ 0.76f, 0.10f, 0.40f, 1.18f };
		g->architecture.branch_angle_deg = (PlantDistribution){ 46.0f, 7.0f, 28.0f, 68.0f };
		g->architecture.apical_dominance = 0.68f;
		g->architecture.acrotony = 0.68f;
		g->architecture.clear_bole_fraction = 0.34f;
		g->architecture.orthotropy = -0.35f; /* high arching then weeping pendulous tips */
		g->architecture.tortuosity = 0.26f;
		g->architecture.crown_flatness = 0.78f;
		g->phyllotaxis.mode = PLANT_PHYLLO_SPIRAL;
		g->phyllotaxis.buds_per_node = 3u;
		g->meristem.lateral_bud_probability = 0.86f;
		g->allometry.pipe_model_exponent = 2.15f;
		g->allometry.basal_radius_ratio = 0.034f;
		g->allometry.tip_min_radius_m = 0.017f;
		g->allometry.stiffness_young_modulus = 0.38f;
		g->allometry.bark_class = 2u; /* ND_PLANT_BARK_BIRCH */
		g->allometry.bark_dark  = plant_rgba_u8(42, 38, 36, 255);
		g->allometry.bark_mid   = plant_rgba_u8(184, 178, 170, 255);
		g->allometry.bark_light = plant_rgba_u8(238, 234, 226, 255);
		g->leaf.complexity = PLANT_LEAF_SIMPLE;
		g->leaf.shape = PLANT_LEAF_SHAPE_OVATE;
		g->leaf.margin = PLANT_MARGIN_SERRATE;
		g->leaf.blade_length_m = (PlantDistribution){ 0.58f, 0.08f, 0.32f, 0.86f };
		g->leaf.width_ratio = (PlantDistribution){ 0.65f, 0.06f, 0.45f, 0.85f };
		g->leaf.curvature = (PlantDistribution){ 0.48f, 0.08f, 0.20f, 0.85f };
		g->leaf.leaves_per_node = 4u;
		g->leaf.canopy_porosity = 0.58f;
		g->leaf.adaxial_deep = plant_rgba_u8(46, 72, 20, 255);
		g->leaf.adaxial_mid  = plant_rgba_u8(86, 124, 34, 255);
		g->leaf.adaxial_lit  = plant_rgba_u8(138, 174, 52, 255);
		g->leaf.abaxial_high = plant_rgba_u8(192, 218, 78, 255);
		g->physiology.mature_height_m = 8.80f;
		g->physiology.mature_crown_width_m = 5.11f;
		break;

	case 3: /* Linum usitatissimum (Common flax) */
		sp->scientific_name = "Linum usitatissimum";
		sp->common_name = "Common flax";
		sp->slug = "linum_usitatissimum";
		sp->life_form = PLANT_LIFEFORM_HERB;
		sp->architectural_model = PLANT_ARCH_MCCLURE;
		g->architecture.model = PLANT_ARCH_MCCLURE;
		g->architecture.max_branch_order = 2u;
		g->architecture.basal_stem_count = 5u;
		g->architecture.internode_length_m = (PlantDistribution){ 0.14f, 0.02f, 0.08f, 0.22f };
		g->architecture.branch_angle_deg = (PlantDistribution){ 32.0f, 5.0f, 18.0f, 48.0f };
		g->architecture.apical_dominance = 0.82f;
		g->architecture.acrotony = 0.82f;
		g->architecture.basitony = 0.65f;
		g->architecture.clear_bole_fraction = 0.15f;
		g->architecture.orthotropy = 0.78f;
		g->architecture.tortuosity = 0.10f;
		g->architecture.crown_flatness = 0.85f;
		g->allometry.basal_radius_ratio = 0.024f;
		g->allometry.tip_min_radius_m = 0.005f;
		g->allometry.root_flare_strength = 0.10f;
		g->allometry.bark_class = 5u; /* ND_PLANT_BARK_HERB */
		g->allometry.bark_dark  = plant_rgba_u8(42, 72, 32, 255);
		g->allometry.bark_mid   = plant_rgba_u8(64, 106, 48, 255);
		g->allometry.bark_light = plant_rgba_u8(92, 142, 68, 255);
		g->leaf.complexity = PLANT_LEAF_SIMPLE;
		g->leaf.shape = PLANT_LEAF_SHAPE_LANCEOLATE;
		g->leaf.blade_length_m = (PlantDistribution){ 0.14f, 0.02f, 0.08f, 0.22f };
		g->leaf.width_ratio = (PlantDistribution){ 0.28f, 0.04f, 0.18f, 0.42f };
		g->leaf.leaves_per_node = 3u;
		g->leaf.canopy_porosity = 0.45f;
		g->leaf.retention_order_threshold = 0.0f;
		g->leaf.adaxial_deep = plant_rgba_u8(36, 74, 32, 255);
		g->leaf.adaxial_mid  = plant_rgba_u8(58, 110, 48, 255);
		g->leaf.adaxial_lit  = plant_rgba_u8(86, 148, 68, 255);
		g->leaf.abaxial_high = plant_rgba_u8(118, 182, 92, 255);
		g->flower.has_flowers = true;
		g->flower.inflorescence = PLANT_INFLORESCENCE_PANICLE;
		g->flower.corolla_type = PLANT_COROLLA_FREE_PETALS;
		g->flower.petal_count = 5u;
		g->flower.flower_radius_m = (PlantDistribution){ 0.055f, 0.008f, 0.03f, 0.085f };
		g->flower.primary_color   = plant_rgba_u8(132, 182, 248, 255);
		g->flower.secondary_color = plant_rgba_u8(64, 108, 196, 255);
		g->physiology.mature_height_m = 0.76f;
		g->physiology.mature_crown_width_m = 0.54f;
		g->physiology.max_age_years = 2.0f;
		break;

	case 4: /* Betula pubescens (Downy birch) */
		sp->scientific_name = "Betula pubescens";
		sp->common_name = "Downy birch";
		sp->slug = "betula_pubescens";
		sp->life_form = PLANT_LIFEFORM_TREE;
		sp->architectural_model = PLANT_ARCH_RAUH;
		g->architecture.model = PLANT_ARCH_RAUH;
		g->architecture.max_branch_order = 3u;
		g->architecture.basal_stem_count = 3u;
		g->architecture.internode_length_m = (PlantDistribution){ 0.68f, 0.09f, 0.36f, 1.04f };
		g->architecture.branch_angle_deg = (PlantDistribution){ 52.0f, 6.0f, 34.0f, 72.0f };
		g->architecture.apical_dominance = 0.60f;
		g->architecture.acrotony = 0.65f;
		g->architecture.clear_bole_fraction = 0.26f;
		g->architecture.orthotropy = 0.28f;
		g->architecture.tortuosity = 0.32f;
		g->architecture.crown_flatness = 0.84f;
		g->phyllotaxis.buds_per_node = 3u;
		g->meristem.lateral_bud_probability = 0.84f;
		g->allometry.basal_radius_ratio = 0.036f;
		g->allometry.tip_min_radius_m = 0.019f;
		g->allometry.bark_class = 2u;
		g->allometry.bark_dark  = plant_rgba_u8(54, 48, 44, 255);
		g->allometry.bark_mid   = plant_rgba_u8(154, 148, 140, 255);
		g->allometry.bark_light = plant_rgba_u8(208, 202, 194, 255);
		g->leaf.complexity = PLANT_LEAF_SIMPLE;
		g->leaf.shape = PLANT_LEAF_SHAPE_OVATE;
		g->leaf.margin = PLANT_MARGIN_SERRATE;
		g->leaf.blade_length_m = (PlantDistribution){ 0.56f, 0.07f, 0.30f, 0.82f };
		g->leaf.width_ratio = (PlantDistribution){ 0.72f, 0.06f, 0.50f, 0.90f };
		g->leaf.leaves_per_node = 4u;
		g->leaf.canopy_porosity = 0.50f;
		g->leaf.adaxial_deep = plant_rgba_u8(28, 58, 26, 255);
		g->leaf.adaxial_mid  = plant_rgba_u8(52, 96, 42, 255);
		g->leaf.adaxial_lit  = plant_rgba_u8(86, 138, 62, 255);
		g->leaf.abaxial_high = plant_rgba_u8(126, 178, 88, 255);
		g->physiology.mature_height_m = 7.20f;
		g->physiology.mature_crown_width_m = 4.67f;
		break;

	case 5: /* Abies alba (European silver fir) */
		sp->scientific_name = "Abies alba";
		sp->common_name = "European silver fir";
		sp->slug = "abies_alba";
		sp->life_form = PLANT_LIFEFORM_TREE;
		sp->architectural_model = PLANT_ARCH_MASSART;
		g->architecture.model = PLANT_ARCH_MASSART;
		g->architecture.max_branch_order = 3u;
		g->architecture.basal_stem_count = 1u;
		g->architecture.internode_length_m = (PlantDistribution){ 0.94f, 0.11f, 0.52f, 1.38f };
		g->architecture.branch_angle_deg = (PlantDistribution){ 78.0f, 4.0f, 64.0f, 89.0f };
		g->architecture.apical_dominance = 0.98f;
		g->architecture.acrotony = 0.48f;
		g->architecture.clear_bole_fraction = 0.14f;
		g->architecture.orthotropy = 0.90f;
		g->architecture.tortuosity = 0.05f;
		g->architecture.crown_flatness = 0.42f;
		g->architecture.branch_shed_threshold = 0.52f;
		g->phyllotaxis.mode = PLANT_PHYLLO_WHORLED;
		g->phyllotaxis.buds_per_node = 4u;
		g->phyllotaxis.divergence_angle_deg = 90.0f;
		g->meristem.lateral_bud_probability = 0.94f;
		g->allometry.pipe_model_exponent = 2.20f;
		g->allometry.basal_radius_ratio = 0.054f;
		g->allometry.tip_min_radius_m = 0.025f;
		g->allometry.root_flare_strength = 0.48f;
		g->allometry.bark_class = 0u;
		g->allometry.bark_dark  = plant_rgba_u8(52, 50, 48, 255);
		g->allometry.bark_mid   = plant_rgba_u8(88, 86, 82, 255);
		g->allometry.bark_light = plant_rgba_u8(128, 126, 122, 255);
		g->leaf.complexity = PLANT_LEAF_NEEDLE;
		g->leaf.shape = PLANT_LEAF_SHAPE_LINEAR;
		g->leaf.blade_length_m = (PlantDistribution){ 0.72f, 0.09f, 0.42f, 1.04f };
		g->leaf.width_ratio = (PlantDistribution){ 0.44f, 0.05f, 0.28f, 0.62f };
		g->leaf.leaflet_count = 8u;
		g->leaf.leaves_per_node = 4u;
		g->leaf.canopy_porosity = 0.48f;
		g->leaf.adaxial_deep = plant_rgba_u8(14, 38, 32, 255);
		g->leaf.adaxial_mid  = plant_rgba_u8(28, 64, 52, 255);
		g->leaf.adaxial_lit  = plant_rgba_u8(52, 98, 78, 255);
		g->leaf.abaxial_high = plant_rgba_u8(112, 158, 138, 255);
		g->fruit.fruit_type = PLANT_FRUIT_CONE;
		g->fruit.length_m = (PlantDistribution){ 0.22f, 0.03f, 0.14f, 0.34f };
		g->fruit.diameter_m = (PlantDistribution){ 0.085f, 0.015f, 0.05f, 0.14f };
		g->fruit.immature_color = plant_rgba_u8(92, 62, 38, 255);
		g->fruit.mature_color   = plant_rgba_u8(148, 106, 68, 255);
		g->physiology.mature_height_m = 12.80f;
		g->physiology.mature_crown_width_m = 6.40f;
		break;

	case 6: /* Arthrocereus rondonianus (Columnar cactus) */
		sp->scientific_name = "Arthrocereus rondonianus";
		sp->common_name = "Rondonia columnar cactus";
		sp->slug = "arthrocereus_rondonianus";
		sp->life_form = PLANT_LIFEFORM_SUCCULENT;
		sp->architectural_model = PLANT_ARCH_CORNER;
		g->architecture.model = PLANT_ARCH_CORNER;
		g->architecture.max_branch_order = 1u;
		g->architecture.basal_stem_count = 1u;
		g->architecture.internode_length_m = (PlantDistribution){ 0.28f, 0.03f, 0.18f, 0.42f };
		g->architecture.branch_angle_deg = (PlantDistribution){ 68.0f, 5.0f, 54.0f, 82.0f };
		g->architecture.apical_dominance = 0.88f;
		g->architecture.acrotony = 0.40f;
		g->architecture.basitony = 0.60f;
		g->architecture.clear_bole_fraction = 0.30f;
		g->architecture.orthotropy = 0.96f;
		g->architecture.tortuosity = 0.03f;
		g->architecture.crown_flatness = 1.35f;
		g->phyllotaxis.mode = PLANT_PHYLLO_WHORLED;
		g->phyllotaxis.buds_per_node = 2u;
		g->phyllotaxis.divergence_angle_deg = 137.5f;
		g->meristem.lateral_bud_probability = 0.82f;
		g->allometry.pipe_model_exponent = 2.40f;
		g->allometry.basal_radius_ratio = 0.095f;
		g->allometry.tip_min_radius_m = 0.085f;
		g->allometry.radius_taper = 0.004f;
		g->allometry.root_flare_strength = 0.05f;
		g->allometry.bark_class = 4u; /* ND_PLANT_BARK_RIBBED */
		g->allometry.bark_dark  = plant_rgba_u8(24, 58, 32, 255);
		g->allometry.bark_mid   = plant_rgba_u8(42, 92, 52, 255);
		g->allometry.bark_light = plant_rgba_u8(68, 132, 76, 255);
		g->leaf.complexity = PLANT_LEAF_SCALE; /* areole spines */
		g->leaf.blade_length_m = (PlantDistribution){ 0.06f, 0.01f, 0.03f, 0.10f };
		g->leaf.leaves_per_node = 2u;
		g->leaf.canopy_porosity = 0.20f;
		g->leaf.adaxial_deep = plant_rgba_u8(128, 104, 48, 255);
		g->leaf.adaxial_mid  = plant_rgba_u8(164, 134, 62, 255);
		g->leaf.adaxial_lit  = plant_rgba_u8(188, 158, 78, 255);
		g->leaf.abaxial_high = plant_rgba_u8(224, 196, 112, 255);
		g->flower.has_flowers = true;
		g->flower.inflorescence = PLANT_INFLORESCENCE_SOLITARY;
		g->flower.corolla_type = PLANT_COROLLA_FUNNELFORM;
		g->flower.petal_count = 8u;
		g->flower.flower_radius_m = (PlantDistribution){ 0.095f, 0.012f, 0.05f, 0.14f };
		g->flower.primary_color   = plant_rgba_u8(248, 232, 238, 255);
		g->flower.secondary_color = plant_rgba_u8(204, 116, 142, 255);
		g->physiology.mature_height_m = 1.29f;
		g->physiology.mature_crown_width_m = 0.93f;
		break;

	case 7: /* Acacia senegal (Gum arabic tree) */
		sp->scientific_name = "Acacia senegal";
		sp->common_name = "Gum arabic tree";
		sp->slug = "acacia_senegal";
		sp->life_form = PLANT_LIFEFORM_TREE;
		sp->architectural_model = PLANT_ARCH_TROLL;
		g->architecture.model = PLANT_ARCH_TROLL;
		g->architecture.max_branch_order = 3u;
		g->architecture.basal_stem_count = 1u;
		g->architecture.internode_length_m = (PlantDistribution){ 0.86f, 0.12f, 0.48f, 1.32f };
		g->architecture.branch_angle_deg = (PlantDistribution){ 66.0f, 6.0f, 48.0f, 84.0f };
		g->architecture.apical_dominance = 0.30f;
		g->architecture.acrotony = 0.90f;
		g->architecture.clear_bole_fraction = 0.38f;
		g->architecture.fork_probability = 0.58f;
		g->architecture.orthotropy = 0.08f; /* tabular horizontal savannah umbrella */
		g->architecture.tortuosity = 0.48f;
		g->architecture.crown_flatness = 0.26f;
		g->phyllotaxis.buds_per_node = 3u;
		g->meristem.lateral_bud_probability = 0.88f;
		g->allometry.pipe_model_exponent = 2.05f;
		g->allometry.basal_radius_ratio = 0.038f;
		g->allometry.tip_min_radius_m = 0.024f;
		g->allometry.root_flare_strength = 0.42f;
		g->allometry.bark_class = 0u;
		g->allometry.bark_dark  = plant_rgba_u8(62, 46, 30, 255);
		g->allometry.bark_mid   = plant_rgba_u8(102, 78, 52, 255);
		g->allometry.bark_light = plant_rgba_u8(146, 114, 78, 255);
		g->leaf.complexity = PLANT_LEAF_BIPINNATE;
		g->leaf.shape = PLANT_LEAF_SHAPE_ELLIPTIC;
		g->leaf.blade_length_m = (PlantDistribution){ 0.84f, 0.11f, 0.48f, 1.24f };
		g->leaf.width_ratio = (PlantDistribution){ 0.78f, 0.08f, 0.50f, 0.98f };
		g->leaf.leaflet_count = 6u;
		g->leaf.leaves_per_node = 4u;
		g->leaf.canopy_porosity = 0.56f;
		g->leaf.adaxial_deep = plant_rgba_u8(38, 58, 18, 255);
		g->leaf.adaxial_mid  = plant_rgba_u8(68, 96, 30, 255);
		g->leaf.adaxial_lit  = plant_rgba_u8(106, 138, 46, 255);
		g->leaf.abaxial_high = plant_rgba_u8(148, 178, 68, 255);
		g->physiology.mature_height_m = 6.00f;
		g->physiology.mature_crown_width_m = 8.80f;
		break;

	case 8: /* Daucus carota (Wild carrot) */
		sp->scientific_name = "Daucus carota";
		sp->common_name = "Wild carrot";
		sp->slug = "daucus_carota";
		sp->life_form = PLANT_LIFEFORM_HERB;
		sp->architectural_model = PLANT_ARCH_LEEUWENBERG;
		g->architecture.model = PLANT_ARCH_LEEUWENBERG;
		g->architecture.max_branch_order = 2u;
		g->architecture.basal_stem_count = 4u;
		g->architecture.internode_length_m = (PlantDistribution){ 0.14f, 0.02f, 0.08f, 0.22f };
		g->architecture.branch_angle_deg = (PlantDistribution){ 42.0f, 6.0f, 26.0f, 58.0f };
		g->architecture.apical_dominance = 0.62f;
		g->architecture.basitony = 0.72f;
		g->architecture.clear_bole_fraction = 0.12f;
		g->architecture.orthotropy = 0.58f;
		g->architecture.crown_flatness = 0.56f;
		g->allometry.basal_radius_ratio = 0.032f;
		g->allometry.tip_min_radius_m = 0.007f;
		g->allometry.root_flare_strength = 0.55f;
		g->allometry.bark_class = 5u;
		g->allometry.bark_dark  = plant_rgba_u8(46, 78, 30, 255);
		g->allometry.bark_mid   = plant_rgba_u8(74, 118, 46, 255);
		g->allometry.bark_light = plant_rgba_u8(108, 156, 68, 255);
		g->leaf.complexity = PLANT_LEAF_PINNATE;
		g->leaf.shape = PLANT_LEAF_SHAPE_PINNATELY_LOBED;
		g->leaf.blade_length_m = (PlantDistribution){ 0.18f, 0.03f, 0.10f, 0.28f };
		g->leaf.width_ratio = (PlantDistribution){ 0.58f, 0.06f, 0.38f, 0.80f };
		g->leaf.leaflet_count = 5u;
		g->leaf.leaves_per_node = 3u;
		g->leaf.canopy_porosity = 0.52f;
		g->leaf.retention_order_threshold = 0.0f;
		g->leaf.adaxial_deep = plant_rgba_u8(28, 68, 26, 255);
		g->leaf.adaxial_mid  = plant_rgba_u8(52, 108, 42, 255);
		g->leaf.adaxial_lit  = plant_rgba_u8(82, 148, 64, 255);
		g->leaf.abaxial_high = plant_rgba_u8(118, 184, 88, 255);
		g->flower.has_flowers = true;
		g->flower.inflorescence = PLANT_INFLORESCENCE_UMBEL;
		g->flower.petal_count = 5u;
		g->flower.flower_radius_m = (PlantDistribution){ 0.095f, 0.012f, 0.05f, 0.14f };
		g->flower.primary_color   = plant_rgba_u8(248, 250, 242, 255);
		g->flower.secondary_color = plant_rgba_u8(186, 198, 168, 255);
		g->physiology.mature_height_m = 0.71f;
		g->physiology.mature_crown_width_m = 0.62f;
		g->physiology.max_age_years = 2.0f;
		break;

	case 9: /* Solanum lycopersicum (Tomato) */
		sp->scientific_name = "Solanum lycopersicum";
		sp->common_name = "Tomato";
		sp->slug = "solanum_lycopersicum";
		sp->life_form = PLANT_LIFEFORM_HERB;
		sp->architectural_model = PLANT_ARCH_LEEUWENBERG;
		g->architecture.model = PLANT_ARCH_LEEUWENBERG;
		g->architecture.max_branch_order = 2u;
		g->architecture.basal_stem_count = 3u;
		g->architecture.internode_length_m = (PlantDistribution){ 0.16f, 0.02f, 0.09f, 0.24f };
		g->architecture.branch_angle_deg = (PlantDistribution){ 56.0f, 6.0f, 38.0f, 75.0f };
		g->architecture.apical_dominance = 0.45f;
		g->architecture.basitony = 0.55f;
		g->architecture.clear_bole_fraction = 0.14f;
		g->architecture.orthotropy = 0.35f;
		g->architecture.tortuosity = 0.34f;
		g->architecture.crown_flatness = 0.78f;
		g->phyllotaxis.buds_per_node = 2u;
		g->allometry.basal_radius_ratio = 0.036f;
		g->allometry.tip_min_radius_m = 0.008f;
		g->allometry.bark_class = 5u;
		g->allometry.bark_dark  = plant_rgba_u8(34, 68, 24, 255);
		g->allometry.bark_mid   = plant_rgba_u8(56, 102, 38, 255);
		g->allometry.bark_light = plant_rgba_u8(82, 138, 56, 255);
		g->leaf.complexity = PLANT_LEAF_PINNATE;
		g->leaf.shape = PLANT_LEAF_SHAPE_OVATE;
		g->leaf.margin = PLANT_MARGIN_DENTATE;
		g->leaf.blade_length_m = (PlantDistribution){ 0.20f, 0.03f, 0.12f, 0.30f };
		g->leaf.width_ratio = (PlantDistribution){ 0.66f, 0.06f, 0.45f, 0.88f };
		g->leaf.leaflet_count = 5u;
		g->leaf.leaves_per_node = 3u;
		g->leaf.canopy_porosity = 0.42f;
		g->leaf.retention_order_threshold = 0.0f;
		g->leaf.adaxial_deep = plant_rgba_u8(24, 62, 20, 255);
		g->leaf.adaxial_mid  = plant_rgba_u8(46, 98, 34, 255);
		g->leaf.adaxial_lit  = plant_rgba_u8(76, 138, 52, 255);
		g->leaf.abaxial_high = plant_rgba_u8(112, 174, 74, 255);
		g->flower.has_flowers = true;
		g->flower.inflorescence = PLANT_INFLORESCENCE_RACEME;
		g->flower.petal_count = 5u;
		g->flower.flower_radius_m = (PlantDistribution){ 0.045f, 0.008f, 0.025f, 0.07f };
		g->flower.primary_color   = plant_rgba_u8(246, 218, 54, 255);
		g->flower.secondary_color = plant_rgba_u8(198, 158, 28, 255);
		g->fruit.fruit_type = PLANT_FRUIT_BERRY;
		g->fruit.length_m = (PlantDistribution){ 0.11f, 0.015f, 0.06f, 0.16f };
		g->fruit.diameter_m = (PlantDistribution){ 0.11f, 0.015f, 0.06f, 0.16f };
		g->fruit.immature_color = plant_rgba_u8(76, 138, 52, 255);
		g->fruit.mature_color   = plant_rgba_u8(242, 68, 44, 255);
		g->physiology.mature_height_m = 0.84f;
		g->physiology.mature_crown_width_m = 0.76f;
		g->physiology.max_age_years = 2.0f;
		break;

	case 10: /* Quercus robur (English pedunculate oak) */
		sp->scientific_name = "Quercus robur";
		sp->common_name = "English pedunculate oak";
		sp->slug = "quercus_robur";
		sp->life_form = PLANT_LIFEFORM_TREE;
		sp->architectural_model = PLANT_ARCH_RAUH;
		g->architecture.model = PLANT_ARCH_RAUH;
		g->architecture.max_branch_order = 3u;
		g->architecture.basal_stem_count = 1u;
		g->architecture.internode_length_m = (PlantDistribution){ 0.96f, 0.13f, 0.52f, 1.48f };
		g->architecture.branch_angle_deg = (PlantDistribution){ 64.0f, 7.0f, 44.0f, 84.0f };
		g->architecture.apical_dominance = 0.42f;
		g->architecture.acrotony = 0.78f;
		g->architecture.clear_bole_fraction = 0.28f;
		g->architecture.fork_probability = 0.42f;
		g->architecture.orthotropy = 0.20f;
		g->architecture.tortuosity = 0.52f;
		g->architecture.crown_flatness = 0.72f;
		g->phyllotaxis.buds_per_node = 3u;
		g->meristem.lateral_bud_probability = 0.88f;
		g->allometry.pipe_model_exponent = 2.02f;
		g->allometry.basal_radius_ratio = 0.072f;
		g->allometry.tip_min_radius_m = 0.034f;
		g->allometry.root_flare_strength = 0.72f;
		g->allometry.bark_class = 0u;
		g->allometry.bark_dark  = plant_rgba_u8(42, 32, 24, 255);
		g->allometry.bark_mid   = plant_rgba_u8(74, 58, 44, 255);
		g->allometry.bark_light = plant_rgba_u8(108, 86, 66, 255);
		g->leaf.complexity = PLANT_LEAF_SIMPLE;
		g->leaf.shape = PLANT_LEAF_SHAPE_PINNATELY_LOBED;
		g->leaf.margin = PLANT_MARGIN_LOBED;
		g->leaf.blade_length_m = (PlantDistribution){ 0.78f, 0.10f, 0.45f, 1.15f };
		g->leaf.width_ratio = (PlantDistribution){ 0.68f, 0.07f, 0.48f, 0.88f };
		g->leaf.lobe_depth = (PlantDistribution){ 0.42f, 0.06f, 0.20f, 0.60f };
		g->leaf.leaves_per_node = 4u;
		g->leaf.canopy_porosity = 0.44f;
		g->leaf.adaxial_deep = plant_rgba_u8(18, 48, 18, 255);
		g->leaf.adaxial_mid  = plant_rgba_u8(36, 82, 30, 255);
		g->leaf.adaxial_lit  = plant_rgba_u8(64, 122, 46, 255);
		g->leaf.abaxial_high = plant_rgba_u8(102, 162, 66, 255);
		g->physiology.mature_height_m = 9.60f;
		g->physiology.mature_crown_width_m = 10.00f;
		g->physiology.max_age_years = 500.0f;
		break;

	case 11: /* Salix babylonica (Weeping willow) */
		sp->scientific_name = "Salix babylonica";
		sp->common_name = "Weeping willow";
		sp->slug = "salix_babylonica";
		sp->life_form = PLANT_LIFEFORM_TREE;
		sp->architectural_model = PLANT_ARCH_TROLL;
		g->architecture.model = PLANT_ARCH_TROLL;
		g->architecture.max_branch_order = 3u;
		g->architecture.basal_stem_count = 1u;
		g->architecture.internode_length_m = (PlantDistribution){ 0.88f, 0.12f, 0.48f, 1.34f };
		g->architecture.branch_angle_deg = (PlantDistribution){ 54.0f, 7.0f, 35.0f, 74.0f };
		g->architecture.apical_dominance = 0.44f;
		g->architecture.acrotony = 0.74f;
		g->architecture.clear_bole_fraction = 0.30f;
		g->architecture.fork_probability = 0.38f;
		g->architecture.orthotropy = -0.78f; /* strong weeping cascade on distal axes */
		g->architecture.tortuosity = 0.38f;
		g->architecture.crown_flatness = 0.88f;
		g->phyllotaxis.buds_per_node = 3u;
		g->meristem.lateral_bud_probability = 0.88f;
		g->allometry.pipe_model_exponent = 2.06f;
		g->allometry.basal_radius_ratio = 0.064f;
		g->allometry.tip_min_radius_m = 0.025f;
		g->allometry.stiffness_young_modulus = 0.22f;
		g->allometry.root_flare_strength = 0.58f;
		g->allometry.bark_class = 0u;
		g->allometry.bark_dark  = plant_rgba_u8(48, 38, 28, 255);
		g->allometry.bark_mid   = plant_rgba_u8(82, 66, 50, 255);
		g->allometry.bark_light = plant_rgba_u8(116, 96, 74, 255);
		g->leaf.complexity = PLANT_LEAF_SIMPLE;
		g->leaf.shape = PLANT_LEAF_SHAPE_LANCEOLATE;
		g->leaf.blade_length_m = (PlantDistribution){ 0.86f, 0.11f, 0.50f, 1.28f };
		g->leaf.width_ratio = (PlantDistribution){ 0.34f, 0.05f, 0.20f, 0.50f };
		g->leaf.curvature = (PlantDistribution){ 0.78f, 0.08f, 0.45f, 0.98f };
		g->leaf.leaves_per_node = 4u;
		g->leaf.canopy_porosity = 0.54f;
		g->leaf.adaxial_deep = plant_rgba_u8(34, 68, 24, 255);
		g->leaf.adaxial_mid  = plant_rgba_u8(64, 114, 38, 255);
		g->leaf.adaxial_lit  = plant_rgba_u8(104, 162, 58, 255);
		g->leaf.abaxial_high = plant_rgba_u8(154, 206, 86, 255);
		g->physiology.mature_height_m = 8.00f;
		g->physiology.mature_crown_width_m = 8.80f;
		break;

	case 12: /* Acer pseudoplatanus (Sycamore maple) */
		sp->scientific_name = "Acer pseudoplatanus";
		sp->common_name = "Sycamore maple";
		sp->slug = "acer_pseudoplatanus";
		sp->life_form = PLANT_LIFEFORM_TREE;
		sp->architectural_model = PLANT_ARCH_RAUH;
		g->architecture.model = PLANT_ARCH_RAUH;
		g->architecture.max_branch_order = 3u;
		g->architecture.basal_stem_count = 1u;
		g->architecture.internode_length_m = (PlantDistribution){ 0.92f, 0.11f, 0.50f, 1.38f };
		g->architecture.branch_angle_deg = (PlantDistribution){ 52.0f, 6.0f, 36.0f, 68.0f };
		g->architecture.apical_dominance = 0.58f;
		g->architecture.acrotony = 0.74f;
		g->architecture.clear_bole_fraction = 0.32f;
		g->architecture.orthotropy = 0.42f;
		g->architecture.tortuosity = 0.22f;
		g->architecture.crown_flatness = 0.84f;
		g->phyllotaxis.mode = PLANT_PHYLLO_DECUSSATE;
		g->phyllotaxis.buds_per_node = 2u;
		g->phyllotaxis.divergence_angle_deg = 90.0f;
		g->phyllotaxis.decussate_phase_flip = true;
		g->meristem.lateral_bud_probability = 0.88f;
		g->allometry.pipe_model_exponent = 2.08f;
		g->allometry.basal_radius_ratio = 0.060f;
		g->allometry.tip_min_radius_m = 0.029f;
		g->allometry.root_flare_strength = 0.56f;
		g->allometry.bark_class = 0u;
		g->allometry.bark_dark  = plant_rgba_u8(54, 46, 38, 255);
		g->allometry.bark_mid   = plant_rgba_u8(92, 80, 68, 255);
		g->allometry.bark_light = plant_rgba_u8(132, 116, 102, 255);
		g->leaf.complexity = PLANT_LEAF_PALMATE;
		g->leaf.shape = PLANT_LEAF_SHAPE_PALMATELY_LOBED;
		g->leaf.margin = PLANT_MARGIN_SERRATE;
		g->leaf.venation_type = PLANT_VENATION_PALMATE;
		g->leaf.blade_length_m = (PlantDistribution){ 0.76f, 0.10f, 0.44f, 1.12f };
		g->leaf.width_ratio = (PlantDistribution){ 0.88f, 0.07f, 0.68f, 1.05f };
		g->leaf.leaflet_count = 5u;
		g->leaf.leaves_per_node = 4u;
		g->leaf.canopy_porosity = 0.38f;
		g->leaf.adaxial_deep = plant_rgba_u8(20, 54, 20, 255);
		g->leaf.adaxial_mid  = plant_rgba_u8(42, 92, 34, 255);
		g->leaf.adaxial_lit  = plant_rgba_u8(74, 136, 52, 255);
		g->leaf.abaxial_high = plant_rgba_u8(118, 178, 76, 255);
		g->physiology.mature_height_m = 9.60f;
		g->physiology.mature_crown_width_m = 8.44f;
		break;

	case 13: /* Olea europaea (European olive) */
		sp->scientific_name = "Olea europaea";
		sp->common_name = "European olive";
		sp->slug = "olea_europaea";
		sp->life_form = PLANT_LIFEFORM_TREE;
		sp->architectural_model = PLANT_ARCH_LEEUWENBERG;
		g->architecture.model = PLANT_ARCH_LEEUWENBERG;
		g->architecture.max_branch_order = 3u;
		g->architecture.basal_stem_count = 1u;
		g->architecture.internode_length_m = (PlantDistribution){ 0.68f, 0.09f, 0.36f, 1.04f };
		g->architecture.branch_angle_deg = (PlantDistribution){ 62.0f, 7.0f, 42.0f, 82.0f };
		g->architecture.apical_dominance = 0.34f;
		g->architecture.acrotony = 0.78f;
		g->architecture.clear_bole_fraction = 0.28f;
		g->architecture.fork_probability = 0.52f;
		g->architecture.orthotropy = 0.16f;
		g->architecture.tortuosity = 0.72f; /* ancient twisted Mediterranean trunk */
		g->architecture.crown_flatness = 0.58f;
		g->phyllotaxis.mode = PLANT_PHYLLO_DECUSSATE;
		g->phyllotaxis.buds_per_node = 2u;
		g->phyllotaxis.decussate_phase_flip = true;
		g->meristem.lateral_bud_probability = 0.88f;
		g->allometry.pipe_model_exponent = 1.96f;
		g->allometry.basal_radius_ratio = 0.078f;
		g->allometry.tip_min_radius_m = 0.032f;
		g->allometry.root_flare_strength = 0.78f;
		g->allometry.bark_class = 3u; /* ND_PLANT_BARK_FLUTED */
		g->allometry.bark_dark  = plant_rgba_u8(52, 48, 42, 255);
		g->allometry.bark_mid   = plant_rgba_u8(94, 88, 78, 255);
		g->allometry.bark_light = plant_rgba_u8(142, 134, 122, 255);
		g->leaf.complexity = PLANT_LEAF_SIMPLE;
		g->leaf.shape = PLANT_LEAF_SHAPE_LANCEOLATE;
		g->leaf.blade_length_m = (PlantDistribution){ 0.64f, 0.08f, 0.36f, 0.92f };
		g->leaf.width_ratio = (PlantDistribution){ 0.45f, 0.05f, 0.30f, 0.65f };
		g->leaf.leaves_per_node = 4u;
		g->leaf.canopy_porosity = 0.46f;
		g->leaf.adaxial_deep = plant_rgba_u8(38, 56, 38, 255);
		g->leaf.adaxial_mid  = plant_rgba_u8(68, 94, 66, 255);
		g->leaf.adaxial_lit  = plant_rgba_u8(108, 138, 102, 255);
		g->leaf.abaxial_high = plant_rgba_u8(164, 192, 158, 255);
		g->physiology.mature_height_m = 6.00f;
		g->physiology.mature_crown_width_m = 7.00f;
		g->physiology.max_age_years = 600.0f;
		break;

	case 14: /* Bellis perennis (Common English daisy) */
		sp->scientific_name = "Bellis perennis";
		sp->common_name = "Common English daisy";
		sp->slug = "bellis_perennis";
		sp->life_form = PLANT_LIFEFORM_HERB;
		sp->architectural_model = PLANT_ARCH_HOLTTUM;
		g->architecture.model = PLANT_ARCH_HOLTTUM;
		g->architecture.max_branch_order = 1u;
		g->architecture.basal_stem_count = 4u;
		g->architecture.internode_length_m = (PlantDistribution){ 0.05f, 0.01f, 0.02f, 0.09f };
		g->architecture.branch_angle_deg = (PlantDistribution){ 38.0f, 5.0f, 20.0f, 55.0f };
		g->architecture.apical_dominance = 0.88f;
		g->architecture.basitony = 0.90f;
		g->architecture.clear_bole_fraction = 0.05f;
		g->architecture.orthotropy = 0.82f;
		g->architecture.crown_flatness = 0.45f;
		g->allometry.basal_radius_ratio = 0.026f;
		g->allometry.tip_min_radius_m = 0.004f;
		g->allometry.root_flare_strength = 0.05f;
		g->allometry.bark_class = 5u;
		g->allometry.bark_dark  = plant_rgba_u8(44, 78, 32, 255);
		g->allometry.bark_mid   = plant_rgba_u8(72, 116, 48, 255);
		g->allometry.bark_light = plant_rgba_u8(104, 152, 68, 255);
		g->leaf.complexity = PLANT_LEAF_SIMPLE;
		g->leaf.shape = PLANT_LEAF_SHAPE_OBOVATE; /* basal spoon-shaped rosette leaves */
		g->leaf.blade_length_m = (PlantDistribution){ 0.085f, 0.01f, 0.05f, 0.13f };
		g->leaf.width_ratio = (PlantDistribution){ 0.62f, 0.06f, 0.42f, 0.82f };
		g->leaf.leaves_per_node = 4u;
		g->leaf.canopy_porosity = 0.35f;
		g->leaf.retention_order_threshold = 0.0f;
		g->leaf.adaxial_deep = plant_rgba_u8(28, 68, 24, 255);
		g->leaf.adaxial_mid  = plant_rgba_u8(52, 108, 40, 255);
		g->leaf.adaxial_lit  = plant_rgba_u8(84, 148, 62, 255);
		g->leaf.abaxial_high = plant_rgba_u8(118, 182, 86, 255);
		g->flower.has_flowers = true;
		g->flower.inflorescence = PLANT_INFLORESCENCE_CAPITULUM;
		g->flower.corolla_type = PLANT_COROLLA_LIGULATE;
		g->flower.petal_count = 12u;
		g->flower.flower_radius_m = (PlantDistribution){ 0.055f, 0.008f, 0.03f, 0.085f };
		g->flower.primary_color   = plant_rgba_u8(250, 246, 244, 255);
		g->flower.secondary_color = plant_rgba_u8(232, 182, 34, 255);
		g->physiology.mature_height_m = 0.16f;
		g->physiology.mature_crown_width_m = 0.24f;
		g->physiology.max_age_years = 3.0f;
		break;

	case 15: /* Typha latifolia (Broadleaf cattail / bulrush) */
	default:
		sp->scientific_name = "Typha latifolia";
		sp->common_name = "Broadleaf cattail";
		sp->slug = "typha_latifolia";
		sp->life_form = PLANT_LIFEFORM_GRAMINOID;
		sp->architectural_model = PLANT_ARCH_MCCLURE;
		g->architecture.model = PLANT_ARCH_MCCLURE;
		g->architecture.max_branch_order = 1u;
		g->architecture.basal_stem_count = 7u;
		g->architecture.internode_length_m = (PlantDistribution){ 0.28f, 0.04f, 0.16f, 0.42f };
		g->architecture.branch_angle_deg = (PlantDistribution){ 18.0f, 4.0f, 8.0f, 30.0f };
		g->architecture.apical_dominance = 0.95f;
		g->architecture.basitony = 0.92f;
		g->architecture.clear_bole_fraction = 0.10f;
		g->architecture.orthotropy = 0.92f;
		g->architecture.crown_flatness = 1.20f;
		g->allometry.basal_radius_ratio = 0.038f;
		g->allometry.tip_min_radius_m = 0.011f;
		g->allometry.root_flare_strength = 0.08f;
		g->allometry.bark_class = 5u;
		g->allometry.bark_dark  = plant_rgba_u8(36, 68, 34, 255);
		g->allometry.bark_mid   = plant_rgba_u8(58, 104, 52, 255);
		g->allometry.bark_light = plant_rgba_u8(86, 142, 74, 255);
		g->leaf.complexity = PLANT_LEAF_SIMPLE;
		g->leaf.shape = PLANT_LEAF_SHAPE_LINEAR;
		g->leaf.venation_type = PLANT_VENATION_PARALLELODROMOUS;
		g->leaf.blade_length_m = (PlantDistribution){ 0.56f, 0.07f, 0.34f, 0.82f };
		g->leaf.width_ratio = (PlantDistribution){ 0.14f, 0.02f, 0.08f, 0.22f };
		g->leaf.curvature = (PlantDistribution){ 0.24f, 0.05f, 0.05f, 0.45f };
		g->leaf.leaves_per_node = 3u;
		g->leaf.canopy_porosity = 0.40f;
		g->leaf.retention_order_threshold = 0.0f;
		g->leaf.adaxial_deep = plant_rgba_u8(24, 58, 32, 255);
		g->leaf.adaxial_mid  = plant_rgba_u8(46, 94, 52, 255);
		g->leaf.adaxial_lit  = plant_rgba_u8(74, 134, 76, 255);
		g->leaf.abaxial_high = plant_rgba_u8(108, 172, 102, 255);
		g->flower.has_flowers = true;
		g->flower.inflorescence = PLANT_INFLORESCENCE_SPIKE;
		g->flower.flower_radius_m = (PlantDistribution){ 0.05f, 0.008f, 0.03f, 0.08f };
		g->flower.primary_color   = plant_rgba_u8(68, 38, 20, 255);
		g->flower.secondary_color = plant_rgba_u8(196, 162, 68, 255);
		g->fruit.fruit_type = PLANT_FRUIT_ACHENE;
		g->fruit.length_m = (PlantDistribution){ 0.22f, 0.03f, 0.14f, 0.32f };
		g->fruit.diameter_m = (PlantDistribution){ 0.048f, 0.006f, 0.03f, 0.07f };
		g->fruit.immature_color = plant_rgba_u8(68, 38, 20, 255);
		g->fruit.mature_color   = plant_rgba_u8(196, 162, 68, 255);
		g->physiology.mature_height_m = 1.38f;
		g->physiology.mature_crown_width_m = 0.93f;
		g->physiology.max_age_years = 8.0f;
		break;
	}

	/* Derive species population priors directly around the canonical genome */
	#define PLANT_INIT_PRIOR_DIST(prior_field, dist_field, mean_sig_frac, trait_id) do { \
		(prior_field).mean_prior  = (dist_field).mean; \
		(prior_field).mean_sigma  = (dist_field).mean * (mean_sig_frac); \
		(prior_field).intra_sigma = (dist_field).sigma; \
		(prior_field).hard_min    = (dist_field).min; \
		(prior_field).hard_max    = (dist_field).max; \
		(prior_field).trait       = (trait_id); \
		(prior_field).population_distribution = (PlantDistribution){ \
			(dist_field).mean, (dist_field).mean * (mean_sig_frac), (dist_field).min, (dist_field).max \
		}; \
		(prior_field).heritability   = 0.72f; \
		(prior_field).mutation_sigma = (dist_field).mean * (mean_sig_frac) * 0.25f; \
	} while (0)

	PLANT_INIT_PRIOR_DIST(sp->internode_length_m, g->architecture.internode_length_m, 0.08f, PLANT_TRAIT_INTERNODE_LENGTH);
	PLANT_INIT_PRIOR_DIST(sp->branch_angle_deg,   g->architecture.branch_angle_deg,   0.07f, PLANT_TRAIT_BRANCH_ANGLE);
	PLANT_INIT_PRIOR_DIST(sp->leaf_length_m,      g->leaf.blade_length_m,             0.08f, PLANT_TRAIT_LEAF_LENGTH);
	PLANT_INIT_PRIOR_DIST(sp->leaf_width_ratio,   g->leaf.width_ratio,                0.07f, PLANT_TRAIT_LEAF_WIDTH_RATIO);
	PLANT_INIT_PRIOR_DIST(sp->flower_radius_m,    g->flower.flower_radius_m,          0.08f, PLANT_TRAIT_BUD_ACTIVATION);
	PLANT_INIT_PRIOR_DIST(sp->fruit_length_m,     g->fruit.length_m,                  0.08f, PLANT_TRAIT_ROOT_DEPTH);

	sp->apical_dominance = (PlantTraitPrior){
		g->architecture.apical_dominance, 0.04f, 0.02f, 0.0f, 1.0f,
		PLANT_TRAIT_BUD_ACTIVATION,
		{ g->architecture.apical_dominance, 0.04f, 0.0f, 1.0f },
		0.70f, 0.015f
	};
	sp->pipe_model_exponent = (PlantTraitPrior){
		g->allometry.pipe_model_exponent, 0.05f, 0.02f, 1.50f, 2.80f,
		PLANT_TRAIT_BRANCH_EXPONENT,
		{ g->allometry.pipe_model_exponent, 0.05f, 1.50f, 2.80f },
		0.78f, 0.015f
	};
	sp->growth_rate = (PlantTraitPrior){
		g->physiology.growth_rate, 0.06f, 0.03f, 0.20f, 2.50f,
		PLANT_TRAIT_PRIMARY_EXTENSION,
		{ g->physiology.growth_rate, 0.06f, 0.20f, 2.50f },
		0.68f, 0.02f
	};
	sp->max_age_years = (PlantTraitPrior){
		g->physiology.max_age_years, g->physiology.max_age_years * 0.08f, 1.0f,
		0.5f, g->physiology.max_age_years * 1.5f,
		PLANT_TRAIT_LATERAL_EXTENSION,
		{ g->physiology.max_age_years, g->physiology.max_age_years * 0.08f, 0.5f, g->physiology.max_age_years * 1.5f },
		0.75f, g->physiology.max_age_years * 0.02f
	};
	#undef PLANT_INIT_PRIOR_DIST

	/* Populate canonical v1.0 trait-indexed priors array */
	sp->inline_priors[0] = sp->growth_rate;
	sp->inline_priors[1] = sp->internode_length_m;
	sp->inline_priors[2] = sp->branch_angle_deg;
	sp->inline_priors[3] = sp->apical_dominance;
	sp->inline_priors[4] = sp->leaf_length_m;
	sp->inline_priors[5] = sp->leaf_width_ratio;
	sp->inline_priors[6] = sp->pipe_model_exponent;
	sp->inline_priors[7] = sp->flower_radius_m;
	sp->inline_priors[8] = sp->fruit_length_m;
	sp->inline_priors[9] = sp->max_age_years;
	sp->priors = sp->inline_priors;
	sp->prior_count = 10u;

	/* Populate canonical v1.0 Cholesky-validated correlation group (symmetric, unit diagonal, PSD) */
	sp->inline_corr_traits[0] = PLANT_TRAIT_PRIMARY_EXTENSION;
	sp->inline_corr_traits[1] = PLANT_TRAIT_INTERNODE_LENGTH;
	sp->inline_corr_traits[2] = PLANT_TRAIT_LEAF_LENGTH;
	sp->inline_corr_traits[3] = PLANT_TRAIT_BRANCH_EXPONENT;
	{
		const float corr4x4[16] = {
			 1.00f,  0.42f,  0.35f, -0.12f,
			 0.42f,  1.00f,  0.48f, -0.08f,
			 0.35f,  0.48f,  1.00f, -0.05f,
			-0.12f, -0.08f, -0.05f,  1.00f
		};
		memcpy(sp->inline_corr_matrix, corr4x4, sizeof(corr4x4));
	}
	sp->inline_corr_group.trait_count = 4u;
	sp->inline_corr_group.traits = sp->inline_corr_traits;
	sp->inline_corr_group.correlation_matrix = sp->inline_corr_matrix;
	sp->correlations = &sp->inline_corr_group;
	sp->correlation_count = 1u;

	/* Populate canonical v1.0 PlantDevelopmentProgram algorithm table */
	sp->development.axis_growth      = (PlantAlgorithm){ PLANT_ALG_AXIS_EXTENSION_V1,   1u };
	sp->development.bud_development  = (PlantAlgorithm){ PLANT_ALG_BUD_ACTIVATION_V1,   1u };
	sp->development.branching        = (PlantAlgorithm){ PLANT_ALG_BRANCHING_V1,        1u };
	sp->development.phyllotaxis      = (PlantAlgorithm){ PLANT_ALG_PHYLLOTAXIS_V1,      1u };
	sp->development.leaf_development = (PlantAlgorithm){ PLANT_ALG_LEAF_EXPANSION_V1,   1u };
	sp->development.root_development = (PlantAlgorithm){ PLANT_ALG_AXIS_EXTENSION_V1,   1u };
	sp->development.tropism          = (PlantAlgorithm){ PLANT_ALG_TROPISM_V1,          1u };
	sp->development.secondary_growth = (PlantAlgorithm){ PLANT_ALG_SECONDARY_GROWTH_V1, 1u };
	sp->development.senescence       = (PlantAlgorithm){ PLANT_ALG_SENESCENCE_V1,       1u };

	plant_genome_sync_traits(g);
	sp->defaults = g->traits;
}

/* ============================================================================
 * 10. Chrono-Deterministic Developmental Simulator & Ephemeral Delta Decay
 * ============================================================================ */

static inline void
plant_phenotype_bind_storage(PlantPhenotype *p, PlantPhenotypeStorage *storage)
{
	if (!p || !storage) return;
	memset(p, 0, sizeof(*p));
	p->axes          = storage->axes;
	p->axis_capacity = PLANT_MAX_AXES;
	p->units         = storage->units;
	p->unit_capacity = PLANT_MAX_UNITS;
	p->buds          = storage->buds;
	p->bud_capacity  = PLANT_MAX_BUDS;
	p->organs        = storage->organs;
	p->organ_capacity = PLANT_MAX_ORGANS;
}

static inline void
plant_phenotype_reset(PlantPhenotype *p)
{
	if (!p) return;
	p->axis_count = 0u;
	p->unit_count = 0u;
	p->bud_count = 0u;
	p->organ_count = 0u;
	p->height_m = 0.0f;
	p->crown_radius_m = 0.0f;
	p->trunk_base_radius_m = 0.0f;
	p->total_leaf_area_m2 = 0.0f;
}

/* Evaluate environmental vigour multiplier V_env in [0.20, 1.15] */
static inline float
plant_eval_env_vigor(const PlantGenome *g, const PlantEnvironment *env)
{
	if (!g || !env) return 1.0f;
	float dt = env->temperature_c - g->physiology.temperature_optimum_c;
	float t_span = (dt >= 0.0f)
		? fmaxf(5.0f, g->physiology.heat_tolerance_c - g->physiology.temperature_optimum_c)
		: fmaxf(5.0f, g->physiology.temperature_optimum_c - g->physiology.cold_tolerance_c);
	float v_temp = plant_clampf(1.0f - 0.5f * (dt / t_span) * (dt / t_span), 0.25f, 1.10f);

	float dw = env->soil_moisture - g->physiology.water_optimum;
	float v_water = plant_clampf(1.0f - (dw * dw) / fmaxf(0.15f, g->physiology.drought_tolerance), 0.30f, 1.10f);

	float dl = env->sunlight - g->physiology.light_optimum;
	float v_light = plant_clampf(1.0f - 0.5f * (dl * dl) / fmaxf(0.15f, g->physiology.light_tolerance), 0.35f, 1.10f);

	return plant_clampf(v_temp * v_water * v_light * g->physiology.growth_rate, 0.20f, 1.25f);
}

/* Inspect active ephemeral deltas for a specific topological node at plant age `age_days`.
 * Returns:
 *   - out_prune_eta: [0..1) if this node/axis was severed and is regrowing, or 1.0f if intact/healed
 *   - out_foliage_eta: [0..1) if leaves were stripped and are regrowing, or 1.0f if intact/healed
 *   - out_fruit_eta: [0..1) if fruit/flowers were harvested and are regrowing, or 1.0f if intact/healed */
static inline void
plant_eval_node_deltas(uint64_t node_topo_hash, float age_days,
                       const PlantDeltaRecord *deltas, uint32_t delta_count,
                       float *out_prune_eta, float *out_foliage_eta, float *out_fruit_eta)
{
	float prune_eta = 1.0f;
	float foliage_eta = 1.0f;
	float fruit_eta = 1.0f;

	for (uint32_t i = 0; i < delta_count; i++) {
		const PlantDeltaRecord *d = &deltas[i];
		if (d->kind == PLANT_DELTA_NONE)
			continue;
		if (d->target_topo_hash != 0ULL && d->target_topo_hash != node_topo_hash)
			continue;
		if (age_days < d->t_event_days)
			continue; /* event is in the future relative to queried age_days */
		float elapsed = age_days - d->t_event_days;
		if (d->t_heal_days <= 1e-5f || elapsed >= d->t_heal_days)
			continue; /* 100% healed -> expired delta has zero effect! */

		float eta = plant_clampf(elapsed / d->t_heal_days, 0.0f, 1.0f);
		if (d->kind == PLANT_DELTA_PRUNE_AXIS || d->kind == PLANT_DELTA_FELL_PLANT) {
			if (eta < prune_eta) prune_eta = eta;
		} else if (d->kind == PLANT_DELTA_STRIP_FOLIAGE) {
			if (eta < foliage_eta) foliage_eta = eta;
		} else if (d->kind == PLANT_DELTA_HARVEST_FRUIT) {
			if (eta < fruit_eta) fruit_eta = eta;
		}
	}

	if (out_prune_eta)   *out_prune_eta   = prune_eta;
	if (out_foliage_eta) *out_foliage_eta = foliage_eta;
	if (out_fruit_eta)   *out_fruit_eta   = fruit_eta;
}

/* Garbage-collect expired deltas in-place. Returns the new active delta_count. */
static inline uint32_t
plant_deltas_compact_expired(PlantDeltaRecord *deltas, uint32_t delta_count, float current_age_days)
{
	if (!deltas || delta_count == 0u)
		return 0u;
	uint32_t keep = 0u;
	for (uint32_t i = 0; i < delta_count; i++) {
		const PlantDeltaRecord *d = &deltas[i];
		if (d->kind == PLANT_DELTA_NONE)
			continue;
		if (d->t_heal_days > 0.0f && current_age_days >= d->t_event_days + d->t_heal_days)
			continue; /* Healed & expired: forget to save memory! */
		if (keep != i)
			deltas[keep] = deltas[i];
		keep++;
	}
	return keep;
}

/* Smooth logistic expansion curve for fractional phytomer elongation f in [0, 1] */
static inline float
plant_logistic_expansion(float f)
{
	if (f <= 0.0f) return 0.0f;
	if (f >= 1.0f) return 1.0f;
	float raw = 1.0f / (1.0f + expf(-9.0f * (f - 0.38f)));
	float lo  = 1.0f / (1.0f + expf(-9.0f * (0.0f - 0.38f)));
	float hi  = 1.0f / (1.0f + expf(-9.0f * (1.0f - 0.38f)));
	return plant_clampf((raw - lo) / (hi - lo), 0.0f, 1.0f);
}

/* Internal recursive axis expansion using coordinate-addressed topo_hash with Growth-Time 3D Light Competition */
static inline void
plant_sim_develop_axis_recursive(
	PlantInstance *inst,
	const PlantEnvironment *env,
	float plant_age_days,
	float eff_axis_age_days,
	float tau_plastochron,
	PlantAxisId parent_axis_id,
	PlantUnitId parent_unit_id,
	PlantBudId origin_bud_id,
	uint8_t order,
	uint64_t axis_topo_hash,
	PlantVec3 start_pos,
	PlantVec3 start_dir,
	float inherited_foliage_eta,
	float inherited_fruit_eta,
	const PlantDeltaRecord *deltas,
	uint32_t delta_count)
{
	PlantPhenotype *pheno = &inst->phenotype;
	const PlantGenome *g = &inst->genome;
	if (pheno->axis_count >= pheno->axis_capacity ||
	    pheno->unit_count >= pheno->unit_capacity ||
	    eff_axis_age_days <= 0.02f * tau_plastochron)
		return;

	/* Check if this entire axis is subject to an active prune/strip delta */
	float axis_prune_eta = 1.0f, axis_fol_eta = inherited_foliage_eta, axis_frt_eta = inherited_fruit_eta;
	plant_eval_node_deltas(axis_topo_hash, plant_age_days, deltas, delta_count,
	                       &axis_prune_eta, &axis_fol_eta, &axis_frt_eta);
	eff_axis_age_days *= axis_prune_eta;
	if (eff_axis_age_days <= 0.02f * tau_plastochron)
		return;

	/* Order-specific plastochron rhythm & maximum phytomer cap:
	 * Lateral shoots and terminal twigs flush with a shorter plastochron than the primary trunk
	 * so upper-crown boughs have full physiological time to subdivide into order-2 and order-3 twigs. */
	float order_vigor = powf(plant_clampf(g->meristem.vigor_decay_per_order, 0.30f, 0.90f), (float)order);
	float order_tau = tau_plastochron * powf(0.68f, (float)order);
	bool is_tree_arch = (g->architecture.max_branch_order >= 3u);
	bool is_multi_stem = (g->architecture.basal_stem_count >= 2u);
	float crown_flat = (g->architecture.crown_flatness > 0.05f) ? g->architecture.crown_flatness : 0.75f;
	float tabular_t = is_tree_arch ? plant_clampf((0.66f - crown_flat) / 0.42f, 0.0f, 1.0f) : 0.0f;
	bool is_excurrent_spire = (is_tree_arch &&
	                           g->architecture.apical_dominance >= 0.85f &&
	                           g->phyllotaxis.buds_per_node >= 3u);
	bool is_sympodial = (is_tree_arch && !is_excurrent_spire &&
	                     (g->architecture.apical_dominance < 0.78f ||
	                      g->architecture.fork_probability > 0.24f ||
	                      g->architecture.clear_bole_fraction >= 0.50f));

	uint32_t max_nodes_for_order = 1u;
	if (order == 0u) {
		if (g->architecture.max_branch_order <= 1u)
			max_nodes_for_order = 3u;
		else if (is_excurrent_spire)
			max_nodes_for_order = 7u;
		else if (is_tree_arch)
			max_nodes_for_order = 5u;
		else
			max_nodes_for_order = 4u;
	} else if (order == 1u) {
		max_nodes_for_order = (is_tree_arch && !is_excurrent_spire && !is_multi_stem) ? 3u : 2u;
	} else if (order == 2u) {
		max_nodes_for_order = (is_tree_arch && !is_excurrent_spire) ? 2u : 1u;
	} else {
		max_nodes_for_order = 1u;
	}

	uint32_t max_children_for_axis = 2u;
	if (order == 0u) {
		if (is_excurrent_spire)
			max_children_for_axis = 18u; /* 6 crown whorls x 3 plagiotropic branches all the way to spire apex */
		else if (is_tree_arch)
			max_children_for_axis = is_multi_stem ? 4u : 8u;
		else
			max_children_for_axis = 4u;
	} else if (order == 1u) {
		max_children_for_axis = is_excurrent_spire ? 2u : (is_multi_stem ? 2u : 3u);
	} else if (order == 2u) {
		max_children_for_axis = is_excurrent_spire ? 1u : 2u;
	}

	float exact_nodes = eff_axis_age_days / fmaxf(1.0f, order_tau);
	if (exact_nodes > (float)max_nodes_for_order)
		exact_nodes = (float)max_nodes_for_order;

	uint32_t full_nodes = (uint32_t)floorf(exact_nodes);
	float frac_node = exact_nodes - (float)full_nodes;
	uint32_t total_nodes = full_nodes + ((frac_node > 0.04f && full_nodes < max_nodes_for_order) ? 1u : 0u);
	if (total_nodes == 0u)
		return;

	/* Conical whorl taper for excurrent spires: upper-spire lateral branches scale down smoothly toward the apex */
	float parent_rel_z = 0.0f;
	if (order >= 1u && parent_unit_id != PLANT_INVALID_ID && parent_unit_id < pheno->unit_count) {
		const PlantGrowthUnit *pu = &pheno->units[parent_unit_id];
		if (pu->branch_order == 0u) {
			parent_rel_z = plant_clampf((float)(pu->node_index + 1u) / 7.0f, 0.0f, 0.95f);
		} else if (pheno->height_m > 0.5f) {
			parent_rel_z = plant_clampf(start_pos.z / pheno->height_m, 0.0f, 0.95f);
		}
	}

	/* Allocate the PlantAxis */
	PlantAxisId axis_id = pheno->axis_count++;
	PlantAxis *ax = &pheno->axes[axis_id];
	memset(ax, 0, sizeof(*ax));
	ax->id = axis_id;
	ax->parent_axis = parent_axis_id;
	ax->parent_unit = parent_unit_id;
	ax->origin_bud = origin_bud_id;
	ax->axis_class = (order == 0u)
		? PLANT_AXIS_PRIMARY
		: ((order < g->architecture.max_branch_order) ? PLANT_AXIS_LATERAL : PLANT_AXIS_SHORT_SHOOT);
	ax->order = order;
	ax->alive = 1u;
	ax->is_root_axis = false;
	ax->first_unit = PLANT_INVALID_ID;
	ax->last_unit = PLANT_INVALID_ID;
	ax->birth_day = fmaxf(0.0f, plant_age_days - eff_axis_age_days);
	ax->age_days = eff_axis_age_days;
	ax->topo_hash = axis_topo_hash;

	if (origin_bud_id != PLANT_INVALID_ID && origin_bud_id < pheno->bud_count) {
		pheno->buds[origin_bud_id].state = PLANT_BUD_DEVELOPED;
		pheno->buds[origin_bud_id].resulting_axis = axis_id;
	}

	PlantVec3 cur_pos = start_pos;
	PlantVec3 cur_dir = plant_v3_norm(start_dir);
	uint64_t cur_hash = axis_topo_hash;
	PlantUnitId prev_uid = PLANT_INVALID_ID;
	float accum_phyllo_deg = (float)(axis_topo_hash & 0xFFULL) * (360.0f / 256.0f);

	/* Struct to defer lateral child axis recursion until all units of this axis exist */
	struct {
		PlantUnitId parent_uid;
		PlantBudId  bud_id;
		uint64_t    child_hash;
		PlantVec3   pos;
		PlantVec3   dir;
		float       child_age_days;
		float       fol_eta;
		float       frt_eta;
	} pending_children[24];
	uint32_t pending_count = 0u;

	for (uint32_t n = 0u; n < total_nodes; n++) {
		if (pheno->unit_count >= pheno->unit_capacity)
			break;
		cur_hash = plant_topo_step_apical(cur_hash, n);

		/* Evaluate node-level deltas (e.g. pruning mid-branch at a specific phytomer) */
		float unit_prune_eta = 1.0f, unit_fol_eta = axis_fol_eta, unit_frt_eta = axis_frt_eta;
		plant_eval_node_deltas(cur_hash, plant_age_days, deltas, delta_count,
		                       &unit_prune_eta, &unit_fol_eta, &unit_frt_eta);
		if (unit_prune_eta <= 0.02f)
			break; /* Branch severed right at this phytomer! */

		float raw_maturity = (n < full_nodes) ? 1.0f : frac_node;
		raw_maturity *= unit_prune_eta;
		float maturity = plant_logistic_expansion(raw_maturity);
		if (maturity <= 0.01f)
			break;

		float rel_height = (float)(n + 1u) / (float)max_nodes_for_order;
		/* Normalized position along the axis [0..1] for accurate clear-bole gating */
		float node_bole_frac = (max_nodes_for_order > 1u)
			? ((float)n / (float)(max_nodes_for_order - 1u))
			: 1.0f;

		/* Sample internode length & directional tropism/tortuosity from cur_hash */
		float base_len = plant_sample_distribution(&g->architecture.internode_length_m, cur_hash, PLANT_CH_INTERNODE_LEN);
		float order_len_scale = powf(0.76f, (float)order);
		if (is_excurrent_spire && order >= 1u) {
			/* Conical spire profile: lower whorls reach wide; upper whorls taper smoothly to the spire leader */
			float cone_taper = 1.24f * (1.0f - 0.74f * parent_rel_z);
			order_len_scale *= plant_clampf(cone_taper, 0.24f, 1.24f);
		} else if (is_sympodial && order == 0u && node_bole_frac >= g->architecture.clear_bole_fraction) {
			/* Above the crown break, the sympodial leader shortens as energy divides into codominant V-vase boughs */
			order_len_scale *= (0.82f - 0.34f * tabular_t);
		} else if (is_sympodial && order == 1u) {
			/* Primary scaffold boughs carry broad diagonal V-vase reach up to the canopy table */
			order_len_scale *= (1.14f + 0.32f * tabular_t);
		} else if (is_sympodial && order >= 2u && tabular_t > 0.1f) {
			/* Secondary/tertiary tabular shoots spread across the upper canopy table */
			order_len_scale *= (1.0f + 0.22f * tabular_t);
		}
		float taper_along_axis = 1.0f - 0.10f * ((float)n / (float)max_nodes_for_order);
		float unit_len = base_len * order_len_scale * taper_along_axis * maturity;

		/* Tropism & tortuosity steering */
		PlantVec3 U, V;
		plant_v3_ortho_basis(cur_dir, &U, &V);
		float tort = g->architecture.tortuosity * (0.65f + 0.28f * (float)order);
		float jx = plant_hash_normal01(cur_hash, PLANT_CH_TORTUOSITY_X) * tort * 0.44f;
		float jy = plant_hash_normal01(cur_hash, PLANT_CH_TORTUOSITY_Y) * tort * 0.44f;
		PlantVec3 steered = plant_v3_add(cur_dir, plant_v3_add(plant_v3_scale(U, jx), plant_v3_scale(V, jy)));

		/* Orthotropy / gravitropism / phototropism / sympodial crown-break / weeping bending */
		float ortho = g->architecture.orthotropy;
		if (order == 0u) {
			if (is_sympodial && node_bole_frac >= g->architecture.clear_bole_fraction && n >= 1u) {
				/* Sympodial crown break: above the clear bole, the primary trunk deflects away from
				 * a rigid vertical pole so the upper crown forms a true fractal V-fork network */
				float sym_ang = (accum_phyllo_deg + 137.5f * (float)n) * (3.14159265f / 180.0f);
				float sym_mag = (1.05f - g->architecture.apical_dominance) * (0.52f + 0.48f * tabular_t) + tort * 0.25f;
				steered.x += cosf(sym_ang) * sym_mag;
				steered.y += sinf(sym_ang) * sym_mag;
				steered.z += (0.22f - 0.14f * tabular_t) * g->plasticity.gravitropism_strength;
			} else {
				/* Excurrent spire or lower clear bole is strongly orthotropic */
				steered.z += 0.48f * g->plasticity.gravitropism_strength;
			}
		} else if (is_excurrent_spire) {
			/* Monopodial whorled spire: lateral whorl branches sweep outward plagiotropically
			 * with a gentle horizontal stance and upturned distal tips (never diving into the ground) */
			float horiz_xy = sqrtf(steered.x * steered.x + steered.y * steered.y);
			if (horiz_xy < 0.25f) {
				float fallback_az = accum_phyllo_deg * (3.14159265f / 180.0f);
				steered.x += cosf(fallback_az) * 0.75f;
				steered.y += sinf(fallback_az) * 0.75f;
			}
			steered.z = (order == 1u)
				? ((n == 0u) ? -0.04f : 0.06f)
				: 0.02f;
		} else {
			/* Centrifugal phototropism: lateral boughs steer outward away from the shaded trunk core */
			float r_xy = sqrtf(cur_pos.x * cur_pos.x + cur_pos.y * cur_pos.y);
			if (r_xy > 0.06f) {
				float centrifugal = (0.18f + 0.08f * (float)order) * (1.0f + 0.30f * tabular_t);
				steered.x += (cur_pos.x / r_xy) * centrifugal;
				steered.y += (cur_pos.y / r_xy) * centrifugal;
			}
			if (ortho < -0.20f && order == 1u && n == 0u) {
				/* Weeping architectures (e.g. Salix, Betula pendula): primary scaffold limbs
				 * arch upward first to build a high structural dome before distal shoots cascade */
				steered.z = fmaxf(0.34f, steered.z + 0.48f);
			} else if (tabular_t > 0.05f && order == 1u) {
				/* Low crown_flatness (tabular umbrella / high dome): primary order-1 scaffold limbs
				 * ascend diagonally in a wide V-vase from the crown break up to the upper canopy table */
				float target_climb = 0.40f + 0.28f * tabular_t;
				steered.z = steered.z * (1.0f - 0.65f * tabular_t) + target_climb * tabular_t;
				steered.z = fmaxf(steered.z, 0.28f + 0.22f * tabular_t);
			} else if (tabular_t > 0.05f && order >= 2u) {
				/* Secondary and tertiary shoots on tabular crowns flatten horizontally into stratified tiers */
				steered.z = steered.z * (1.0f - 0.85f * tabular_t) + 0.08f * tabular_t;
			} else {
				/* Standard orthotropy (positive=ascending dome, negative=weeping distal shoots) */
				float grav_pull = ortho * 0.36f - (1.0f - g->allometry.stiffness_young_modulus) * 0.24f * (float)order;
				steered.z += grav_pull;
			}
		}
		if (env) {
			steered = plant_v3_add(steered, plant_v3_scale(env->mean_light_dir, g->plasticity.phototropism_strength * 0.16f));
		}
		/* Growth-time 3D light gradient phototropic steering using the Beer-Lambert voxel grid */
		if (inst->light_vox_valid && order >= 1u) {
			enum { LF_N = 12 };
			float r_bound = inst->light_vox_bounds_r;
			float h_bound = inst->light_vox_bounds_h;
			if (r_bound > 0.1f && h_bound > 0.1f) {
				float inv_2r = 1.0f / (2.0f * r_bound);
				float inv_h  = 1.0f / h_bound;
				int ix = (int)floorf(((cur_pos.x + r_bound) * inv_2r) * (float)LF_N);
				int iy = (int)floorf(((cur_pos.y + r_bound) * inv_2r) * (float)LF_N);
				int iz = (int)floorf((cur_pos.z * inv_h) * (float)LF_N);
				if (ix >= 0 && ix < LF_N && iy >= 0 && iy < LF_N && iz >= 0 && iz < LF_N) {
					int ix_p = (ix + 1 < LF_N) ? (ix + 1) : ix;
					int ix_m = (ix > 0) ? (ix - 1) : ix;
					int iy_p = (iy + 1 < LF_N) ? (iy + 1) : iy;
					int iy_m = (iy > 0) ? (iy - 1) : iy;
					int iz_p = (iz + 1 < LF_N) ? (iz + 1) : iz;
					float gx = inst->light_vox_val[iz][iy][ix_p] - inst->light_vox_val[iz][iy][ix_m];
					float gy = inst->light_vox_val[iz][iy_p][ix] - inst->light_vox_val[iz][iy_m][ix];
					float gz = inst->light_vox_val[iz_p][iy][ix] - inst->light_vox_val[iz][iy][ix];
					if (gx * gx + gy * gy + gz * gz > 1e-4f) {
						PlantVec3 grad = plant_v3_norm(plant_v3(gx, gy, fmaxf(0.0f, gz)));
						float photo_str = plant_clampf(g->plasticity.phototropism_strength * 0.5f +
						                                   g->plasticity.shade_avoidance * 0.5f, 0.0f, 1.0f);
						steered = plant_v3_add(steered, plant_v3_scale(grad, photo_str * 0.45f));
					}
				}
			}
		}
		cur_dir = plant_v3_norm(steered);

		/* Enforce genome-driven crown-base floor so lateral branches never plunge below the clear bole */
		float min_branch_z = 0.05f;
		if (is_tree_arch && order >= 1u) {
			float ref_h = (pheno->height_m > 1.0f) ? pheno->height_m : (g->physiology.mature_height_m * 0.45f);
			float floor_frac = (ortho < -0.25f)
				? fmaxf(0.28f, g->architecture.clear_bole_fraction * 0.90f)
				: fmaxf(0.22f, g->architecture.clear_bole_fraction * 0.95f);
			if (order >= 2u && tabular_t > 0.25f) {
				/* On tabular crowns, order>=2 canopy-table shoots sit above the ascending order-1 V-vase scaffold */
				floor_frac = fmaxf(floor_frac, g->architecture.clear_bole_fraction + 0.26f * tabular_t);
			}
			min_branch_z = fmaxf(0.15f, ref_h * floor_frac);
		}
		if (cur_pos.z + cur_dir.z * unit_len < min_branch_z) {
			float req_dz = (min_branch_z - cur_pos.z) / fmaxf(0.1f, unit_len);
			cur_dir.z = fmaxf(cur_dir.z, plant_clampf(req_dz, 0.04f, 0.55f));
			cur_dir = plant_v3_norm(cur_dir);
		}

		PlantVec3 tip_pos = plant_v3_add(cur_pos, plant_v3_scale(cur_dir, unit_len));

		/* Allocate PlantGrowthUnit */
		PlantUnitId uid = pheno->unit_count++;
		PlantGrowthUnit *u = &pheno->units[uid];
		memset(u, 0, sizeof(*u));
		u->id = uid;
		u->axis_id = axis_id;
		u->prev_in_axis = prev_uid;
		u->next_in_axis = PLANT_INVALID_ID;
		u->parent_unit = (n == 0u) ? parent_unit_id : prev_uid;
		u->type = PLANT_UNIT_PHYTOMER;
		u->base_pos = cur_pos;
		u->tip_pos = tip_pos;
		u->orientation = plant_quat_from_tangent(cur_dir, accum_phyllo_deg * (3.14159265f / 180.0f));
		u->length_m = unit_len;
		u->local_azimuth_rad = atan2f(cur_dir.y, cur_dir.x);
		u->local_elevation_rad = asinf(plant_clampf(cur_dir.z, -1.0f, 1.0f));
		u->birth_day = plant_age_days - (eff_axis_age_days - (float)n * order_tau);
		u->maturity = maturity;
		u->branch_order = order;
		u->node_index = (uint8_t)n;
		u->topo_hash = cur_hash;
		u->first_bud = PLANT_INVALID_ID;
		u->bud_count = 0u;
		u->first_organ = PLANT_INVALID_ID;
		u->organ_count = 0u;

		if (prev_uid != PLANT_INVALID_ID)
			pheno->units[prev_uid].next_in_axis = uid;
		else
			ax->first_unit = uid;
		ax->last_unit = uid;
		ax->unit_count++;
		ax->length_m += unit_len;

		/* Update macroscopic bounding envelope */
		if (tip_pos.z > pheno->height_m)
			pheno->height_m = tip_pos.z;
		float horiz_r = sqrtf(tip_pos.x * tip_pos.x + tip_pos.y * tip_pos.y);
		if (horiz_r > pheno->crown_radius_m)
			pheno->crown_radius_m = horiz_r;

		/* Advance phyllotactic rotation around cur_dir */
		float div_deg = g->phyllotaxis.divergence_angle_deg +
			plant_hash_normal01(cur_hash, PLANT_CH_PHYLLO_JITTER) * g->phyllotaxis.divergence_jitter_deg;
		if (g->phyllotaxis.mode == PLANT_PHYLLO_DECUSSATE && g->phyllotaxis.decussate_phase_flip && (n & 1u))
			div_deg += 90.0f;
		accum_phyllo_deg = fmodf(accum_phyllo_deg + div_deg, 360.0f);

		/* Emit axillary buds & foliar/floral organs at this phytomer node */
		uint8_t buds_here = g->phyllotaxis.buds_per_node;
		if (buds_here == 0u) buds_here = 1u;
		if (order >= 1u && !is_excurrent_spire && buds_here > 2u)
			buds_here = 2u;
		float dist_from_apex = (float)(total_nodes - 1u - n);

		/* Apical dominance hormonal inhibition: if the apex downstream was pruned,
		 * dist_from_apex is 0 at the cut face, releasing dormant buds below the cut! */
		float apical_inhib = g->architecture.apical_dominance * expf(-dist_from_apex * 0.85f);
		if (n == total_nodes - 1u && unit_prune_eta < 0.95f)
			apical_inhib = 0.0f; /* traumatic epicormic release below prune scar */

		/* Positional activation profile (clear bole self-pruning vs acrotony/basitony) */
		float pos_weight = 1.0f;
		if (order == 0u && node_bole_frac < g->architecture.clear_bole_fraction && total_nodes > 2u && n < total_nodes - 1u) {
			pos_weight = 0.0f; /* self-pruned clear bole zone */
		} else {
			float acro = g->architecture.acrotony * rel_height;
			float basi = g->architecture.basitony * (1.0f - rel_height);
			float meso = (1.0f - 0.5f * (g->architecture.acrotony + g->architecture.basitony));
			pos_weight = plant_clampf(0.45f + acro + basi + 0.5f * meso, 0.25f, 1.40f);
		}

		plant_v3_ortho_basis(cur_dir, &U, &V);
		PlantBudId prev_bud_on_unit = PLANT_INVALID_ID;

		/* Limit per-node lateral child spawns on non-whorled trunks so lower nodes never starve the upper crown */
		uint32_t node_children_spawned = 0u;
		uint32_t max_children_this_node = (is_excurrent_spire && order == 0u)
			? 3u
			: ((order == 0u) ? (is_multi_stem ? 1u : (tabular_t > 0.35f ? 3u : 2u)) : 1u);

		for (uint8_t b = 0u; b < buds_here; b++) {
			float az_deg = accum_phyllo_deg + (360.0f / (float)buds_here) * (float)b;
			float az_rad = az_deg * (3.14159265f / 180.0f);
			PlantVec3 radial_dir = plant_v3_norm(
				plant_v3_add(plant_v3_scale(U, cosf(az_rad)), plant_v3_scale(V, sinf(az_rad))));

			/* On excurrent whorled spires, order>=1 lateral branchlets fan out horizontally left/right (pectinate) */
			if (is_excurrent_spire && order >= 1u) {
				PlantVec3 h_side = plant_v3_norm(plant_v3(-cur_dir.y, cur_dir.x, 0.0f));
				float sign = (b & 1u) ? 1.0f : -1.0f;
				radial_dir = plant_v3_norm(plant_v3(h_side.x * sign, h_side.y * sign, 0.02f));
			}

			float ins_deg = plant_sample_distribution(&g->architecture.branch_angle_deg,
				plant_topo_step_lateral(cur_hash, order + 1u, b), PLANT_CH_BRANCH_ANGLE);
			float ins_rad = ins_deg * (3.14159265f / 180.0f);
			PlantVec3 bud_dir = plant_v3_norm(
				plant_v3_add(plant_v3_scale(cur_dir, cosf(ins_rad)),
				             plant_v3_scale(radial_dir, sinf(ins_rad))));

			uint64_t bud_hash = plant_topo_step_lateral(cur_hash, order + 1u, b);
			PlantBudId bid = PLANT_INVALID_ID;
			if (pheno->bud_count < pheno->bud_capacity) {
				bid = pheno->bud_count++;
				PlantBud *bud = &pheno->buds[bid];
				memset(bud, 0, sizeof(*bud));
				bud->id = bid;
				bud->parent_axis = axis_id;
				bud->parent_unit = uid;
				bud->next_on_unit = PLANT_INVALID_ID;
				bud->target_axis_class = (order + 1u < g->architecture.max_branch_order)
					? PLANT_AXIS_LATERAL : PLANT_AXIS_SHORT_SHOOT;
				bud->state = PLANT_BUD_DORMANT;
				bud->fate = PLANT_BUD_VEGETATIVE;
				bud->resulting_axis = PLANT_INVALID_ID;
				bud->activation_probability = plant_clampf(
					g->meristem.lateral_bud_probability * (1.0f - 0.55f * apical_inhib) * pos_weight,
					0.0f, 1.0f);
				bud->birth_day = u->birth_day;
				bud->age_days = fmaxf(0.0f, eff_axis_age_days - (float)(n + 1u) * order_tau);
				bud->position = tip_pos;
				bud->direction = bud_dir;
				bud->vigor = order_vigor * pos_weight;
				bud->inhibitor_level = apical_inhib;
				bud->topo_hash = bud_hash;
				if (u->first_bud == PLANT_INVALID_ID)
					u->first_bud = bid;
				if (prev_bud_on_unit != PLANT_INVALID_ID)
					pheno->buds[prev_bud_on_unit].next_on_unit = bid;
				prev_bud_on_unit = bid;
				u->bud_count++;
			}

			/* Test whether this axillary bud breaks dormancy to spawn a lateral child axis */
			if (order < g->architecture.max_branch_order &&
			    pos_weight > 0.05f &&
			    node_children_spawned < max_children_this_node &&
			    pending_count < max_children_for_axis &&
			    pending_count < 24u) {
				float p_break = g->meristem.lateral_bud_probability *
				                (1.0f - 0.50f * apical_inhib) * pos_weight;
				/* Light-based bud break modulation: shaded buds less likely to break */
				if (inst->light_vox_valid && order >= 1u) {
					enum { LF_N = 12 };
					float r_bound = inst->light_vox_bounds_r;
					float h_bound = inst->light_vox_bounds_h;
					if (r_bound > 0.1f && h_bound > 0.1f) {
						float inv_2r = 1.0f / (2.0f * r_bound);
						float inv_h  = 1.0f / h_bound;
						int ix = (int)floorf(((tip_pos.x + r_bound) * inv_2r) * (float)LF_N);
						int iy = (int)floorf(((tip_pos.y + r_bound) * inv_2r) * (float)LF_N);
						int iz = (int)floorf((tip_pos.z * inv_h) * (float)LF_N);
						if (ix >= 0 && ix < LF_N && iy >= 0 && iy < LF_N && iz >= 0 && iz < LF_N) {
							float loc_l = inst->light_vox_val[iz][iy][ix];
							float shed_thresh = plant_clampf(g->architecture.branch_shed_threshold * 0.38f, 0.10f, 0.34f);
							if (loc_l < shed_thresh) {
								p_break *= plant_clampf(loc_l / fmaxf(shed_thresh, 0.05f), 0.15f, 1.0f);
							}
						}
					}
				}

				/* Guarantee active lateral scaffolds at every crown node on trees */
				if (is_tree_arch && (b == 0u || (tabular_t > 0.35f && order == 0u)))
					p_break = fmaxf(p_break, 0.94f);
				else if (is_excurrent_spire && order == 0u)
					p_break = fmaxf(p_break, 0.90f);

				float draw = plant_hash_u01(bud_hash, PLANT_CH_BUD_BREAK);
				float child_age = eff_axis_age_days - ((float)n + 0.55f) * order_tau;
				if (draw < p_break && child_age > 0.15f * order_tau) {
					if (bid != PLANT_INVALID_ID)
						pheno->buds[bid].state = PLANT_BUD_ACTIVE;
					pending_children[pending_count].parent_uid = uid;
					pending_children[pending_count].bud_id = bid;
					pending_children[pending_count].child_hash = bud_hash;
					pending_children[pending_count].pos = tip_pos;
					pending_children[pending_count].dir = bud_dir;
					pending_children[pending_count].child_age_days = child_age;
					pending_children[pending_count].fol_eta = unit_fol_eta;
					pending_children[pending_count].frt_eta = unit_frt_eta;
					pending_count++;
					node_children_spawned++;
				}
			}
		}

		/* Sympodial apical fork at axis tip:
		 * - On sympodial trunks (order == 0, !is_excurrent_spire), the crown top bifurcates into
		 *   diverging codominant upper boughs rather than ending as a vertical pole.
		 * - On tree scaffold limbs and secondary boughs (order 1 and order 2), the distal tip bifurcates
		 *   fractally into diverging child sub-branches (order + 1). */
		bool should_apical_fork = is_tree_arch &&
		                          (order < g->architecture.max_branch_order) &&
		                          (n + 1u == total_nodes) &&
		                          !is_excurrent_spire;
		if (!should_apical_fork &&
		    order < g->architecture.max_branch_order &&
		    n + 1u == total_nodes &&
		    n >= 1u &&
		    g->architecture.fork_probability > 0.15f) {
			uint64_t ftest = plant_topo_step_lateral(cur_hash, order + 1u, 98u);
			should_apical_fork = (plant_hash_u01(ftest, PLANT_CH_FORK_TEST) < g->architecture.fork_probability);
		}
		if (should_apical_fork &&
		    pending_count < max_children_for_axis &&
		    pending_count < 24u) {
			uint64_t fork_hash = plant_topo_step_lateral(cur_hash, order + 1u, 99u);
			float az_rad = (accum_phyllo_deg + 165.0f) * (3.14159265f / 180.0f);
			PlantVec3 radial_dir = plant_v3_norm(
				plant_v3_add(plant_v3_scale(U, cosf(az_rad)), plant_v3_scale(V, sinf(az_rad))));
			PlantVec3 fork_dir = plant_v3_norm(
				plant_v3_add(plant_v3_scale(cur_dir, 0.70f), plant_v3_scale(radial_dir, 0.71f)));
			float child_age = eff_axis_age_days - ((float)n + 0.35f) * order_tau;
			if (child_age > 0.15f * order_tau) {
				pending_children[pending_count].parent_uid = uid;
				pending_children[pending_count].bud_id = PLANT_INVALID_ID;
				pending_children[pending_count].child_hash = fork_hash;
				pending_children[pending_count].pos = tip_pos;
				pending_children[pending_count].dir = fork_dir;
				pending_children[pending_count].child_age_days = child_age;
				pending_children[pending_count].fol_eta = unit_fol_eta;
				pending_children[pending_count].frt_eta = unit_frt_eta;
				pending_count++;
			}
		}

		/* Organogenesis: emit leaves, flowers, and fruits on foliage-bearing shoots */
		PlantOrganId prev_org_on_unit = PLANT_INVALID_ID;
		bool is_distal_shoot = (order >= g->architecture.max_branch_order - 1u) ||
		                       (g->architecture.max_branch_order <= 1u) ||
		                       (n + 1u == total_nodes && rel_height >= g->architecture.clear_bole_fraction);
		if (is_distal_shoot && unit_fol_eta > 0.15f) {
			float fol_scale = (unit_fol_eta < 1.0f)
				? plant_clampf((unit_fol_eta - 0.15f) / 0.85f, 0.05f, 1.0f)
				: 1.0f;
			uint8_t lcount = g->leaf.leaves_per_node;
			if (lcount == 0u) lcount = 2u;

			for (uint8_t li = 0u; li < lcount; li++) {
				if (pheno->organ_count >= pheno->organ_capacity)
					break;
				uint64_t org_hash = plant_topo_step_organ(cur_hash, PLANT_ORGAN_LEAF, li);
				float az_deg = accum_phyllo_deg + (360.0f / (float)lcount) * (float)li + 32.0f;
				float az_rad = az_deg * (3.14159265f / 180.0f);
				PlantVec3 rad_v = plant_v3_norm(
					plant_v3_add(plant_v3_scale(U, cosf(az_rad)), plant_v3_scale(V, sinf(az_rad))));

				/* Leaf blade direction: spreads outward and slightly skyward (or pendulous if weeping) */
				float pitch_z = (g->architecture.orthotropy < -0.25f) ? -0.55f : 0.25f;
				PlantVec3 ldir = plant_v3_norm(
					plant_v3_add(plant_v3_scale(rad_v, 0.86f),
					             plant_v3_add(plant_v3_scale(cur_dir, 0.35f),
					                          plant_v3(0.0f, 0.0f, pitch_z))));
				PlantVec3 lnorm = plant_v3_norm(plant_v3(rad_v.x * 0.25f, rad_v.y * 0.25f, 0.94f));

				float blen = plant_sample_distribution(&g->leaf.blade_length_m, org_hash, PLANT_CH_LEAF_LEN);
				float wrat = plant_sample_distribution(&g->leaf.width_ratio, org_hash, PLANT_CH_LEAF_WIDTH);
				float lcurv = plant_sample_distribution(&g->leaf.curvature, org_hash, PLANT_CH_LEAF_CURVE);
				float lmat = maturity * fol_scale;
				blen *= (0.55f + 0.45f * lmat);
				float bwid = blen * wrat;

				/* Attach along the upper portion of the internode */
				float t_attach = (lcount > 1u) ? (0.45f + 0.55f * ((float)li / (float)(lcount - 1u))) : 1.0f;
				PlantVec3 apos = plant_v3_add(cur_pos, plant_v3_scale(plant_v3_sub(tip_pos, cur_pos), t_attach));

				PlantOrganId oid = pheno->organ_count++;
				PlantOrgan *org = &pheno->organs[oid];
				memset(org, 0, sizeof(*org));
				org->id = oid;
				org->type = PLANT_ORGAN_LEAF;
				org->parent_unit = uid;
				org->next_on_unit = PLANT_INVALID_ID;
				org->birth_day = u->birth_day;
				org->age_days = fmaxf(0.0f, plant_age_days - u->birth_day);
				org->attachment_angle_rad = az_rad;
				org->attachment_pos = apos;
				org->orientation = plant_quat_from_tangent(ldir, az_rad);
				org->direction = ldir;
				org->normal = lnorm;
				org->length_m = blen;
				org->width_m = bwid;
				org->thickness_m = g->leaf.thickness_mm.mean * 0.001f;
				org->curvature = lcurv;
				org->maturity = lmat;
				org->health = 1.0f;
				/* Light-based leaf health from growth-time Beer-Lambert grid */
				if (inst->light_vox_valid) {
					enum { LF_N = 12 };
					float r_bound = inst->light_vox_bounds_r;
					float h_bound = inst->light_vox_bounds_h;
					if (r_bound > 0.1f && h_bound > 0.1f) {
						float inv_2r = 1.0f / (2.0f * r_bound);
						float inv_h  = 1.0f / h_bound;
						int ix = (int)floorf(((org->attachment_pos.x + r_bound) * inv_2r) * (float)LF_N);
						int iy = (int)floorf(((org->attachment_pos.y + r_bound) * inv_2r) * (float)LF_N);
						int iz = (int)floorf((org->attachment_pos.z * inv_h) * (float)LF_N);
						if (ix >= 0 && ix < LF_N && iy >= 0 && iy < LF_N && iz >= 0 && iz < LF_N) {
							float loc_l = inst->light_vox_val[iz][iy][ix];
							org->health = plant_clampf(0.25f + 0.75f * loc_l, 0.20f, 1.0f);
						}
					}
				}
				org->branch_order = order;
				org->sub_index = li;
				org->topo_hash = org_hash;
				if (u->first_organ == PLANT_INVALID_ID)
					u->first_organ = oid;
				if (prev_org_on_unit != PLANT_INVALID_ID)
					pheno->organs[prev_org_on_unit].next_on_unit = oid;
				prev_org_on_unit = oid;
				u->organ_count++;

				float area = 0.72f * blen * bwid;
				u->leaf_area_m2 += area;
				pheno->total_leaf_area_m2 += area;
			}
		}

		/* Reproductive Organogenesis (Flowers & Fruits/Cones) on mature terminal units */
		float age_maturity_ratio = plant_age_days / fmaxf(30.0f, g->physiology.max_age_years * 365.0f * 0.15f);
		if (n + 1u == total_nodes &&
		    (order >= g->architecture.max_branch_order - 1u || g->architecture.max_branch_order <= 1u) &&
		    age_maturity_ratio >= g->flower.bloom_onset_maturity &&
		    unit_frt_eta > 0.20f) {
			float frt_scale = (unit_frt_eta < 1.0f)
				? plant_clampf((unit_frt_eta - 0.20f) / 0.80f, 0.10f, 1.0f)
				: 1.0f;

			if (g->flower.has_flowers && pheno->organ_count < pheno->organ_capacity) {
				uint64_t fl_hash = plant_topo_step_organ(cur_hash, PLANT_ORGAN_FLOWER, 0u);
				if (plant_hash_u01(fl_hash, PLANT_CH_FLOWER_FATE) <= g->meristem.flower_bud_probability + 0.45f) {
					float frad = plant_sample_distribution(&g->flower.flower_radius_m, fl_hash, PLANT_CH_FLOWER_RAD) * frt_scale;
					PlantOrganId oid = pheno->organ_count++;
					PlantOrgan *org = &pheno->organs[oid];
					memset(org, 0, sizeof(*org));
					org->id = oid;
					org->type = PLANT_ORGAN_FLOWER;
					org->parent_unit = uid;
					org->next_on_unit = PLANT_INVALID_ID;
					org->birth_day = u->birth_day;
					org->age_days = fmaxf(0.0f, plant_age_days - u->birth_day);
					org->attachment_pos = tip_pos;
					org->orientation = u->orientation;
					org->direction = plant_v3_norm(plant_v3(cur_dir.x * 0.4f, cur_dir.y * 0.4f, 0.92f));
					org->normal = org->direction;
					org->length_m = frad * 2.0f;
					org->width_m = frad * 2.0f;
					org->thickness_m = g->flower.corolla_depth_m.mean;
					org->maturity = maturity * frt_scale;
					org->health = 1.0f;
				/* Light-based leaf health from growth-time Beer-Lambert grid */
				if (inst->light_vox_valid) {
					enum { LF_N = 12 };
					float r_bound = inst->light_vox_bounds_r;
					float h_bound = inst->light_vox_bounds_h;
					if (r_bound > 0.1f && h_bound > 0.1f) {
						float inv_2r = 1.0f / (2.0f * r_bound);
						float inv_h  = 1.0f / h_bound;
						int ix = (int)floorf(((org->attachment_pos.x + r_bound) * inv_2r) * (float)LF_N);
						int iy = (int)floorf(((org->attachment_pos.y + r_bound) * inv_2r) * (float)LF_N);
						int iz = (int)floorf((org->attachment_pos.z * inv_h) * (float)LF_N);
						if (ix >= 0 && ix < LF_N && iy >= 0 && iy < LF_N && iz >= 0 && iz < LF_N) {
							float loc_l = inst->light_vox_val[iz][iy][ix];
							org->health = plant_clampf(0.25f + 0.75f * loc_l, 0.20f, 1.0f);
						}
					}
				}
					org->branch_order = order;
					org->topo_hash = fl_hash;
					if (u->first_organ == PLANT_INVALID_ID)
						u->first_organ = oid;
					if (prev_org_on_unit != PLANT_INVALID_ID)
						pheno->organs[prev_org_on_unit].next_on_unit = oid;
					prev_org_on_unit = oid;
					u->organ_count++;
				}
			}

			if (g->fruit.fruit_type != PLANT_FRUIT_NONE && pheno->organ_count < pheno->organ_capacity) {
				uint64_t fr_hash = plant_topo_step_organ(cur_hash, PLANT_ORGAN_FRUIT, 0u);
				if (plant_hash_u01(fr_hash, PLANT_CH_FLOWER_FATE) <= g->meristem.flower_bud_probability + 0.40f) {
					float flen = plant_sample_distribution(&g->fruit.length_m, fr_hash, PLANT_CH_FRUIT_LEN) * frt_scale;
					float fdia = plant_sample_distribution(&g->fruit.diameter_m, fr_hash, PLANT_CH_FRUIT_DIAM) * frt_scale;
					PlantOrganId oid = pheno->organ_count++;
					PlantOrgan *org = &pheno->organs[oid];
					memset(org, 0, sizeof(*org));
					org->id = oid;
					org->type = PLANT_ORGAN_FRUIT;
					org->parent_unit = uid;
					org->next_on_unit = PLANT_INVALID_ID;
					org->birth_day = u->birth_day;
					org->age_days = fmaxf(0.0f, plant_age_days - u->birth_day);
					org->attachment_pos = tip_pos;
					org->orientation = u->orientation;
					org->direction = (g->fruit.fruit_type == PLANT_FRUIT_CONE || g->fruit.fruit_type == PLANT_FRUIT_ACHENE)
						? plant_v3(0.0f, 0.0f, 1.0f)
						: plant_v3(0.0f, 0.0f, -1.0f);
					org->normal = plant_v3(0.0f, 0.0f, 1.0f);
					org->length_m = flen;
					org->width_m = fdia;
					org->thickness_m = fdia;
					org->maturity = maturity * frt_scale;
					org->health = 1.0f;
				/* Light-based leaf health from growth-time Beer-Lambert grid */
				if (inst->light_vox_valid) {
					enum { LF_N = 12 };
					float r_bound = inst->light_vox_bounds_r;
					float h_bound = inst->light_vox_bounds_h;
					if (r_bound > 0.1f && h_bound > 0.1f) {
						float inv_2r = 1.0f / (2.0f * r_bound);
						float inv_h  = 1.0f / h_bound;
						int ix = (int)floorf(((org->attachment_pos.x + r_bound) * inv_2r) * (float)LF_N);
						int iy = (int)floorf(((org->attachment_pos.y + r_bound) * inv_2r) * (float)LF_N);
						int iz = (int)floorf((org->attachment_pos.z * inv_h) * (float)LF_N);
						if (ix >= 0 && ix < LF_N && iy >= 0 && iy < LF_N && iz >= 0 && iz < LF_N) {
							float loc_l = inst->light_vox_val[iz][iy][ix];
							org->health = plant_clampf(0.25f + 0.75f * loc_l, 0.20f, 1.0f);
						}
					}
				}
					org->branch_order = order;
					org->topo_hash = fr_hash;
					if (u->first_organ == PLANT_INVALID_ID)
						u->first_organ = oid;
					if (prev_org_on_unit != PLANT_INVALID_ID)
						pheno->organs[prev_org_on_unit].next_on_unit = oid;
					prev_org_on_unit = oid;
					u->organ_count++;
				}
			}
		}

		cur_pos = tip_pos;
		prev_uid = uid;
	}

	/* Now expand all activated lateral child axes in deterministic order */
	for (uint32_t c = 0u; c < pending_count; c++) {
		plant_sim_develop_axis_recursive(
			inst, env,
			plant_age_days,
			pending_children[c].child_age_days,
			tau_plastochron,
			axis_id,
			pending_children[c].parent_uid,
			pending_children[c].bud_id,
			(uint8_t)(order + 1u),
			pending_children[c].child_hash,
			pending_children[c].pos,
			pending_children[c].dir,
			pending_children[c].fol_eta,
			pending_children[c].frt_eta,
			deltas, delta_count);
	}
}

/* Basipetal Da Vinci Pipe-Model Secondary Growth Pass:
 * Sweeps in reverse topological order (from terminal twigs down to root collar)
 * so every parent internode's cross-sectional capacity satisfies:
 *   r_parent^alpha = r_next_in_axis^alpha + sum(r_lateral_child^alpha) + taper_term */
static inline void
plant_sim_solve_pipe_model_allometry(PlantInstance *inst)
{
	PlantPhenotype *pheno = &inst->phenotype;
	const PlantGenome *g = &inst->genome;
	if (pheno->unit_count == 0u)
		return;

	float alpha = plant_clampf(g->allometry.pipe_model_exponent, 1.40f, 3.00f);
	float inv_alpha = 1.0f / alpha;
	float r_min = fmaxf(0.003f, g->allometry.tip_min_radius_m);
	float pipe_sum[PLANT_MAX_UNITS];
	for (uint32_t i = 0u; i < pheno->unit_count && i < PLANT_MAX_UNITS; i++)
		pipe_sum[i] = 0.0f;

	/* Reverse traversal: child units always have higher index than their parent_unit */
	for (int32_t i = (int32_t)pheno->unit_count - 1; i >= 0; i--) {
		PlantGrowthUnit *u = &pheno->units[i];
		float tip_r_pow = powf(r_min * (0.65f + 0.35f * u->maturity), alpha);
		float leaf_pipe = powf(r_min * sqrtf(1.0f + u->leaf_area_m2 * 12.0f), alpha);

		float downstream_pow = pipe_sum[i];
		if (downstream_pow < tip_r_pow)
			downstream_pow = tip_r_pow;
		if (u->leaf_area_m2 > 0.0f && downstream_pow < leaf_pipe)
			downstream_pow = leaf_pipe;

		float tip_r = powf(downstream_pow, inv_alpha);
		float taper_add = powf(g->allometry.radius_taper * u->length_m + r_min * 0.25f, alpha);
		float base_pow = downstream_pow + taper_add;
		float base_r = powf(base_pow, inv_alpha) * (1.0f + g->allometry.bark_thickness_ratio);

		/* Basal root-collar buttress flare on primary stem base */
		if (u->branch_order == 0u && u->node_index == 0u) {
			base_r *= (1.0f + 0.45f * g->allometry.root_flare_strength);
		}

		u->tip_radius_m = tip_r;
		u->base_radius_m = base_r;
		u->radius_m = 0.5f * (base_r + tip_r);

		/* Propagate cross-sectional area pipe capacity & leaf area basipetally to parent_unit */
		if (u->parent_unit != PLANT_INVALID_ID && u->parent_unit < (uint32_t)i) {
			pipe_sum[u->parent_unit] += base_pow;
			pheno->units[u->parent_unit].leaf_area_m2 += u->leaf_area_m2;
		}
	}

	/* Record axis base radii & maximum trunk base radius */
	pheno->trunk_base_radius_m = 0.0f;
	for (uint32_t a = 0u; a < pheno->axis_count; a++) {
		PlantAxis *ax = &pheno->axes[a];
		if (ax->first_unit != PLANT_INVALID_ID && ax->first_unit < pheno->unit_count) {
			ax->base_radius_m = pheno->units[ax->first_unit].base_radius_m;
			if (ax->order == 0u && !ax->is_root_axis && ax->base_radius_m > pheno->trunk_base_radius_m)
				pheno->trunk_base_radius_m = ax->base_radius_m;
		}
	}
}

/* Build/update the 12^3 Beer-Lambert light competition voxel grid for growth-time phototropic steering
 * and self-pruning. Runs in pass-1 coordinates inside plant_simulate_at_time so evaluation stays
 * strictly chrono-deterministic and dt-invariant. */
static inline void
plant_build_light_voxel_grid(PlantInstance *plant, const PlantEnvironment *env)
{
	if (!plant) return;
	plant->light_vox_valid = false;
	if (plant->phenotype.organ_count < 6u || plant->phenotype.height_m < 0.5f)
		return;
	if (plant->genome.architecture.max_branch_order < 2u)
		return;

	enum { LF_N = 12 };
	memset(plant->light_vox_lai, 0, sizeof(plant->light_vox_lai));

	float r_bound = fmaxf(0.5f, plant->phenotype.crown_radius_m * 1.08f);
	float h_bound = fmaxf(0.5f, plant->phenotype.height_m * 1.05f);
	float inv_2r  = 1.0f / (2.0f * r_bound);
	float inv_h   = 1.0f / h_bound;

	for (uint32_t i = 0u; i < plant->phenotype.organ_count; i++) {
		const PlantOrgan *o = &plant->phenotype.organs[i];
		if (o->type != PLANT_ORGAN_LEAF) continue;
		int ix = (int)floorf(((o->attachment_pos.x + r_bound) * inv_2r) * (float)LF_N);
		int iy = (int)floorf(((o->attachment_pos.y + r_bound) * inv_2r) * (float)LF_N);
		int iz = (int)floorf((o->attachment_pos.z * inv_h) * (float)LF_N);
		if (ix < 0) ix = 0; else if (ix >= LF_N) ix = LF_N - 1;
		if (iy < 0) iy = 0; else if (iy >= LF_N) iy = LF_N - 1;
		if (iz < 0) iz = 0; else if (iz >= LF_N) iz = LF_N - 1;
		plant->light_vox_lai[iz][iy][ix] += o->length_m * o->width_m * 0.72f;
	}

	float porosity = plant_clampf(plant->genome.leaf.canopy_porosity, 0.15f, 0.92f);
	float cell_w = (2.0f * r_bound) / (float)LF_N;
	float k_ext = (1.15f - 0.75f * porosity) / fmaxf(1e-4f, cell_w * cell_w);
	float base_sun = plant_clampf(env->sunlight, 0.25f, 1.25f);

	for (int iy = 0; iy < LF_N; iy++) {
		for (int ix = 0; ix < LF_N; ix++) {
			float accum_tau = 0.0f;
			for (int iz = LF_N - 1; iz >= 0; iz--) {
				float fx = ((float)ix + 0.5f) / (float)LF_N * 2.0f - 1.0f;
				float fy = ((float)iy + 0.5f) / (float)LF_N * 2.0f - 1.0f;
				float r_norm = sqrtf(fx * fx + fy * fy);
				float sky_side = plant_clampf(r_norm * 0.48f, 0.0f, 0.45f);
				float direct_l = base_sun * expf(-accum_tau * 0.32f);
				plant->light_vox_val[iz][iy][ix] = plant_clampf(direct_l * (1.0f - sky_side) + base_sun * sky_side, 0.08f, 1.25f);
				accum_tau += plant->light_vox_lai[iz][iy][ix] * k_ext;
			}
		}
	}

	plant->light_vox_bounds_r = r_bound;
	plant->light_vox_bounds_h = h_bound;
	plant->light_vox_valid = true;
}

/* Primary Chrono-Deterministic Evaluator:
 * Evaluates a plant's exact PlantPhenotype at age `target_age_days` given optional
 * ephemeral delta records `deltas[0..delta_count-1]`.
 *
 * Because all stochastic draws are coordinate-addressed by `topo_hash`, calling
 * `plant_simulate_at_time(inst, env, t0 + dt, ...)` in a single jump produces the
 * exact same phenotype graph as stepping incrementally! */
static inline void
plant_simulate_at_time(PlantInstance *plant,
                       const PlantEnvironment *env,
                       float target_age_days,
                       const PlantDeltaRecord *deltas,
                       uint32_t delta_count)
{
	if (!plant) return;
	PlantEnvironment default_env = plant_environment_default();
	if (!env) env = &default_env;

	plant->age_days = fmaxf(0.0f, target_age_days);
	plant->light_vox_valid = false;
	plant_phenotype_reset(&plant->phenotype);
	if (plant->age_days <= 1e-4f || !plant->phenotype.axes || !plant->phenotype.units)
		return;

	/* Check whole-plant felling delta (target_topo_hash == 0) */
	float root_prune_eta = 1.0f, root_fol_eta = 1.0f, root_frt_eta = 1.0f;
	plant_eval_node_deltas(0ULL, plant->age_days, deltas, delta_count,
	                       &root_prune_eta, &root_fol_eta, &root_frt_eta);
	float eff_plant_age = plant->age_days * root_prune_eta;
	if (eff_plant_age <= 1e-4f)
		return;

	float v_env = plant_eval_env_vigor(&plant->genome, env);
	float tau_eff = plant->genome.meristem.plastochron_days / fmaxf(0.18f, v_env);
	uint8_t stem_count = plant->genome.architecture.basal_stem_count;
	if (stem_count == 0u) stem_count = 1u;
	if (stem_count > 8u)  stem_count = 8u;

	uint64_t root_seed = plant_mix64(plant->genome.genetic_seed ^ PLANT_TOPO_ROOT_SALT);

	int max_passes = (plant->genome.architecture.max_branch_order >= 2u) ? 2 : 1;
	for (int pass = 0; pass < max_passes; pass++) {
		if (pass == 1) {
			plant_build_light_voxel_grid(plant, env);
			if (!plant->light_vox_valid)
				break;
			plant_phenotype_reset(&plant->phenotype);
		}
		for (uint8_t s = 0u; s < stem_count; s++) {
			uint64_t stem_hash = plant_topo_step_lateral(root_seed, 0u, s);
			PlantVec3 base_pos = plant_v3(0.0f, 0.0f, 0.0f);
			PlantVec3 base_dir = plant_v3(0.0f, 0.0f, 1.0f);

			if (stem_count > 1u) {
				float ang = (6.2831853f * (float)s) / (float)stem_count +
				            0.25f * plant_hash_normal01(stem_hash, PLANT_CH_ROLL_VAR);
				float spread_r = 0.04f + 0.02f * (float)stem_count;
				base_pos = plant_v3(cosf(ang) * spread_r, sinf(ang) * spread_r, 0.0f);
				float tilt = 0.14f + 0.04f * (float)s;
				base_dir = plant_v3_norm(plant_v3(cosf(ang) * tilt, sinf(ang) * tilt, 1.0f));
			}

			/* Multi-stem clumps stagger secondary culm emergence slightly */
			float stem_age = eff_plant_age * (1.0f - 0.06f * (float)s);
			if (stem_age > 0.0f) {
				plant_sim_develop_axis_recursive(
					plant, env,
					plant->age_days,
					stem_age,
					tau_eff,
					PLANT_INVALID_ID,
					PLANT_INVALID_ID,
					PLANT_INVALID_ID,
					0u,
					stem_hash,
					base_pos,
					base_dir,
					root_fol_eta,
					root_frt_eta,
					deltas, delta_count);
			}
		}
	}
	plant->light_vox_valid = false;

	/* Solve basipetal Da Vinci pipe-model vascular radii across the developed MTG */
	plant_sim_solve_pipe_model_allometry(plant);

	/* Normalize macroscopic height to target species envelope at current age maturity */
	float max_days = fmaxf(60.0f, plant->genome.meristem.plastochron_days * 12.0f);
	float mat_frac = plant_clampf(eff_plant_age / max_days, 0.12f, 1.0f);
	float target_h = plant->genome.physiology.mature_height_m * (0.35f + 0.65f * sqrtf(mat_frac));
	if (plant->phenotype.height_m > 1e-4f && target_h > 1e-4f) {
		float scale = target_h / plant->phenotype.height_m;
		/* Scale all spatial positions, lengths, and radii consistently */
		for (uint32_t i = 0u; i < plant->phenotype.unit_count; i++) {
			PlantGrowthUnit *u = &plant->phenotype.units[i];
			u->base_pos = plant_v3_scale(u->base_pos, scale);
			u->tip_pos  = plant_v3_scale(u->tip_pos, scale);
			u->length_m *= scale;
			u->base_radius_m *= scale;
			u->tip_radius_m  *= scale;
			u->radius_m      *= scale;
			u->leaf_area_m2  *= (scale * scale);
		}
		for (uint32_t i = 0u; i < plant->phenotype.bud_count; i++) {
			plant->phenotype.buds[i].position = plant_v3_scale(plant->phenotype.buds[i].position, scale);
		}
		for (uint32_t i = 0u; i < plant->phenotype.organ_count; i++) {
			PlantOrgan *o = &plant->phenotype.organs[i];
			o->attachment_pos = plant_v3_scale(o->attachment_pos, scale);
			o->length_m    *= scale;
			o->width_m     *= scale;
			o->thickness_m *= scale;
		}
		for (uint32_t i = 0u; i < plant->phenotype.axis_count; i++) {
			plant->phenotype.axes[i].length_m      *= scale;
			plant->phenotype.axes[i].base_radius_m *= scale;
		}
		plant->phenotype.height_m            *= scale;
		plant->phenotype.crown_radius_m      *= scale;
		plant->phenotype.trunk_base_radius_m *= scale;
		plant->phenotype.total_leaf_area_m2  *= (scale * scale);
	}

	/* 3D Intra-Crown Beer-Lambert Light Competition, Phototropic Foraging & Self-Pruning Pass:
	 * Builds a 12x12x12 3D canopy leaf-area voxel grid over the crown envelope, integrates downward
	 * and sunward optical depth (modulated by genome.architecture.canopy_porosity), and:
	 *   1. Bends shaded lateral shoots outward along the local light gradient (phototropism_strength + shade_avoidance),
	 *   2. Attenuates or self-prunes deep interior/lower shaded leaves where light < branch_shed_threshold
	 *      while preserving the outer sunlit shell, creating authentic hollow SpeedTree/Skyrim sub-crowns. */
	if (plant->genome.architecture.max_branch_order >= 2u &&
	    plant->phenotype.organ_count > 6u &&
	    plant->phenotype.height_m > 0.5f) {
		enum { LF_N = 12 };
		float vox_lai[LF_N][LF_N][LF_N];
		float vox_light[LF_N][LF_N][LF_N];
		memset(vox_lai, 0, sizeof(vox_lai));

		float r_bound = fmaxf(0.5f, plant->phenotype.crown_radius_m * 1.08f);
		float h_bound = fmaxf(0.5f, plant->phenotype.height_m * 1.05f);
		float inv_2r  = 1.0f / (2.0f * r_bound);
		float inv_h   = 1.0f / h_bound;

		for (uint32_t i = 0u; i < plant->phenotype.organ_count; i++) {
			const PlantOrgan *o = &plant->phenotype.organs[i];
			if (o->type != PLANT_ORGAN_LEAF) continue;
			int ix = (int)floorf(((o->attachment_pos.x + r_bound) * inv_2r) * (float)LF_N);
			int iy = (int)floorf(((o->attachment_pos.y + r_bound) * inv_2r) * (float)LF_N);
			int iz = (int)floorf((o->attachment_pos.z * inv_h) * (float)LF_N);
			if (ix < 0) ix = 0; else if (ix >= LF_N) ix = LF_N - 1;
			if (iy < 0) iy = 0; else if (iy >= LF_N) iy = LF_N - 1;
			if (iz < 0) iz = 0; else if (iz >= LF_N) iz = LF_N - 1;
			vox_lai[iz][iy][ix] += o->length_m * o->width_m * 0.72f;
		}

		float porosity = plant_clampf(plant->genome.leaf.canopy_porosity, 0.15f, 0.92f);
		float k_ext = (1.15f - 0.75f * porosity) / fmaxf(0.04f, (2.0f * r_bound / (float)LF_N) * (2.0f * r_bound / (float)LF_N));
		float base_sun = plant_clampf(env->sunlight, 0.25f, 1.25f);

		for (int iy = 0; iy < LF_N; iy++) {
			for (int ix = 0; ix < LF_N; ix++) {
				float accum_tau = 0.0f;
				for (int iz = LF_N - 1; iz >= 0; iz--) {
					/* Include partial lateral sky light near crown periphery */
					float fx = ((float)ix + 0.5f) / (float)LF_N * 2.0f - 1.0f;
					float fy = ((float)iy + 0.5f) / (float)LF_N * 2.0f - 1.0f;
					float r_norm = sqrtf(fx * fx + fy * fy);
					float sky_side = plant_clampf(r_norm * 0.48f, 0.0f, 0.45f);
					float direct_l = base_sun * expf(-accum_tau * 0.32f);
					vox_light[iz][iy][ix] = plant_clampf(direct_l * (1.0f - sky_side) + base_sun * sky_side, 0.08f, 1.25f);
					accum_tau += vox_lai[iz][iy][ix] * k_ext;
				}
			}
		}

		float shed_thresh = plant_clampf(plant->genome.architecture.branch_shed_threshold * 0.38f, 0.10f, 0.34f);
		float photo_str = plant_clampf(plant->genome.plasticity.phototropism_strength * 0.5f +
		                               plant->genome.plasticity.shade_avoidance * 0.5f, 0.0f, 1.0f);
		plant->phenotype.total_leaf_area_m2 = 0.0f;

		for (uint32_t i = 0u; i < plant->phenotype.organ_count; i++) {
			PlantOrgan *o = &plant->phenotype.organs[i];
			if (o->type != PLANT_ORGAN_LEAF) continue;
			int ix = (int)floorf(((o->attachment_pos.x + r_bound) * inv_2r) * (float)LF_N);
			int iy = (int)floorf(((o->attachment_pos.y + r_bound) * inv_2r) * (float)LF_N);
			int iz = (int)floorf((o->attachment_pos.z * inv_h) * (float)LF_N);
			if (ix < 0) ix = 0; else if (ix >= LF_N) ix = LF_N - 1;
			if (iy < 0) iy = 0; else if (iy >= LF_N) iy = LF_N - 1;
			if (iz < 0) iz = 0; else if (iz >= LF_N) iz = LF_N - 1;

			float loc_l = vox_light[iz][iy][ix];
			int ix_p = (ix + 1 < LF_N) ? (ix + 1) : ix;
			int ix_m = (ix > 0) ? (ix - 1) : ix;
			int iy_p = (iy + 1 < LF_N) ? (iy + 1) : iy;
			int iy_m = (iy > 0) ? (iy - 1) : iy;
			int iz_p = (iz + 1 < LF_N) ? (iz + 1) : iz;
			float gx = vox_light[iz][iy][ix_p] - vox_light[iz][iy][ix_m];
			float gy = vox_light[iz][iy_p][ix] - vox_light[iz][iy_m][ix];
			float gz = vox_light[iz_p][iy][ix] - loc_l;

			/* Phototropic leaf & shoot orientation foraging toward local open-sky light gradient */
			if (gx * gx + gy * gy + gz * gz > 1e-4f) {
				PlantVec3 grad = plant_v3_norm(plant_v3(gx, gy, fmaxf(0.0f, gz)));
				o->direction = plant_v3_norm(plant_v3_add(o->direction, plant_v3_scale(grad, 0.30f * photo_str)));
			}

			/* Self-shading vigor modulation: sunlit outer crown flushes fuller; deep shaded interior thins */
			float light_vigor = plant_clampf((loc_l - shed_thresh * 0.45f) / fmaxf(0.15f, 0.82f - shed_thresh * 0.45f), 0.35f, 1.12f);
			o->health = plant_clampf(0.30f + 0.70f * loc_l, 0.25f, 1.0f);
			o->length_m *= (0.78f + 0.24f * light_vigor);
			o->width_m  *= (0.78f + 0.24f * light_vigor);
			plant->phenotype.total_leaf_area_m2 += 0.72f * o->length_m * o->width_m;
		}
	}

	/* Compute physiological state metrics */
	float wood_vol = 0.0f;
	for (uint32_t i = 0u; i < plant->phenotype.unit_count; i++) {
		const PlantGrowthUnit *u = &plant->phenotype.units[i];
		wood_vol += 3.14159265f * u->radius_m * u->radius_m * u->length_m;
	}
	plant->biomass_kg = wood_vol * plant->genome.allometry.wood_density * 1000.0f +
	                    plant->phenotype.total_leaf_area_m2 * 0.12f;
	plant->carbohydrate_reserve = plant_clampf(0.25f + 0.55f * v_env, 0.05f, 1.0f);
	plant->carbon_reserves_g = plant->biomass_kg * 1000.0f * plant->carbohydrate_reserve * 0.15f;
	plant->water_potential = plant_clampf(env->soil_moisture - 0.15f, -2.5f, 0.0f);
	plant->water_status = plant_clampf(env->soil_moisture, 0.0f, 1.0f);
	plant->stress_index = plant_clampf(1.0f - v_env, 0.0f, 1.0f);
	float gdd_per_day = fmaxf(0.0f, env->temperature_c - plant->genome.phenology.base_temperature_C);
	plant->accumulated_degree_days = plant->age_days * gdd_per_day;
}

/* Incremental step wrapper: advances `plant->age_days` by `dt_days` and evaluates
 * the chrono-deterministic phenotype. Because evaluation is coordinate-addressed,
 * N calls of `plant_simulate_step(p, env, dt)` are bit-identical to one call of
 * `plant_simulate_step(p, env, N * dt)`! */
static inline void
plant_simulate_step(PlantInstance *plant,
                    const PlantEnvironment *env,
                    float dt_days)
{
	if (!plant) return;
	float new_age = fmaxf(0.0f, plant->age_days + dt_days);
	plant_simulate_at_time(plant, env, new_age, NULL, 0u);
}

/* IDEA.md §7 & §11 canonical spatial-sampler step function */
static inline PlantStatus
plant_simulate_step_sampler(PlantInstance *plant,
                            PlantEnvironmentSampler environment,
                            void *environment_context,
                            float time_days,
                            float delta_days)
{
	if (!plant)
		return PLANT_ERR_NULL_PTR;
	float target_age = (time_days > plant->age_days)
		? (time_days + delta_days)
		: fmaxf(0.0f, plant->age_days + delta_days);
	PlantEnvironment sampled = environment
		? environment(environment_context, plant_v3(0.0f, 0.0f, 0.0f), target_age)
		: plant_environment_default();
	plant_simulate_at_time(plant, &sampled, target_age, NULL, 0u);
	return PLANT_OK;
}

/* IDEA.md §11 Instance Lifecycle API (zero-import static storage pool fallback) */
static inline PlantStatus
plant_instance_init(PlantInstance *plant, const PlantGenome *genome)
{
	if (!plant || !genome)
		return PLANT_ERR_NULL_PTR;
	static PlantPhenotypeStorage s_default_arenas[4];
	static uint32_t s_arena_cursor = 0u;

	PlantAxis       *existing_axes   = plant->phenotype.axes;
	uint32_t         existing_ax_cap = plant->phenotype.axis_capacity;
	PlantGrowthUnit *existing_units  = plant->phenotype.units;
	uint32_t         existing_un_cap = plant->phenotype.unit_capacity;
	PlantBud        *existing_buds   = plant->phenotype.buds;
	uint32_t         existing_bd_cap = plant->phenotype.bud_capacity;
	PlantOrgan      *existing_orgs   = plant->phenotype.organs;
	uint32_t         existing_or_cap = plant->phenotype.organ_capacity;

	memset(plant, 0, sizeof(*plant));
	plant->instance_id = genome->individual_id ? genome->individual_id : genome->genetic_seed;
	plant->genome = *genome;
	plant->simulation_seed = genome->genetic_seed;
	plant->water_status = 1.0f;

	if (existing_axes && existing_units && existing_buds && existing_orgs) {
		plant->phenotype.axes           = existing_axes;
		plant->phenotype.axis_capacity  = existing_ax_cap;
		plant->phenotype.units          = existing_units;
		plant->phenotype.unit_capacity  = existing_un_cap;
		plant->phenotype.buds           = existing_buds;
		plant->phenotype.bud_capacity   = existing_bd_cap;
		plant->phenotype.organs         = existing_orgs;
		plant->phenotype.organ_capacity = existing_or_cap;
	} else {
		PlantPhenotypeStorage *arena = &s_default_arenas[s_arena_cursor++ & 3u];
		plant_phenotype_bind_storage(&plant->phenotype, arena);
	}
	return PLANT_OK;
}

static inline void
plant_instance_destroy(PlantInstance *plant)
{
	if (!plant) return;
	plant_phenotype_reset(&plant->phenotype);
	plant->phenotype.axes   = NULL;
	plant->phenotype.units  = NULL;
	plant->phenotype.buds   = NULL;
	plant->phenotype.organs = NULL;
}

/* ============================================================================
 * 11. Explicit Portable Serialization & Schema Versioning/Migration (IDEA.md §9)
 * ============================================================================ */

#define PLANT_GENOME_WIRE_MAGIC 0x4E445047u /* 'NDPG' */
#define PLANT_PHENO_WIRE_MAGIC  0x4E445050u /* 'NDPP' */

static inline void
plant_wire_put_u32(uint8_t *buf, size_t *off, uint32_t v)
{
	buf[(*off)++] = (uint8_t)(v & 0xFFu);
	buf[(*off)++] = (uint8_t)((v >> 8) & 0xFFu);
	buf[(*off)++] = (uint8_t)((v >> 16) & 0xFFu);
	buf[(*off)++] = (uint8_t)((v >> 24) & 0xFFu);
}

static inline uint32_t
plant_wire_get_u32(const uint8_t *buf, size_t *off)
{
	uint32_t v = (uint32_t)buf[(*off)] |
	             ((uint32_t)buf[(*off) + 1] << 8) |
	             ((uint32_t)buf[(*off) + 2] << 16) |
	             ((uint32_t)buf[(*off) + 3] << 24);
	*off += 4u;
	return v;
}

static inline void
plant_wire_put_u64(uint8_t *buf, size_t *off, uint64_t v)
{
	plant_wire_put_u32(buf, off, (uint32_t)(v & 0xFFFFFFFFULL));
	plant_wire_put_u32(buf, off, (uint32_t)(v >> 32));
}

static inline uint64_t
plant_wire_get_u64(const uint8_t *buf, size_t *off)
{
	uint64_t lo = plant_wire_get_u32(buf, off);
	uint64_t hi = plant_wire_get_u32(buf, off);
	return lo | (hi << 32);
}

static inline void
plant_wire_put_f32(uint8_t *buf, size_t *off, float f)
{
	uint32_t bits = 0u;
	memcpy(&bits, &f, sizeof(bits));
	plant_wire_put_u32(buf, off, bits);
}

static inline float
plant_wire_get_f32(const uint8_t *buf, size_t *off)
{
	uint32_t bits = plant_wire_get_u32(buf, off);
	float f = 0.0f;
	memcpy(&f, &bits, sizeof(f));
	return f;
}

static inline void
plant_wire_put_dist(uint8_t *buf, size_t *off, const PlantDistribution *d)
{
	plant_wire_put_f32(buf, off, d->mean);
	plant_wire_put_f32(buf, off, d->sigma);
	plant_wire_put_f32(buf, off, d->min);
	plant_wire_put_f32(buf, off, d->max);
}

static inline PlantDistribution
plant_wire_get_dist(const uint8_t *buf, size_t *off)
{
	PlantDistribution d;
	d.mean  = plant_wire_get_f32(buf, off);
	d.sigma = plant_wire_get_f32(buf, off);
	d.min   = plant_wire_get_f32(buf, off);
	d.max   = plant_wire_get_f32(buf, off);
	return d;
}

/* Explicit field-by-field binary serialization of PlantGenome (no raw struct dump) */
static inline PlantStatus
plant_genome_serialize(const PlantGenome *g, uint8_t *buf, size_t cap, size_t *out_len)
{
	const size_t needed = 192u;
	if (out_len) *out_len = needed;
	if (!g) return PLANT_ERR_NULL_PTR;
	if (!buf || cap < needed) return PLANT_ERR_SERIALIZE_BUFFER;

	size_t off = 0u;
	plant_wire_put_u32(buf, &off, PLANT_GENOME_WIRE_MAGIC);
	plant_wire_put_u32(buf, &off, g->schema_version ? g->schema_version : PLANT_GENOME_SCHEMA_VERSION);
	plant_wire_put_u32(buf, &off, g->species_id);
	plant_wire_put_u64(buf, &off, g->individual_id);
	plant_wire_put_u64(buf, &off, g->parent_id[0]);
	plant_wire_put_u64(buf, &off, g->parent_id[1]);
	plant_wire_put_u64(buf, &off, g->genetic_seed);

	plant_wire_put_dist(buf, &off, &g->architecture.internode_length_m);
	plant_wire_put_dist(buf, &off, &g->architecture.branch_angle_deg);
	plant_wire_put_f32(buf, &off, g->architecture.apical_dominance);
	plant_wire_put_f32(buf, &off, g->architecture.orthotropy);
	plant_wire_put_f32(buf, &off, g->architecture.tortuosity);
	plant_wire_put_f32(buf, &off, g->allometry.pipe_model_exponent);
	plant_wire_put_f32(buf, &off, g->allometry.root_flare_strength);
	plant_wire_put_dist(buf, &off, &g->leaf.blade_length_m);
	plant_wire_put_dist(buf, &off, &g->leaf.width_ratio);
	plant_wire_put_dist(buf, &off, &g->flower.flower_radius_m);
	plant_wire_put_dist(buf, &off, &g->fruit.length_m);
	plant_wire_put_dist(buf, &off, &g->fruit.diameter_m);
	plant_wire_put_f32(buf, &off, g->physiology.growth_rate);
	plant_wire_put_f32(buf, &off, g->physiology.mature_height_m);
	plant_wire_put_f32(buf, &off, g->physiology.mature_crown_width_m);
	plant_wire_put_f32(buf, &off, g->physiology.max_age_years);

	if (out_len) *out_len = off;
	return PLANT_OK;
}

static inline PlantStatus
plant_genome_deserialize(PlantGenome *out_g, const uint8_t *buf, size_t len)
{
	if (!out_g || !buf) return PLANT_ERR_NULL_PTR;
	if (len < 192u) return PLANT_ERR_SERIALIZE_BUFFER;

	size_t off = 0u;
	uint32_t magic = plant_wire_get_u32(buf, &off);
	if (magic != PLANT_GENOME_WIRE_MAGIC)
		return PLANT_ERR_SCHEMA_VERSION;
	uint32_t ver = plant_wire_get_u32(buf, &off);
	if (ver != PLANT_GENOME_SCHEMA_VERSION)
		return PLANT_ERR_SCHEMA_VERSION;
	uint32_t sp_id = plant_wire_get_u32(buf, &off);

	PlantSpeciesDef sp;
	plant_species_init_preset(&sp, sp_id < 16u ? sp_id : 0u);
	*out_g = sp.canonical_genome;
	out_g->schema_version = ver;
	out_g->species_id = sp_id;
	out_g->individual_id = plant_wire_get_u64(buf, &off);
	out_g->parent_id[0]  = plant_wire_get_u64(buf, &off);
	out_g->parent_id[1]  = plant_wire_get_u64(buf, &off);
	out_g->genetic_seed  = plant_wire_get_u64(buf, &off);

	out_g->architecture.internode_length_m = plant_wire_get_dist(buf, &off);
	out_g->architecture.branch_angle_deg   = plant_wire_get_dist(buf, &off);
	out_g->architecture.apical_dominance   = plant_wire_get_f32(buf, &off);
	out_g->architecture.orthotropy         = plant_wire_get_f32(buf, &off);
	out_g->architecture.tortuosity         = plant_wire_get_f32(buf, &off);
	out_g->allometry.pipe_model_exponent   = plant_wire_get_f32(buf, &off);
	out_g->allometry.root_flare_strength   = plant_wire_get_f32(buf, &off);
	out_g->leaf.blade_length_m             = plant_wire_get_dist(buf, &off);
	out_g->leaf.width_ratio                = plant_wire_get_dist(buf, &off);
	out_g->flower.flower_radius_m          = plant_wire_get_dist(buf, &off);
	out_g->fruit.length_m                  = plant_wire_get_dist(buf, &off);
	out_g->fruit.diameter_m                = plant_wire_get_dist(buf, &off);
	out_g->physiology.growth_rate          = plant_wire_get_f32(buf, &off);
	out_g->physiology.mature_height_m      = plant_wire_get_f32(buf, &off);
	out_g->physiology.mature_crown_width_m = plant_wire_get_f32(buf, &off);
	out_g->physiology.max_age_years        = plant_wire_get_f32(buf, &off);

	plant_genome_sync_traits(out_g);
	return PLANT_OK;
}

/* Schema Migration API (IDEA.md §9) */
static inline PlantStatus
plant_genome_migrate_v1_to_v2(const PlantGenome *v1, PlantGenome *v2_out)
{
	if (!v1 || !v2_out)
		return PLANT_ERR_NULL_PTR;
	if (v1->schema_version != 1u)
		return PLANT_ERR_SCHEMA_VERSION;
	*v2_out = *v1;
	plant_genome_sync_traits(v2_out);
	v2_out->schema_version = 2u;
	return PLANT_OK;
}

static inline PlantStatus
plant_species_migrate_v1_to_v2(const PlantSpeciesDef *v1, PlantSpeciesDef *v2_out)
{
	if (!v1 || !v2_out)
		return PLANT_ERR_NULL_PTR;
	if (v1->schema_version != 1u)
		return PLANT_ERR_SCHEMA_VERSION;
	*v2_out = *v1;
	v2_out->schema_version = 2u;
	return PLANT_OK;
}

/* Stateless World-Coordinate Chrono-Deterministic Helper:
 * Derives a wild plant's current epoch, genetic_seed, and age_days at world clock `world_day`
 * from `(world_seed, tile_x, tile_y, slot_idx)` with zero persistent storage. */
static inline void
plant_world_slot_chrono_state(uint64_t world_seed,
                              int32_t tile_x, int32_t tile_y, uint32_t slot_idx,
                              const PlantSpeciesDef *species,
                              float world_day,
                              uint64_t *out_genetic_seed,
                              float *out_age_days)
{
	uint64_t coord_hash = plant_mix64(
		world_seed ^
		((uint64_t)(uint32_t)tile_x * 0x9E3779B97F4A7C15ULL) ^
		((uint64_t)(uint32_t)tile_y * 0xBF58476D1CE4E5B9ULL) ^
		((uint64_t)slot_idx * 0x94D049BB133111EBULL) ^
		((uint64_t)(species ? species->id : 0u) << 32));

	float max_years = (species && species->canonical_genome.physiology.max_age_years > 1.0f)
		? species->canonical_genome.physiology.max_age_years
		: 100.0f;
	/* Natural growth horizon in plastochron days so wild flora spans juvenile -> mature stages */
	float cycle_days = fmaxf(90.0f, max_years * 3.0f);
	float phase_days = plant_hash_u01(coord_hash, 0u) * cycle_days;
	float total_days = fmaxf(0.0f, world_day) + phase_days;
	uint64_t epoch = (uint64_t)floorf(total_days / cycle_days);
	float age_in_cycle = total_days - (float)epoch * cycle_days;
	/* Ensure wild plants are at least sprouted seedlings (minimum 20% of developmental horizon) */
	float min_sprout = 0.22f * cycle_days;
	float eff_age = min_sprout + 0.78f * age_in_cycle;

	if (out_genetic_seed)
		*out_genetic_seed = plant_mix64(coord_hash ^ epoch);
	if (out_age_days)
		*out_age_days = eff_age;
}

#ifdef __cplusplus
}
#endif

#endif /* ND_PLANT_MODEL_H */
