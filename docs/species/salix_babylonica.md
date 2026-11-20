# *Salix babylonica* — Weeping Willow (`ID 11`)

## 1. Taxonomic & Architectural Identity
- **Species ID**: `11` (`salix_babylonica`)
- **Scientific Name**: *Salix babylonica* L.
- **Common Name**: Weeping Willow / Babylon Willow
- **Family**: Salicaceae
- **Life Form**: Deciduous Riparian Broadleaf Tree (`PLANT_LIFEFORM_TREE`)
- **Hallé–Oldeman Architectural Model**: **Troll's Model** (`PLANT_ARCH_TROLL`) with strong positive gravitropism on distal shoots (`gravitropism = -0.58`) — stout, leaning trunk and arching primary scaffold boughs from which long, flexible, whip-like 2nd- and 3rd-order branchlets cascade vertically toward the ground or water surface.
- **Primary Biomes**: Wetland / Riparian Marsh (`5`), Temperate Rainforest (`13`), Tropical Rainforest Floodplain (`17`), Tropical Seasonal (`16`).

## 2. Real-World Botanical Dimensions & Proportional Scale (`1 m = 45.0 wu`, Lamb = `0.40 m = 18.0 wu`)

| Metric Parameter | Real-World Botanical Range | Representative Mature Value | Proportional to `40 cm` Lamb (`0.40 m`) | World Units (`1 m = 45.0 wu`) |
| :--- | :--- | :--- | :--- | :--- |
| **Mature Tree Height (`H_big`)** | `12.0 – 20.0 m` | **`16.0 m`** | **`40.0×` Lamb height** | **`720 wu`** (`540 – 900 wu`) |
| **Mature Crown Spread (`W_big`)** | `12.0 – 22.0 m` | **`17.5 m`** | **`43.75×` Lamb height** | **`788 wu`** (`540 – 990 wu`) |
| **Juvenile / Sapling Height (`H_small`)** | `2.5 – 4.5 m` | **`3.4 m`** | **`8.5×` Lamb height** | **`153 wu`** (`112 – 202 wu`) |
| **Juvenile Crown Spread (`W_small`)** | `2.2 – 4.2 m` | **`3.2 m`** | **`8.0×` Lamb height** | **`144 wu`** (`99 – 189 wu`) |
| **Trunk DBH (Diameter at Breast Height)** | `0.60 – 1.20 m` | **`0.85 m`** | **`2.12×` Lamb height** | **`38.2 wu`** (`27.0 – 54.0 wu`) |
| **Clear Bole Height (Trunk Fork Height)** | `25% – 35%` of height (`3.0 – 6.5 m`) | **`30%` (`4.8 m`)** | **`12.0×` Lamb height** | **`216 wu`** |
| **Pendulous Curtain Skirt Clearance** | Tips hang down to `1.0 – 2.5 m` above ground | **`1.6 m` (`4×` Lamb)** | **`4.0×` Lamb height** | **`72 wu`** |

## 3. Structural, Bark & Foliage Morphology (For 3D Mesh & Shader Definition)
- **Trunk & Bark (`ND_PLANT_BARK_FISSURED = 0`)**:
  - Stout, often leaning riparian bole (`orthotropy = 0.65`, `root_flare_strength = 0.36`) with interlacing grey-brown ridged bark (`RGB(48, 40, 32)` to `RGB(122, 106, 86)`).
- **Branching & Crown Habit**:
  - Broad, fountain-like arching primary scaffold boughs (`branch_angle_deg = 54° ± 8°`) rising to the upper dome and then spilling over into long, vertical, curtain-like weeping streamers (`ND_MAT3D_FOL_BLADE` / narrow lanceolate pendulous sheets).
- **Foliage (`ND_MAT3D_FOL_BLADE` / Narrow Lanceolate Pendulous Fronds)**:
  - Narrowly lanceolate leaves (`8 – 16 cm` long, `1 – 2 cm` wide, `width_ratio = 0.18`) with finely serrulate margins, warm yellow-green / chartreuse above (`RGB(34, 72, 28)` to `RGB(168, 212, 98)`) and glaucous silvery-green beneath.
