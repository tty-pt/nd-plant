# *Quercus robur* — English Pedunculate Oak (`ID 10`)

## 1. Taxonomic & Architectural Identity
- **Species ID**: `10` (`quercus_robur`)
- **Scientific Name**: *Quercus robur* L.
- **Common Name**: English Oak / Pedunculate Oak
- **Family**: Fagaceae
- **Life Form**: Broadleaf Deciduous Canopy Tree (`PLANT_LIFEFORM_TREE`)
- **Hallé–Oldeman Architectural Model**: **Rauh's Model** (`PLANT_ARCH_RAUH`) with rhythmic flushing, strong acrotony (`acrotony = 0.65`), and low apical dominance in maturity (`apical_dominance = 0.32`) — producing a massive, short trunk that divides into heavy, gnarled, twisting structural boughs supporting a vast hemispherical dome.
- **Primary Biomes**: Woodland (`11`), Temperate Seasonal Forest (`12`), Temperate Rainforest (`13`), Riparian Wetland Margin (`5`).

## 2. Real-World Botanical Dimensions & Proportional Scale (`1 m = 45.0 wu`, Lamb = `0.40 m = 18.0 wu`)

| Metric Parameter | Real-World Botanical Range | Representative Mature Value | Proportional to `40 cm` Lamb (`0.40 m`) | World Units (`1 m = 45.0 wu`) |
| :--- | :--- | :--- | :--- | :--- |
| **Mature Tree Height (`H_big`)** | `20.0 – 35.0 m` | **`24.0 m`** | **`60.0×` Lamb height** | **`1080 wu`** (`900 – 1575 wu`) |
| **Mature Crown Spread (`W_big`)** | `20.0 – 35.0 m` | **`25.5 m`** | **`63.75×` Lamb height** | **`1148 wu`** (`900 – 1575 wu`) |
| **Juvenile / Sapling Height (`H_small`)** | `2.8 – 5.5 m` | **`4.0 m`** | **`10.0×` Lamb height** | **`180 wu`** (`126 – 248 wu`) |
| **Juvenile Crown Spread (`W_small`)** | `2.2 – 4.5 m` | **`3.4 m`** | **`8.5×` Lamb height** | **`153 wu`** (`99 – 202 wu`) |
| **Trunk DBH (Diameter at Breast Height)** | `1.00 – 2.50 m` | **`1.50 m`** | **`3.75×` Lamb height** | **`67.5 wu`** (`45.0 – 112.5 wu`) |
| **Root-Collar Basal Diameter** | `1.60 – 3.80 m` | **`2.30 m`** | **`5.75×` Lamb height** | **`103.5 wu`** |
| **Clear Bole Height** | `25% – 35%` of height (`5.0 – 10.0 m`) | **`28%` (`6.7 m`)** | **`16.8×` Lamb height** | **`302 wu`** |

## 3. Structural, Bark & Foliage Morphology (For 3D Mesh & Shader Definition)
- **Trunk & Bark (`ND_PLANT_BARK_FISSURED = 0`)**:
  - Stout, heavily buttressed pillar with 4 to 6 flaring root-collar spurs (`root_flare_strength = 0.50`) that spread wider than two standing Lambs at ground contact.
  - Deeply furrowed, dark grey-brown longitudinal bark ridges (`RGB(42, 32, 24)` in fissures to `RGB(118, 96, 74)` on sun-bleached ridge crests).
- **Branching & Crown Habit**:
  - Primary boughs (`branch_angle_deg = 62° ± 9°`, `tortuosity = 0.34`) twist and elbow abruptly (`gnarl = 0.70`) due to recurrent terminal bud abortion and sylleptic lateral take-over.
  - The crown is divided into distinct, billowy sub-crowns separated by deep shadow hollows (`AO = 0.16–0.28`), through which the gnarled inner limbs remain clearly visible.
- **Foliage (`ND_MAT3D_FOL_LOBED`, Auriculate Pinnately Lobed Leaves)**:
  - Oblong-obovate leaves (`7 – 14 cm`) with 4–6 pairs of rounded, asymmetric sinuous lobes (`PLANT_MARGIN_LOBED`) and a very short petiole.
  - Deep, rich forest green (`RGB(24, 58, 22)` deep canopy umbra to `RGB(142, 184, 82)` sunlit upper dome).
- **Fruit (Pedunculate Acorns)**:
  - Ovoid acorns (`2 – 3 cm`) borne in 1–3s on long slender peduncles (`4 – 8 cm`), ripening from green to warm chestnut brown.
