# *Pseudotsuga menziesii* — Coast Douglas-fir (`ID 1`)

## 1. Taxonomic & Architectural Identity
- **Species ID**: `1` (`pseudotsuga_menziesii`)
- **Scientific Name**: *Pseudotsuga menziesii* (Mirb.) Franco
- **Common Name**: Coast Douglas-fir
- **Family**: Pinaceae
- **Life Form**: Giant Evergreen Conifer Tree (`PLANT_LIFEFORM_TREE`)
- **Hallé–Oldeman Architectural Model**: **Massart's Model** (`PLANT_ARCH_MASSART`) — orthotropic monopodial trunk with rhythmic growth producing regular, strongly plagiotropic (horizontal to sweeping-drooping) branch whorls at each annual growth unit boundary.
- **Primary Biomes**: Temperate Rainforest (`13`), Coniferous Forest (`8`), Montane Taiga Transition (`9`).

## 2. Real-World Botanical Dimensions & Proportional Scale (`1 m = 45.0 wu`, Lamb = `0.40 m = 18.0 wu`)

| Metric Parameter | Real-World Botanical Range | Representative Mature Value | Proportional to `40 cm` Lamb (`0.40 m`) | World Units (`1 m = 45.0 wu`) |
| :--- | :--- | :--- | :--- | :--- |
| **Mature Tree Height (`H_big`)** | `40.0 – 75.0 m` | **`52.0 m`** | **`130.0×` Lamb height** | **`2340 wu`** (`1800 – 3375 wu`) |
| **Mature Crown Spread (`W_big`)** | `10.0 – 18.0 m` | **`14.0 m`** | **`35.0×` Lamb height** | **`630 wu`** (`450 – 810 wu`) |
| **Juvenile / Sapling Height (`H_small`)** | `3.5 – 7.5 m` | **`5.2 m`** | **`13.0×` Lamb height** | **`234 wu`** (`158 – 338 wu`) |
| **Juvenile Crown Spread (`W_small`)** | `2.0 – 4.0 m` | **`2.8 m`** | **`7.0×` Lamb height** | **`126 wu`** (`90 – 180 wu`) |
| **Trunk DBH (Diameter at Breast Height)** | `1.50 – 2.50 m` | **`1.80 m`** | **`4.50×` Lamb height** | **`81.0 wu`** (`67.5 – 112.5 wu`) |
| **Root-Collar Basal Diameter** | `2.10 – 3.50 m` | **`2.60 m`** | **`6.50×` Lamb height** | **`117.0 wu`** |
| **Clear Bole Height** | `18% – 35%` of height (`7 – 22 m`) | **`22%` (`11.4 m`)** | **`28.5×` Lamb height** | **`515 wu`** |

## 3. Structural, Bark & Foliage Morphology (For 3D Mesh & Shader Definition)
- **Trunk & Bark (`ND_PLANT_BARK_FISSURED = 0`)**:
  - Massive, straight, columnar trunk with heavy buttressed root spurs (`root_flare_strength = 0.42`).
  - Thick, corky, deeply furrowed dark reddish-brown to charcoal-brown ridges (`RGB(44, 30, 22)` to `RGB(112, 82, 58)`).
- **Branching & Crown Habit**:
  - Tall, narrow, spire-like conical crown (`apical_dominance = 0.96`, `orthotropy = 0.96`) with 9 to 14 distinct pseudo-whorl tiers.
  - Upper whorls angle slightly upward (`65°–75°`), mid-crown whorls extend horizontally (`80°–90°`), and lower whorls droop downward (`100°–115°`) with upturned branchlet tips and weathered deadwood branch stubs below the live crown.
- **Foliage (`ND_MAT3D_FOL_CONIFER`, Spirally Arranged Bottle-Brush Needles)**:
  - Soft, flat linear needles (`2 – 3.5 cm`) radiating spirally around the twigs in dense, layered, pendulous-fringed sprays.
  - Deep forest-emerald colour (`RGB(14, 42, 26)` deep whorl interior to `RGB(82, 138, 86)` sunlit tips).
- **Cones**:
  - Pendulous oval cones (`5 – 10 cm`) with distinctive exserted 3-pointed bracts.
