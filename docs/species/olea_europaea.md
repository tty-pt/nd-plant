# *Olea europaea* — European / Mediterranean Olive (`ID 13`)

## 1. Taxonomic & Architectural Identity
- **Species ID**: `13` (`olea_europaea`)
- **Scientific Name**: *Olea europaea* L.
- **Common Name**: European Olive / Mediterranean Olive
- **Family**: Oleaceae
- **Life Form**: Evergreen Broadleaf Sclerophyll Tree (`PLANT_LIFEFORM_TREE`)
- **Hallé–Oldeman Architectural Model**: **Leeuwenberg's Model** (`PLANT_ARCH_LEEUWENBERG`) with opposite decussate phyllotaxis (`PLANT_PHYLLOTAXIS_OPPOSITE_DECUSSATE`) and high tortuosity (`tortuosity = 0.42`) — sympodial, multi-forking orthotropic modules that twist and gnarl with age into a wide, billowy, irregular crown over a deeply fluted, cavernous trunk.
- **Primary Biomes**: Mediterranean Scrub & Woodland (`4`), Dry Savannah Edge (`15`), Temperate Grassland Outcrop (`10`).

## 2. Real-World Botanical Dimensions & Proportional Scale (`1 m = 45.0 wu`, Lamb = `0.40 m = 18.0 wu`)

| Metric Parameter | Real-World Botanical Range | Representative Mature Value | Proportional to `40 cm` Lamb (`0.40 m`) | World Units (`1 m = 45.0 wu`) |
| :--- | :--- | :--- | :--- | :--- |
| **Mature Tree Height (`H_big`)** | `6.0 – 12.0 m` | **`8.5 m`** | **`21.25×` Lamb height** | **`382 wu`** (`270 – 540 wu`) |
| **Mature Crown Spread (`W_big`)** | `7.0 – 14.0 m` | **`10.2 m`** | **`25.5×` Lamb height** | **`459 wu`** (`315 – 630 wu`) |
| **Juvenile / Sapling Height (`H_small`)** | `1.8 – 3.5 m` | **`2.5 m`** | **`6.25×` Lamb height** | **`112 wu`** (`81 – 158 wu`) |
| **Juvenile Crown Spread (`W_small`)** | `1.8 – 3.6 m` | **`2.6 m`** | **`6.5×` Lamb height** | **`117 wu`** (`81 – 162 wu`) |
| **Trunk DBH (Fluted / Gnarled Bole)** | `0.50 – 1.50 m` | **`0.90 m`** | **`2.25×` Lamb height** | **`40.5 wu`** (`22.5 – 67.5 wu`) |
| **Root-Collar Basal Diameter** | `0.85 – 2.20 m` | **`1.40 m`** | **`3.50×` Lamb height** | **`63.0 wu`** |
| **Clear Bole Height** | `25% – 35%` of height (`1.8 – 3.8 m`) | **`28%` (`2.4 m`)** | **`6.0×` Lamb height** | **`108 wu`** |

## 3. Structural, Bark & Foliage Morphology (For 3D Mesh & Shader Definition)
- **Trunk & Bark (`ND_PLANT_BARK_FLUTED = 3`)**:
  - Squat, massive, twisted, and **deeply fluted** sculptural trunk (`root_flare_strength = 0.55`, `tortuosity = 0.42`) with cord-like spiraling sapwood ribs and dark hollow crevices.
  - Pale ash-grey to silvery-taupe bark (`RGB(58, 52, 44)` in deep flutes to `RGB(146, 136, 120)` on sun-bleached ridges).
- **Branching & Crown Habit**:
  - Short trunk divides at `25%–35%` height into 3–5 thick, contorted primary limbs (`branch_angle_deg = 58° ± 9°`) supporting distinct, asymmetrical, cloud-like foliage pads with open air windows between boughs.
- **Foliage (`ND_MAT3D_FOL_OVATE`, Sclerophyllous Lanceolate Silvery-Sage Leaves)**:
  - Opposite, leathery, lanceolate-elliptic leaves (`4 – 8 cm` long, `width_ratio = 0.26`) with entire margins (`PLANT_MARGIN_ENTIRE`).
  - Dusty grey-green / olive-sage adaxial surface (`RGB(42, 58, 36)` to `RGB(118, 144, 102)`) and **bright silvery-white peltate-scaly abaxial underside** (`RGB(168, 192, 152)`), giving the whole crown a shimmering Mediterranean silver-olive sheen in wind and sunlight.
- **Fruit (Drupes / Olives)**:
  - Ovoid drupes (`1.5 – 3.0 cm`) ripening from green to deep purple-black.
