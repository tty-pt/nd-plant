# *Betula pendula* — Silver Birch (`ID 2`)

## 1. Taxonomic & Architectural Identity
- **Species ID**: `2` (`betula_pendula`)
- **Scientific Name**: *Betula pendula* Roth
- **Common Name**: Silver Birch / European White Birch
- **Family**: Betulaceae
- **Life Form**: Deciduous Broadleaf Tree (`PLANT_LIFEFORM_TREE`)
- **Hallé–Oldeman Architectural Model**: **Troll's Model** (`PLANT_ARCH_TROLL`) — axes are secondarily plagiotropic with strong distal drooping (pendulous 2nd- and 3rd-order twigs), while the proximal portion erects by superposition into a high, airy, weeping crown.
- **Primary Biomes**: Boreal Taiga (`9`), Woodland (`11`), Temperate Seasonal Forest (`12`), Coniferous Forest (`8`).

## 2. Real-World Botanical Dimensions & Proportional Scale (`1 m = 45.0 wu`, Lamb = `0.40 m = 18.0 wu`)

| Metric Parameter | Real-World Botanical Range | Representative Mature Value | Proportional to `40 cm` Lamb (`0.40 m`) | World Units (`1 m = 45.0 wu`) |
| :--- | :--- | :--- | :--- | :--- |
| **Mature Tree Height (`H_big`)** | `15.0 – 25.0 m` | **`19.0 m`** | **`47.5×` Lamb height** | **`855 wu`** (`675 – 1125 wu`) |
| **Mature Crown Spread (`W_big`)** | `7.0 – 12.0 m` | **`9.5 m`** | **`23.8×` Lamb height** | **`428 wu`** (`315 – 540 wu`) |
| **Juvenile / Sapling Height (`H_small`)** | `2.5 – 4.5 m` | **`3.4 m`** | **`8.5×` Lamb height** | **`153 wu`** (`112 – 202 wu`) |
| **Juvenile Crown Spread (`W_small`)** | `1.4 – 2.6 m` | **`2.0 m`** | **`5.0×` Lamb height** | **`90 wu`** (`63 – 117 wu`) |
| **Trunk DBH (Diameter at Breast Height)** | `0.30 – 0.70 m` | **`0.46 m`** | **`1.15×` Lamb height** | **`20.7 wu`** (`13.5 – 31.5 wu`) |
| **Clear Bole Height** | `30% – 40%` of height (`4.5 – 10 m`) | **`34%` (`6.5 m`)** | **`16.2×` Lamb height** | **`292 wu`** |

## 3. Structural, Bark & Foliage Morphology (For 3D Mesh & Shader Definition)
- **Trunk & Bark (`ND_PLANT_BARK_BIRCH = 2`)**:
  - Slender, graceful, often twin-stemmed (`basal_stem_count = 2`) trunk.
  - **Lower basal bark (`z < 0.25 H`)**: Rough, deeply fissured black-charcoal diamond crags (`RGB(58, 54, 52)`).
  - **Mid & upper trunk**: Brilliant chalky silver-white periderm (`RGB(236, 232, 226)`) scored with horizontal dark lenticel bands and dark diamond branch-scar chevrons.
  - **Fine 2nd/3rd-order twigs**: Glossy dark reddish-brown to purple-black pendulous whip-like branchlets.
- **Branching & Crown Habit**:
  - Ascending primary scaffold limbs (`branch_angle_deg = 46° ± 7°`) that arch outward and dissolve into long, cascading curtains of fine weeping secondary and tertiary branchlets (`gravitropism = -0.22`).
- **Foliage (`ND_MAT3D_FOL_OVATE`, Rhombic-Deltoid Doubly Serrate Leaves)**:
  - Small (`3 – 7 cm`), triangular-ovate leaves with acuminate tips and doubly serrate margins (`PLANT_MARGIN_DOUBLY_SERRATE`).
  - Light, translucent yellow-green to emerald (`RGB(42, 84, 32)` to `RGB(178, 214, 102)`), arranged in airy, sun-dappled pendulous veils that let the white trunk and sky shine through the interior crown.
