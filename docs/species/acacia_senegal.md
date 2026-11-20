# *Acacia senegal* — Gum Arabic / Umbrella Thorn Acacia (`ID 7`)

## 1. Taxonomic & Architectural Identity
- **Species ID**: `7` (`acacia_senegal`)
- **Scientific Name**: *Senegalia senegal* (L.) Britton (syn. *Acacia senegal* (L.) Willd.)
- **Common Name**: Gum Arabic Tree / Savannah Umbrella Thorn
- **Family**: Fabaceae (Mimosoideae)
- **Life Form**: Deciduous / Semi-Evergreen Savanna Tree (`PLANT_LIFEFORM_TREE`)
- **Hallé–Oldeman Architectural Model**: **Troll's Model** (`PLANT_ARCH_TROLL`) with extreme acrotony (`acrotony = 0.85`) and low apical dominance (`apical_dominance = 0.25`) — trunk forks early into wide, crooked, plagiotropic diagonal limbs that spread horizontally into a broad, flat-topped umbrella canopy (`crown_width / height = 1.45`).
- **Primary Biomes**: Tropical Savannah (`15`), Tropical Seasonal Forest (`16`).

## 2. Real-World Botanical Dimensions & Proportional Scale (`1 m = 45.0 wu`, Lamb = `0.40 m = 18.0 wu`)

| Metric Parameter | Real-World Botanical Range | Representative Mature Value | Proportional to `40 cm` Lamb (`0.40 m`) | World Units (`1 m = 45.0 wu`) |
| :--- | :--- | :--- | :--- | :--- |
| **Mature Tree Height (`H_big`)** | `5.0 – 12.0 m` | **`8.5 m`** | **`21.25×` Lamb height** | **`382 wu`** (`225 – 540 wu`) |
| **Mature Crown Spread (`W_big`)** | `8.0 – 16.0 m` | **`12.4 m`** | **`31.0×` Lamb height** | **`558 wu`** (`360 – 720 wu`) |
| **Juvenile / Sapling Height (`H_small`)** | `1.6 – 3.5 m` | **`2.4 m`** | **`6.0×` Lamb height** | **`108 wu`** (`72 – 158 wu`) |
| **Juvenile Crown Spread (`W_small`)** | `2.0 – 4.5 m` | **`3.2 m`** | **`8.0×` Lamb height** | **`144 wu`** (`90 – 202 wu`) |
| **Trunk DBH (Diameter at Breast Height)** | `0.30 – 0.60 m` | **`0.45 m`** | **`1.12×` Lamb height** | **`20.2 wu`** (`13.5 – 27.0 wu`) |
| **Clear Bole Height (Under-Canopy Clearance)** | `35% – 45%` of height (`2.0 – 5.0 m`) | **`38%` (`3.2 m`)** | **`8.0×` Lamb height** | **`145 wu`** |

## 3. Structural, Bark & Foliage Morphology (For 3D Mesh & Shader Definition)
- **Trunk & Bark (`ND_PLANT_BARK_FISSURED = 0`)**:
  - Short, crooked, often leaning bole (`tortuosity = 0.28`) with sun-baked yellowish-brown to dark grey fissured bark (`RGB(58, 42, 26)` to `RGB(142, 108, 72)`).
  - Forks at `35%–45%` height into 3 to 5 wide-angled (`66°–84°`), sinuous scaffold limbs fully visible from beneath the high, flat canopy ceiling.
- **Branching & Crown Habit**:
  - Iconic **flat-topped East-African savannah tabletop silhouette**: secondary and tertiary branches fan out into horizontal, stratified, paper-thin foliage tables at the top of the crown (`z = 0.72 H .. 1.00 H`).
- **Foliage (`ND_MAT3D_FOL_OVATE` / Bipinnate Feathery Pinnae)**:
  - Finely divided bipinnate compound leaves (`PLANT_LEAF_BIPINNATE`) with tiny dusty olive-sage leaflets (`RGB(44, 72, 26)` to `RGB(172, 198, 92)`) that cast a lacy, sun-dappled shadow on the savannah grass below.
