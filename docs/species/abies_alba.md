# *Abies alba* — European Silver Fir (`ID 5`)

## 1. Taxonomic & Architectural Identity
- **Species ID**: `5` (`abies_alba`)
- **Scientific Name**: *Abies alba* Mill.
- **Common Name**: European Silver Fir
- **Family**: Pinaceae
- **Life Form**: Tall Evergreen Conifer Tree (`PLANT_LIFEFORM_TREE`)
- **Hallé–Oldeman Architectural Model**: **Massart's Model** (`PLANT_ARCH_MASSART`) — strictly orthotropic monopodial column (`orthotropy = 0.98`, `apical_dominance = 0.97`) with regular horizontal (`80°`) tiers of plagiotropic branches that flatten into a "stork's nest" table crown in old-growth trees.
- **Primary Biomes**: Boreal Forest / Taiga (`9`), Montane Coniferous Forest (`8`), Subalpine Timberline (`3`).

## 2. Real-World Botanical Dimensions & Proportional Scale (`1 m = 45.0 wu`, Lamb = `0.40 m = 18.0 wu`)

| Metric Parameter | Real-World Botanical Range | Representative Mature Value | Proportional to `40 cm` Lamb (`0.40 m`) | World Units (`1 m = 45.0 wu`) |
| :--- | :--- | :--- | :--- | :--- |
| **Mature Tree Height (`H_big`)** | `35.0 – 55.0 m` | **`44.0 m`** | **`110.0×` Lamb height** | **`1980 wu`** (`1575 – 2475 wu`) |
| **Mature Crown Spread (`W_big`)** | `9.0 – 15.0 m` | **`12.0 m`** | **`30.0×` Lamb height** | **`540 wu`** (`405 – 675 wu`) |
| **Juvenile / Sapling Height (`H_small`)** | `3.0 – 6.5 m` | **`4.6 m`** | **`11.5×` Lamb height** | **`207 wu`** (`135 – 292 wu`) |
| **Juvenile Crown Spread (`W_small`)** | `1.8 – 3.5 m` | **`2.5 m`** | **`6.25×` Lamb height** | **`112 wu`** (`81 – 158 wu`) |
| **Trunk DBH (Diameter at Breast Height)** | `1.00 – 1.80 m` | **`1.35 m`** | **`3.38×` Lamb height** | **`60.8 wu`** (`45.0 – 81.0 wu`) |
| **Root-Collar Basal Diameter** | `1.40 – 2.50 m` | **`1.90 m`** | **`4.75×` Lamb height** | **`85.5 wu`** |
| **Clear Bole Height** | `14% – 25%` of height (`5 – 13 m`) | **`16%` (`7.0 m`)** | **`17.5×` Lamb height** | **`315 wu`** |

## 3. Structural, Bark & Foliage Morphology (For 3D Mesh & Shader Definition)
- **Trunk & Bark (`ND_PLANT_BARK_FISSURED = 0`, Silver-Grey Plate Bark)**:
  - Straight, cylindrical column with smooth silvery-grey bark in youth that breaks into rectangular silver-ash scaly plates (`RGB(64, 62, 60)` to `RGB(154, 152, 148)`) in maturity.
- **Branching & Crown Habit**:
  - Strictly horizontal tiered whorls (`branch_angle_deg = 80° ± 4°`) radiating in 5-branch rings around the trunk, with clear vertical shadow clefts between tiers.
- **Foliage (`ND_MAT3D_FOL_CONIFER`, Pectinate Two-Ranked Comb Needles)**:
  - Flattened, blunt-tipped needles (`1.8 – 3.0 cm`) parted in two flat, comb-like ranks along the left and right sides of each shoot.
  - Glossy dark blue-green adaxial (upper) surface (`RGB(16, 46, 36)` to `RGB(48, 94, 72)`) and **two chalk-white stomatal bands** on the abaxial (lower) surface (`RGB(108, 168, 142)`), giving the boughs a cool silvery-glaucous shimmer when tilted in the wind.
- **Cones (`ND_PLANT_ACCENT_CONE = 1`)**:
  - Upright cylindrical cones (`10 – 18 cm` tall, `3 – 5 cm` wide) standing **erect like candles** on the upper surface of top-crown whorls.
