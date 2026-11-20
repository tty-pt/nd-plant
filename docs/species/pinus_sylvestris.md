# *Pinus sylvestris* — Scots Pine (`ID 0`)

## 1. Taxonomic & Architectural Identity
- **Species ID**: `0` (`pinus_sylvestris`)
- **Scientific Name**: *Pinus sylvestris* L.
- **Common Name**: Scots Pine
- **Family**: Pinaceae
- **Life Form**: Evergreen Conifer Tree (`PLANT_LIFEFORM_TREE`)
- **Hallé–Oldeman Architectural Model**: **Rauh's Model** (`PLANT_ARCH_RAUH`) — monopodial orthotropic trunk growing rhythmically, producing sub-verticillate tiers of branches morphogenetically identical to the trunk; self-pruning lower tiers leave a tall, clear, sun-lit upper crown in maturity.
- **Primary Biomes**: Boreal Forest / Taiga (`9`), Coniferous Forest (`8`), Alpine Timberline (`3`), Montane Mediterranean (`4`).

## 2. Real-World Botanical Dimensions & Proportional Scale (`1 m = 45.0 wu`, Lamb = `0.40 m = 18.0 wu`)

| Metric Parameter | Real-World Botanical Range | Representative Mature Value | Proportional to `40 cm` Lamb (`0.40 m`) | World Units (`1 m = 45.0 wu`) |
| :--- | :--- | :--- | :--- | :--- |
| **Mature Tree Height (`H_big`)** | `20.0 – 35.0 m` | **`25.0 m`** | **`62.5×` Lamb height** | **`1125 wu`** (`900 – 1575 wu`) |
| **Mature Crown Spread (`W_big`)** | `7.0 – 12.0 m` | **`9.5 m`** | **`23.8×` Lamb height** | **`428 wu`** (`315 – 540 wu`) |
| **Juvenile / Sapling Height (`H_small`)** | `2.5 – 5.0 m` | **`3.6 m`** | **`9.0×` Lamb height** | **`162 wu`** (`112 – 225 wu`) |
| **Juvenile Crown Spread (`W_small`)** | `1.6 – 3.2 m` | **`2.4 m`** | **`6.0×` Lamb height** | **`108 wu`** (`72 – 144 wu`) |
| **Trunk DBH (Diameter at Breast Height)** | `0.50 – 1.20 m` | **`0.75 m`** | **`1.88×` Lamb height** | **`33.8 wu`** (`22.5 – 54.0 wu`) |
| **Root-Collar Basal Diameter** | `0.75 – 1.65 m` | **`1.10 m`** | **`2.75×` Lamb height** | **`49.5 wu`** |
| **Clear Bole Height (Self-Pruned Trunk)** | `55% – 70%` of height (`11 – 22 m`) | **`58%` (`14.5 m`)** | **`36.2×` Lamb height** | **`652 wu`** |
| **Dead Lower Branch Stub Zone** | `30% – 55%` of height | `4 – 7` weathered grey-brown stubs | `1.5× – 3.0×` Lamb length | `25 – 55 wu` stub length |

## 3. Structural, Bark & Foliage Morphology (For 3D Mesh & Shader Definition)
- **Trunk & Bark (`ND_PLANT_BARK_SCOTS_PINE = 1`)**:
  - **Lower bole (`z < 0.55 H`)**: Deeply fissured, thick, rugged dark purplish-brown / charcoal plates (`RGB(58, 36, 26)` to `RGB(116, 72, 48)`).
  - **Upper bole & primary boughs (`z > 0.55 H`)**: Thin, papery, flaking **warm orange-ochre / copper-red plates** (`RGB(172, 98, 56)`), a signature visual hallmark of *Pinus sylvestris*.
- **Branching & Crown Habit**:
  - Juveniles have a conical pyramidal habit; mature trees lose their lower whorls (leaving short weathered deadwood stubs along the middle trunk) and develop a picturesque, irregular, flat-topped or dome-shaped upper canopy of contorted horizontal-to-ascending red-barked limbs (`branch_angle_deg = 70° ± 8°`, `acrotony = 0.72`).
- **Foliage (`ND_MAT3D_FOL_CONIFER`, Paired Glaucous Needles)**:
  - Needles borne in fascicles of 2, `4 – 7 cm` long, slightly twisted, **glaucous blue-green** (`RGB(22, 56, 38)` deep umbra to `RGB(98, 156, 108)` sunlit tips).
  - Arranged in dense, discrete hemispherical/tufted needle pads at the upturned ends of secondary and tertiary twigs, with clear sky gaps between major bough tiers.
- **Cones**:
  - Ovoid-conic woody cones (`4 – 7 cm`), dull brown when mature, pointing backward along the shoot.
