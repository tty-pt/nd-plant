# *Acer pseudoplatanus* — Sycamore Maple (`ID 12`)

## 1. Taxonomic & Architectural Identity
- **Species ID**: `12` (`acer_pseudoplatanus`)
- **Scientific Name**: *Acer pseudoplatanus* L.
- **Common Name**: Sycamore Maple / Great Maple
- **Family**: Sapindaceae
- **Life Form**: Large Broadleaf Deciduous Canopy Tree (`PLANT_LIFEFORM_TREE`)
- **Hallé–Oldeman Architectural Model**: **Rauh's Model** (`PLANT_ARCH_RAUH`) with opposite decussate phyllotaxis (`PLANT_PHYLLOTAXIS_OPPOSITE_DECUSSATE`, `divergence_angle_deg = 90°`, `leaves_per_node = 2`) — vigorous orthotropic trunk and ascending opposite scaffold limbs forming a dense, symmetrical, ovoid-to-domed crown.
- **Primary Biomes**: Temperate Seasonal Forest (`12`), Woodland (`11`), Temperate Rainforest (`13`).

## 2. Real-World Botanical Dimensions & Proportional Scale (`1 m = 45.0 wu`, Lamb = `0.40 m = 18.0 wu`)

| Metric Parameter | Real-World Botanical Range | Representative Mature Value | Proportional to `40 cm` Lamb (`0.40 m`) | World Units (`1 m = 45.0 wu`) |
| :--- | :--- | :--- | :--- | :--- |
| **Mature Tree Height (`H_big`)** | `20.0 – 35.0 m` | **`24.0 m`** | **`60.0×` Lamb height** | **`1080 wu`** (`900 – 1575 wu`) |
| **Mature Crown Spread (`W_big`)** | `15.0 – 25.0 m` | **`20.0 m`** | **`50.0×` Lamb height** | **`900 wu`** (`675 – 1125 wu`) |
| **Juvenile / Sapling Height (`H_small`)** | `2.8 – 5.5 m` | **`4.0 m`** | **`10.0×` Lamb height** | **`180 wu`** (`126 – 248 wu`) |
| **Juvenile Crown Spread (`W_small`)** | `2.0 – 4.0 m` | **`3.0 m`** | **`7.5×` Lamb height** | **`135 wu`** (`90 – 180 wu`) |
| **Trunk DBH (Diameter at Breast Height)** | `0.80 – 1.80 m` | **`1.20 m`** | **`3.00×` Lamb height** | **`54.0 wu`** (`36.0 – 81.0 wu`) |
| **Root-Collar Basal Diameter** | `1.20 – 2.50 m` | **`1.75 m`** | **`4.38×` Lamb height** | **`78.8 wu`** |
| **Clear Bole Height** | `30% – 40%` of height (`6.0 – 12.0 m`) | **`32%` (`7.7 m`)** | **`19.2×` Lamb height** | **`346 wu`** |

## 3. Structural, Bark & Foliage Morphology (For 3D Mesh & Shader Definition)
- **Trunk & Bark (`ND_PLANT_BARK_FISSURED = 0`, Flaking Plate Bark)**:
  - Straight, columnar bole (`orthotropy = 0.80`) with smooth grey-buff bark in youth that exfoliates in irregular, curling pinkish-grey to warm tan polygonal plates (`RGB(54, 46, 40)` to `RGB(138, 122, 106)`) in maturity.
- **Branching & Crown Habit**:
  - Opposite decussate branching produces balanced pairs of ascending scaffold limbs (`branch_angle_deg = 48° ± 6°`) supporting a dense, tightly layered dome of overlapping 5-lobed leaf fans.
- **Foliage (`ND_MAT3D_FOL_LOBED`, 5-Lobed Palmate Leaves)**:
  - Large (`10 – 18 cm`), palmately 5-lobed leaves (`PLANT_LEAF_PALMATE_LOBED`, `PLANT_VENATION_PALMATE`) with coarse crenate-serrate margins on long red-tinged petioles.
  - Deep emerald to warm golden-green (`RGB(28, 64, 24)` interior shade to `RGB(154, 196, 86)` sunlit crown exterior), casting deep interior self-shadowing (`AO = 0.15–0.26`).
- **Fruit (Paired Winged Samaras)**:
  - Pendulous racemes of paired winged samaras ("helicopter keys") diverging at `60°–90°`.
