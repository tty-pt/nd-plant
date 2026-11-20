# IDEA.md — Chrono-Deterministic Developmental Botany with Ephemeral Delta Decay

## 1. Executive Summary

NeverDark requires a real-world grounded plant genetics, development, and rendering architecture (`nd-plant`) that achieves four seemingly contradictory goals simultaneously:

1. **Botanical Authenticity (Genetics $\to$ Development $\to$ Phenotype $\to$ Geometry)**:
   Plants are not static procedural mesh props driven by ad-hoc shader knobs. Every plant belongs to a biological species (`PlantSpeciesDef` with genetic trait priors), possesses an individual sampled or bred genome (`PlantGenome` encoding architecture, phyllotaxis, meristem organogenesis, allometry, leaf/flower/fruit morphology, physiology, and environmental plasticity), and develops an explicit Multi-Scale Tree Graph (`PlantPhenotype`: `PlantAxis`, `PlantGrowthUnit`, `PlantBud`, `PlantOrgan`) before any 3D or 2D geometry is emitted.
2. **Continuous Growth Over Time**:
   Plants visibly age, flush new phytomers, thicken their trunks and boughs via Da Vinci pipe-model secondary growth, flower, fruit, senesce, and turn over across the world clock.
3. **Strict $\Delta t$-Invariant Chrono-Determinism**:
   Evaluating a plant at world timestamp $T_{\text{now}} = t_0 + \Delta t$ produces the **exact same `PlantPhenotype` graph** regardless of whether the area was observed continuously in 1-day ticks or visited for the first time after 50 game-years.
4. **Zero-Memory Unvisited World & Ephemeral Self-Healing (Delta Decay)**:
   Unmodified wild areas consume **zero bytes** of RAM or database storage. Player or ecological perturbations (chopping a branch, pruning, stripping bark/leaves, picking fruit, or picking up a stone) are recorded as compact, time-bounded delta records (`PlantDeltaRecord` / `WorldDeltaRecord`). Over time, pruned branches regrow from dormant/epicormic buds, harvested organs replenish, and once a delta reaches its healing horizon $t_{\text{healed}}$, the record is garbage-collected and the area mathematically converges back to the zero-storage canonical world state.

---

## 2. Architectural Separation of Concerns

```text
+-------------------------------------------------------------------------+
|                        1. SPECIES DEFINITION                            |
|                            (PlantSpeciesDef)                            |
|   Immutable species taxonomy, architectural model (Rauh, Massart,       |
|   Leeuwenberg, Troll, Schoute, Corner, etc.), life-form, and            |
|   population-level statistical trait priors (PlantTraitPrior).          |
+------------------------------------+------------------------------------+
                                     |
                                     |  plant_genome_sample(species, genetic_seed)
                                     |  plant_genome_cross(parent_a, parent_b, seed)
                                     v
+-------------------------------------------------------------------------+
|                        2. INDIVIDUAL GENOME                             |
|                              (PlantGenome)                              |
|   9 modular sub-genomes:                                                |
|     - PlantArchitectureGenome  (branching order, apical dominance)      |
|     - PlantPhyllotaxisGenome   (divergence angle, buds/node, decussate) |
|     - PlantMeristemGenome      (plastochron, bud break, dormancy)       |
|     - PlantAllometryGenome     (pipe exponent alpha, taper, flare, bark)|
|     - PlantLeafGenome          (complexity, shape, blade/petiole, veins)|
|     - PlantFlowerGenome        (inflorescence, symmetry, parts, color)  |
|     - PlantFruitGenome         (type, size, ripening color, retention)  |
|     - PlantPhysiologyGenome    (light/water/temp/wind optima, max age)  |
|     - PlantPlasticityGenome    (phototropism, gravitropism, shade)      |
+------------------------------------+------------------------------------+
                                     |
                                     |  plant_simulate_at_time(instance, env, t_now, deltas)
                                     |  plant_simulate_step(instance, env, dt_days)
                                     v
+-------------------------------------------------------------------------+
|                        3. DEVELOPED PHENOTYPE                           |
|                             (PlantPhenotype)                            |
|   Index-addressed Multi-Scale Tree Graph (MTG):                         |
|     - PlantAxis[]       : trunk, boughs, twigs, roots (order, parent)   |
|     - PlantGrowthUnit[] : phytomers / internodes (base, tip, radius,    |
|                           orientation, topological hash, organ links)   |
|     - PlantBud[]        : apical & axillary meristems (fate, state)     |
|     - PlantOrgan[]      : leaves, flowers, fruits, thorns, catkins      |
+------------------------------------+------------------------------------+
                                     |
                                     |  plant_geometry_build(plant, lod, mesh_out)
                                     v
+-------------------------------------------------------------------------+
|                        4. GEOMETRY CONSUMER                             |
|                     (nd_seven_mesh3d.h / 2D Lamina)                     |
|   Pure consumer of PlantPhenotype:                                      |
|     - Sweeps continuous bark tubes along PlantAxis phytomer chains      |
|     - Welds branch collars and basal root buttress flares               |
|     - Emits species-specific leaf blades, needle sprays, flowers, fruit |
+-------------------------------------------------------------------------+
```

