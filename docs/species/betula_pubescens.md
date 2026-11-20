# *Betula pubescens* — Downy / Arctic Birch (`ID 4`)

## 1. Taxonomic & Architectural Identity
- **Species ID**: `4` (`betula_pubescens`)
- **Scientific Name**: *Betula pubescens* Ehrh.
- **Common Name**: Downy Birch / Moor Birch / Arctic White Birch
- **Family**: Betulaceae
- **Life Form**: Deciduous Broadleaf Tree (`PLANT_LIFEFORM_TREE`)
- **Hallé–Oldeman Architectural Model**: **Rauh's Model** (`PLANT_ARCH_RAUH`) with high basitony (`basal_stem_count = 3`) — often multi-stemmed or wind-sculpted subarctic/krummholz habit, with ascending to spreading (non-pendulous) downy twigs.
- **Primary Biomes**: Boreal Forest / Taiga (`9`), Subarctic Tundra Edge (`2`), Alpine Meadow / Timberline (`3`), Shrubland (`7`).

## 2. Real-World Botanical Dimensions & Proportional Scale (`1 m = 45.0 wu`, Lamb = `0.40 m = 18.0 wu`)

| Metric Parameter | Real-World Botanical Range | Representative Mature Value | Proportional to `40 cm` Lamb (`0.40 m`) | World Units (`1 m = 45.0 wu`) |
| :--- | :--- | :--- | :--- | :--- |
| **Mature Tree Height (`H_big`)** | `10.0 – 20.0 m` | **`14.5 m`** | **`36.2×` Lamb height** | **`652 wu`** (`450 – 900 wu`) |
| **Mature Crown Spread (`W_big`)** | `6.0 – 10.0 m` | **`8.0 m`** | **`20.0×` Lamb height** | **`360 wu`** (`270 – 450 wu`) |
| **Juvenile / Sapling Height (`H_small`)** | `1.8 – 3.6 m` | **`2.6 m`** | **`6.5×` Lamb height** | **`117 wu`** (`81 – 162 wu`) |
| **Juvenile Crown Spread (`W_small`)** | `1.4 – 2.4 m` | **`1.8 m`** | **`4.5×` Lamb height** | **`81 wu`** (`63 – 108 wu`) |
| **Trunk DBH (Per Stem)** | `0.25 – 0.50 m` | **`0.35 m`** | **`0.88×` Lamb height** | **`15.8 wu`** (`11.2 – 22.5 wu`) |
| **Clear Bole Height** | `25% – 35%` of height (`2.5 – 6.5 m`) | **`26%` (`3.8 m`)** | **`9.5×` Lamb height** | **`170 wu`** |

## 3. Structural, Bark & Foliage Morphology (For 3D Mesh & Shader Definition)
- **Trunk & Bark (`ND_PLANT_BARK_BIRCH = 2`)**:
  - Multi-stemmed (`2–4` diverging crooked stems from a low root crown) with higher tortuosity (`tortuosity = 0.24`) than *B. pendula*.
  - Matte greyish-white to creamy-buff bark (`RGB(218, 214, 206)`) with horizontal grey lenticel bands, lacking the deep black basal diamond fissures of *B. pendula*.
- **Branching & Crown Habit**:
  - Compact, ovoid-to-irregular crown with ascending and spreading branches (`branch_angle_deg = 52° ± 7°`) that do **not** droop into long weeping curtains.
- **Foliage (`ND_MAT3D_FOL_OVATE`, Rounded-Ovate Serrate Leaves)**:
  - Ovate leaves (`2 – 5 cm`) with rounded bases and singly serrate margins (`PLANT_MARGIN_SERRATE`), slightly darker and cooler green (`RGB(38, 78, 34)` to `RGB(162, 198, 92)`) than *B. pendula*.