**Hard Boundary Rule**:
- `external/nd-plant` owns Stages 1, 2, and 3 (`include/nd/plant_model.h`, `src/libnd-plant.c`). It is pure C99/C11 with zero OpenGL, WebGL, DOM, or renderer dependencies, and compiles identically for native server (`.so`) and browser (`wasm32`).
- `mods/nd/ux/nd_seven_mesh3d.h` owns Stage 4. It never invents botanical rules; it translates a `PlantPhenotype` graph into vertex/index buffers at the requested LOD.

---

## 3. Stateless Chrono-Determinism: How Plants Grow Without Being Stored

### 3.1 Spatial Birth & Lifecycle Addressing
For any wild terrain tile `(world_x, world_y)` and plant slot `s in {0, 1, 2}`:
1. **Existence & Species**: Determined by `nd_plant_noise_eval_bio_dims()` from the tile's elevation, water table, temperature, rainfall, and overstory canopy budget.
2. **Individual Genetic Seed (`genetic_seed`)**:
   $$\text{seed}_{\text{base}} = \text{SplitMix64}(\text{world\_seed} \oplus (\text{tile\_x} \cdot p_1) \oplus (\text{tile\_y} \cdot p_2) \oplus s)$$
3. **Turnover Epoch & Birth Timestamp (`t_birth`)**:
   Wild plants live, grow old, die, and are replaced by new seedlings over natural ecological cycles. Given species longevity $T_{\text{life}} = \text{max\_age\_years} \cdot 365$ days and fallow gap $T_{\text{gap}}$:
   $$T_{\text{cycle}} = T_{\text{life}} + T_{\text{gap}}$$
   $$\phi = \text{seed}_{\text{base}} \bmod T_{\text{cycle}}$$
   $$\text{epoch} = \left\lfloor \frac{T_{\text{now}} + \phi}{T_{\text{cycle}}} \right\rfloor, \qquad \text{age\_days}(T_{\text{now}}) = (T_{\text{now}} + \phi) \bmod T_{\text{cycle}}$$
   $$\text{genetic\_seed} = \text{SplitMix64}(\text{seed}_{\text{base}} \oplus \text{epoch})$$
   - While $\text{age\_days} < T_{\text{life}}$, the individual with `genetic_seed` is alive at exact age $\text{age\_days}$.
   - When $\text{age\_days}$ wraps into the next epoch, a new genetic individual germinates in that niche.

### 3.2 Topological Coordinate Hashing (`topo_hash`) for $\Delta t$-Invariance
In traditional L-systems or step-by-step simulators, advancing by $\Delta t = 100$ days in one jump vs. $100 \times 1$-day steps consumes random numbers from a sequential PRNG in different orders (e.g. breadth-first vs. depth-first across growing axes). Consequently, the tree changes shape depending on how often a player visits the chunk.

We eliminate sequential PRNG state entirely during development. Every `PlantGrowthUnit` $u$ in the `PlantPhenotype` carries a deterministic 64-bit **Topological Coordinate Hash** (`topo_hash`):

- **Root / Collar Origin Unit**:
  $$H_0 = \text{SplitMix64}(\text{genetic\_seed} \oplus \text{0x504C414E545F3030ULL})$$
- **Apical Continuation** (unit $u$ extends along the same `PlantAxis` to unit $u'$ at plastochron index $m$):
  $$H_{u'} = \text{SplitMix64}(H_u \oplus \text{0x41504943414C5F31ULL} \oplus m)$$
- **Lateral Branching** (axillary bud $b \in [0, B-1]$ on unit $u$ breaks dormancy to initiate a child `PlantAxis` of order $d+1$):
  $$H_{\text{child}} = \text{SplitMix64}(H_u \oplus \text{0x4C41544552414C32ULL} \oplus ((d + 1) \ll 16) \oplus b)$$

Every intra-individual stochastic sample at unit $u$ — such as:
- Internode length drawn from `genome.architecture.internode_length_m` (`PlantDistribution`)
- Branching pitch angle drawn from `genome.architecture.branch_angle_deg`
- Phyllotactic divergence jitter drawn from `genome.phyllotaxis.divergence_jitter_deg`
- Axillary bud activation test against `genome.meristem.lateral_bud_probability`
- Organ dimensions drawn from `genome.leaf.blade_length_m`, `width_ratio`, `curvature`

is computed as a pure stateless hash of $(H_u, \text{CHANNEL\_ID})$.

**Theorem ($\Delta t$-Invariance)**:
Because a node's developmental parameters depend solely on its immutable topological path $H_u$ from the seed collar and its physiological birth time $t_{\text{birth}}(u)$, evaluating `plant_simulate_at_time(T)` in a single analytical pass at $T = t_0 + \Delta t$ produces the **bit-identical** `PlantPhenotype` graph as stepping through intermediate timestamps $t_0 \to t_1 \to \dots \to T$.

---

## 4. Biologically Grounded Developmental Mechanics

### 4.1 Thermal Time & Plastochron Rhythm
A meristem emits new phytomers (`PlantGrowthUnit`) at a species-specific plastochron interval $\tau_p$ (`plastochron_days`), modulated by environmental vigour $V_{\text{env}} \in [0.1, 1.0]$ computed from `PlantEnvironment` against `PlantPhysiologyGenome` (temperature response curve, soil moisture vs. drought tolerance, light availability):
$$\tau_{\text{eff}} = \frac{\tau_p}{\max(0.15, V_{\text{env}})}$$

At age $A$ (in days), the primary axis has elapsed $N_0 = A / \tau_{\text{eff}}$ plastochrons:
- Complete phytomers $k \in \{0, \dots, \lfloor N_0 \rfloor - 1\}$ have full maturity ($f_k = 1.0$).
- The active terminal phytomer $k = \lfloor N_0 \rfloor$ has fractional expansion $f_k = N_0 - \lfloor N_0 \rfloor \in [0, 1)$, elongated via a smooth logistic curve:
  $$s(f) = \frac{1}{1 + e^{-10(f - 0.35)}}$$
  so shoots and terminal leaves unfold smoothly over time without popping.

### 4.2 Phyllotaxis & Axillary Bud Placement
At each node $k$ along an axis, the meristem places $B = \text{buds\_per\_node}$ axillary buds (`PlantBud`):
- **Spiral / Alternate (`PLANT_PHYLLO_ALTERNATE`, `PLANT_PHYLLO_SPIRAL`)**:
  Each successive node rotates around the parent axis tangent $\mathbf{T}$ by `divergence_angle_deg` (e.g., $137.5^\circ$ golden angle or $180^\circ$ distichous) plus `divergence_jitter_deg`.
- **Opposite / Decussate (`PLANT_PHYLLO_OPPOSITE`, `PLANT_PHYLLO_DECUSSATE`)**:
  $B = 2$ buds placed $180^\circ$ apart; if `decussate_phase_flip` is enabled, each successive node rotates the pair by $90^\circ$.
- **Whorled (`PLANT_PHYLLO_WHORLED`)**:
  $B \ge 3$ buds spaced by $360^\circ / B$ around the node collar, characteristic of conifers (*Pinus*, *Pseudotsuga*, *Abies*).

### 4.3 Apical Dominance, Acrotony/Basitony & Lateral Bud Break
An axillary bud sitting at distance $d_{\text{apex}}$ (in phytomers) below its parent axis's active apical meristem experiences hormonal inhibition:
$$I_{\text{apical}} = \text{apical\_dominance} \cdot e^{-d_{\text{apex}} / \lambda_{\text{dom}}}$$
If the parent apical meristem is damaged, senescent, or pruned (by a player delta!), $I_{\text{apical}}$ drops to $0$, automatically releasing dormant lateral buds below the cut (epicormic / traumatic reiteration).

The positional activation profile along the parent axis is shaped by **acrotony** (tree-like upper-branch dominance, e.g., oaks and pines) vs. **basitony** (shrub-like basal branching, e.g., flax, bushes, rosettes):
$$P_{\text{break}}(u, b) = P_{\text{lateral}} \cdot (1 - I_{\text{apical}}) \cdot W_{\text{position}}(u) \cdot V_{\text{light}}(u)$$

When a deterministic draw $\text{rand01}(H_u, \text{CH\_BUD\_BREAK} + b) < P_{\text{break}}$, the bud transitions to `PLANT_BUD_ACTIVE` at birth time $t_{\text{bud}} = t_{\text{node}} + \tau_{\text{lag}}$ and recursively develops a child `PlantAxis` of order $\text{order} + 1$ (up to `max_branch_order`).

### 4.4 Tropisms & Structural Biomechanics
As each `PlantGrowthUnit` elongates from `base_pos` to `tip_pos`, its heading vector $\mathbf{D}$ is updated by:
1. **Gravitropism & Phototropism (`PlantPlasticityGenome`)**:
   - Orthotropic shoots (conifer leaders, main trunks) steer toward zenith $+\mathbf{Z}$ and mean sky light vector $\mathbf{L}_{\text{light}}$.
   - Plagiotropic boughs steer toward horizontal canopy spread.
2. **Mechanical Self-Weight & Weeping Bending**:
   - Distal shoots deflect under gravity inversely proportional to `wood_density` and `stiffness_young_modulus`, producing naturalarching boughs (*Salix babylonica* weeping curtains, *Betula pendula* pendulous tips).
3. **Tortuosity / Gnarl**:
   - Deterministic 3D curl perturbation derived from $H_u$, giving ancient *Olea europaea* and *Quercus robur* limbs their characteristic twisted character.

### 4.5 Basipetal Da Vinci Pipe-Model Allometry
Once all active `PlantGrowthUnit`s and `PlantOrgan`s are emitted for timestamp $T_{\text{now}}$, a single reverse-topological pass (from highest-order terminal twigs down to the root collar) computes exact vascular radii using Leonardo da Vinci's area-preserving pipe model ($r_p^\alpha = \sum r_i^\alpha$):

1. Every terminal twig tip starts with minimum conductive radius $r_{\text{tip}}$ proportional to its supported leaf area.
2. Flowing basipetally from unit $u_{\text{next}}$ and all lateral child axes $c \in \text{children}(u)$ into unit $u$:
   $$A_u = r_{u_{\text{next}}}^\alpha + \sum_{c} r_{c,\text{base}}^\alpha + \kappa_{\text{taper}} L_u^\alpha$$
   $$r_u = A_u^{1 / \alpha} \cdot (1 + \text{bark\_thickness\_ratio})$$
   where $\alpha = \text{genome.allometry.pipe\_model\_exponent}$ (typically $1.8 \dots 2.5$).
3. Near the ground collar ($z \to 0$), `root_flare_strength` expands $r_0$ smoothly into buttressed root flares.

---

## 5. Ephemeral Delta Decay: Self-Healing Worlds in Zero Persistent Memory

### 5.1 The Perturbation Model (`PlantDeltaRecord`)
When a player interacts with a plant (e.g., chops a bough, harvests fruit, strips leaves, or fells a sapling) or picks up a surface stone, we record only the delta:

```c
typedef enum {
    PLANT_DELTA_NONE = 0,
    PLANT_DELTA_PRUNE_AXIS,     /* Branch/stem severed at target_topo_hash */
    PLANT_DELTA_HARVEST_FRUIT,  /* Reproductive organs harvested on subtree */
    PLANT_DELTA_STRIP_FOLIAGE,  /* Leaves browsed/stripped on subtree */
    PLANT_DELTA_FELL_PLANT      /* Entire above-ground plant cut at collar */
} PlantDeltaKind;

typedef struct {
    uint64_t       target_topo_hash; /* 0 = whole plant, or specific PlantGrowthUnit topo_hash */
    PlantDeltaKind kind;
    float          t_event_days;     /* Plant age (or world day) when perturbation occurred */
    float          t_heal_days;      /* Duration until perturbation is 100% regenerated */
} PlantDeltaRecord;
```

### 5.2 Continuous Regrowth & Asymptotic Convergence
During `plant_simulate_at_time(instance, env, age_days, deltas, delta_count)`:
For any active delta where $t_{\text{event}} \le \text{age\_days} < t_{\text{event}} + t_{\text{heal}}$, we compute the normalized **Recovery Progress**:
$$\eta = \frac{\text{age\_days} - t_{\text{event}}}{t_{\text{heal}}} \in [0, 1)$$

- **If `eta >= 1.0` (Expired Delta)**:
  The delta has completely healed. The simulator ignores it (and the server storage layer deletes the record). The subtree evaluates to its exact unaltered canonical state!
- **If `0 <= eta < 1.0` (Regrowing Delta)**:
  - **`PLANT_DELTA_HARVEST_FRUIT` / `PLANT_DELTA_STRIP_FOLIAGE`**:
    Organs on the affected subtree re-emerge when $\eta$ passes a bud-initiation threshold (e.g. $\eta > 0.25$) and scale their size by $(\eta - 0.25) / 0.75$, smoothly reaching $1.0$ at $\eta = 1.0$.
  - **`PLANT_DELTA_PRUNE_AXIS` / `PLANT_DELTA_FELL_PLANT`**:
    At the severed unit $u_{\text{cut}}$ (`topo_hash == target_topo_hash`):
    - The downstream child units along the cut subtree are evaluated with a **catch-up effective age**:
      $$A_{\text{regrow}}(\eta) = \eta \cdot A_{\text{canonical}}$$
    - When $\eta = 0$ (just cut), $A_{\text{regrow}} = 0$, so the branch terminates cleanly at the cut scar (and releases lateral buds immediately below the cut via reduced apical inhibition!).
    - As $\eta \to 1$, the epicormic replacement shoot elongates through the exact topological coordinate tree ($H_u$) at accelerated catch-up vigour until, at $\eta = 1.0$, $A_{\text{regrow}} = A_{\text{canonical}}$ and the branch seamlessly rejoins the canonical zero-memory trajectory.

---

## 6. Real-Time 3D & 2D Rendering Architecture (`plant_geometry_build`)

To render thousands of genetically distinct plants at 60 FPS in WebGL (`mods/nd/ux/nd_seven_mesh3d.h`):

1. **Direct MTG-to-Mesh Synthesis (`plant_geometry_build`)**:
   - **Wood Skeleton Sweep**: Iterates over `phenotype.axes[a]`, walking the linked list of `PlantGrowthUnit`s (`first_unit` $\to$ `next`). Emits continuous, UV-wrapped tubular bark geometry whose cross-section tapers from `base_radius_m` to `tip_radius_m`, with root-flare fluting at `order == 0` base and smooth collar transitions at branch junctions.
   - **Botanical Organ Meshing**: Iterates over `phenotype.organs[o]`.
     - `PLANT_ORGAN_LEAF`: Uses `PlantLeafGenome` (`complexity`, `shape`, `margin`, `blade_length_m`, `width_ratio`, `petiole_length_m`, `curvature`) to emit articulated petiole + cambered leaf blades (`SIMPLE`), rachis + paired leaflets (`PINNATE`), radiating palmate lobes (`PALMATE`), or conifer needle sprays (`NEEDLE`).
     - `PLANT_ORGAN_FLOWER` & `PLANT_ORGAN_FRUIT`: Uses `PlantFlowerGenome` (`inflorescence`, `corolla_type`, `petal_count`, `primary_color`, `secondary_color`) and `PlantFruitGenome` (`fruit_type`, `length_m`, `diameter_m`, `mature_color`) to place blossoms, umbels, cones, spadices, or berries directly at their developmental bud nodes.
2. **Multi-LOD Detail Levels (`detail_level`)**:
   - `detail_level = 0` (Close-up / Hero): Full branching orders ($0 \dots \text{max\_order}$), higher radial tube segments, individual leaf/flower/fruit organ geometry.
   - `detail_level = 1` (Standard World VBO): Primary/secondary/tertiary woody skeleton + clustered organ cards synthesized from the phenotype's terminal organ nodes.
   - `detail_level = 2` (Far Overstory): Trunk and primary scaffold boughs + consolidated crown clusters.
3. **Genetic Cohort Caching + Dynamic Hero Slots**:
   - Unperturbed background plants draw from a pre-baked per-species **Genetic Cohort Bank** (multiple sampled `PlantGenome` individuals $\times$ developmental growth stages baked into the static VBO atlas).
   - Nearby hero plants, actively growing agricultural crops, or plants with active `PlantDeltaRecord`s are evaluated on-demand in `< 0.25 ms` and uploaded into a bounded LRU mesh ring buffer.

---

## 7. Verification & Quality Gates

1. **Botanical Validation (`plant_species_validate`)**: Every species definition and sampled genome is checked for biological and numerical invariants (valid distributions, finite bounds, valid phyllotaxis and pipe-model exponents).
2. **Determinism & $\Delta t$-Invariance Test**: Verify that `plant_simulate_at_time(T)` produces the exact same `PlantPhenotype` (unit count, axis radii, organ positions, bounding dimensions) as incremental `plant_simulate_step()` calls summing to $T$.
3. **Da Vinci Pipe-Model Invariant Test**: Verify across all branching junctions in developed trees that parent cross-sectional capacity satisfies $r_{\text{parent}}^\alpha \ge \sum r_{\text{child}}^\alpha$.
4. **Ephemeral Delta Decay Convergence Test**: Verify that pruning an axis at $t_{\text{event}}$ truncates the subtree for $\eta = 0$, progressively regrows it for $0 < \eta < 1$, and becomes bit-identical to the unperturbed phenotype at $\eta \ge 1.0$.
5. **Isomorphic Dual-Compile & Boundary Checks**: Zero forbidden symbols in `mods/nd/ux/`, clean native + `wasm32` compilation, and passing standalone unit suites.
