# `nd-plant` Botanical Species Dossiers & Proportional Scale Reference

This directory (`external/nd-plant/docs/species/`) contains the authoritative botanical, morphological, allometric, and proportional scale specifications for every tree species in `nd-plant` relative to the **40 cm (`0.40 m = 18.0 wu`) Lamb** (`1.00 m = 45.0 wu`).

---

## 1. Fundamental Metric & Proportional Scale Law

- **Reference Creature (The Lamb)**:
  - Standing height on four legs (hooves to head crown): **`0.40 m` (`40 cm` = `18.0 wu`)** (`ND_MESH3D_LAMB_BODY_WU = 18.0f`).
  - Conversion factor: **`1.00 m = 45.0 wu`** (`1.0 wu = 2.222 cm`, `ND_MESH3D_WU_PER_METER = 45.0f`).
- **Decoupled 3D World Stature vs. 2D Ortho Atlas Sprite Resolution**:
  - In the 3D scene (`s_vs_mesh3d` / `s_fs_mesh3d`), trees render at their true metric stature (`w_wu, h_wu`) while their far-zoom 2D orthographic fallback sprites (`sprite_w, sprite_h <= 248 px`) are baked with normalized orthographic scale (`px_per_wu = min(240.0 / w_wu, 240.0 / h_wu)`).

---

## 2. Canonical Tree Species Summary Table (Relative to the `0.40 m = 18.0 wu` Lamb)

| Species ID | Botanical Name (`slug`) | Common Name | Hallé–Oldeman Model | Real Mature Height (`m`) | Real Crown Spread (`m`) | Real Trunk DBH (`m`) | Clear Bole (`%`) | True Proportional Mature Height (`× 40 cm Lamb` / `wu`) | Dossier File |
| :- | :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **0** | *Pinus sylvestris* (`pinus_sylvestris`) | Scots Pine | `PLANT_ARCH_RAUH` | `20 – 35 m` | `7 – 12 m` | `0.50 – 1.20 m` | `55 – 70%` | **`50× – 87.5×` Lamb** (`900 – 1575 wu`) | [`pinus_sylvestris.md`](file:///home/quirinpa/site/external/nd-plant/docs/species/pinus_sylvestris.md) |
| **1** | *Pseudotsuga menziesii* (`pseudotsuga_menziesii`) | Coast Douglas-fir | `PLANT_ARCH_MASSART` | `40 – 75 m` | `10 – 18 m` | `1.50 – 2.50 m` | `18 – 35%` | **`100× – 187.5×` Lamb** (`1800 – 3375 wu`) | [`pseudotsuga_menziesii.md`](file:///home/quirinpa/site/external/nd-plant/docs/species/pseudotsuga_menziesii.md) |
| **2** | *Betula pendula* (`betula_pendula`) | Silver Birch | `PLANT_ARCH_TROLL` | `15 – 25 m` | `7 – 12 m` | `0.30 – 0.70 m` | `30 – 40%` | **`37.5× – 62.5×` Lamb** (`675 – 1125 wu`) | [`betula_pendula.md`](file:///home/quirinpa/site/external/nd-plant/docs/species/betula_pendula.md) |
| **4** | *Betula pubescens* (`betula_pubescens`) | Downy / Arctic Birch | `PLANT_ARCH_RAUH` | `10 – 20 m` | `6 – 10 m` | `0.25 – 0.50 m` | `25 – 35%` | **`25× – 50×` Lamb** (`450 – 900 wu`) | [`betula_pubescens.md`](file:///home/quirinpa/site/external/nd-plant/docs/species/betula_pubescens.md) |
| **5** | *Abies alba* (`abies_alba`) | European Silver Fir | `PLANT_ARCH_MASSART` | `35 – 55 m` | `9 – 15 m` | `1.00 – 1.80 m` | `14 – 25%` | **`87.5× – 137.5×` Lamb** (`1575 – 2475 wu`) | [`abies_alba.md`](file:///home/quirinpa/site/external/nd-plant/docs/species/abies_alba.md) |
| **7** | *Acacia senegal* (`acacia_senegal`) | Gum Arabic / Umbrella Thorn | `PLANT_ARCH_TROLL` | `5 – 12 m` | `8 – 16 m` | `0.30 – 0.60 m` | `35 – 45%` | **`12.5× – 30×` Lamb** (`225 – 540 wu`) | [`acacia_senegal.md`](file:///home/quirinpa/site/external/nd-plant/docs/species/acacia_senegal.md) |
| **10** | *Quercus robur* (`quercus_robur`) | English Pedunculate Oak | `PLANT_ARCH_RAUH` | `20 – 35 m` | `20 – 35 m` | `1.00 – 2.50 m` | `25 – 35%` | **`50× – 87.5×` Lamb** (`900 – 1575 wu`) | [`quercus_robur.md`](file:///home/quirinpa/site/external/nd-plant/docs/species/quercus_robur.md) |
| **11** | *Salix babylonica* (`salix_babylonica`) | Weeping Willow | `PLANT_ARCH_TROLL` | `12 – 20 m` | `12 – 22 m` | `0.60 – 1.20 m` | `25 – 35%` | **`30× – 50×` Lamb** (`540 – 900 wu`) | [`salix_babylonica.md`](file:///home/quirinpa/site/external/nd-plant/docs/species/salix_babylonica.md) |
| **12** | *Acer pseudoplatanus* (`acer_pseudoplatanus`) | Sycamore Maple | `PLANT_ARCH_RAUH` | `20 – 35 m` | `15 – 25 m` | `0.80 – 1.80 m` | `30 – 40%` | **`50× – 87.5×` Lamb** (`900 – 1575 wu`) | [`acer_pseudoplatanus.md`](file:///home/quirinpa/site/external/nd-plant/docs/species/acer_pseudoplatanus.md) |
| **13** | *Olea europaea* (`olea_europaea`) | European Olive | `PLANT_ARCH_LEEUWENBERG` | `6 – 12 m` | `7 – 14 m` | `0.50 – 1.50 m` | `25 – 35%` | **`15× – 30×` Lamb** (`270 – 540 wu`) | [`olea_europaea.md`](file:///home/quirinpa/site/external/nd-plant/docs/species/olea_europaea.md) |

---

## 3. Non-Tree Companion Species Reference (`1:1` Metric Scale to the 40 cm Lamb)

| Species ID | Botanical Name (`slug`) | Common Name | Life Form | Real Mature Height (`m`) | Real Spread (`m`) | True Proportional Height (`× 40 cm Lamb` / `wu`) |
| :- | :--- | :--- | :--- | :--- | :--- | :--- |
| **3** | *Linum usitatissimum* (`linum_usitatissimum`) | Common Flax | Herb | `0.60 – 1.00 m` | `0.30 – 0.50 m` | `1.5× – 2.5×` Lamb (`27 – 45 wu`, target `34 wu = 0.76 m`) |
| **6** | *Arthrocereus rondonianus* (`arthrocereus_rondonianus`) | Columnar Cactus | Succulent | `0.80 – 1.80 m` | `0.50 – 1.00 m` | `2.0× – 4.5×` Lamb (`36 – 81 wu`, target `58 wu = 1.29 m`) |
| **8** | *Daucus carota* (`daucus_carota`) | Wild Carrot / Queen Anne's Lace | Herb | `0.50 – 0.90 m` | `0.40 – 0.60 m` | `1.25× – 2.25×` Lamb (`22 – 40 wu`, target `32 wu = 0.71 m`) |
| **9** | *Solanum lycopersicum* (`solanum_lycopersicum`) | Wild Tomato Bush | Herb | `0.70 – 1.20 m` | `0.60 – 1.00 m` | `1.75× – 3.0×` Lamb (`31 – 54 wu`, target `38 wu = 0.84 m`) |
| **14** | *Bellis perennis* (`bellis_perennis`) | Common Meadow Daisy | Rosette Herb | `0.08 – 0.16 m` | `0.12 – 0.22 m` | `0.20× – 0.40×` Lamb (`3.6 – 7.2 wu`, target `6 wu = 0.13 m`) |
| **15** | *Typha latifolia* (`typha_latifolia`) | Broadleaf Cattail / Bulrush | Wetland Reed | `1.50 – 2.50 m` | `0.80 – 1.20 m` | `3.75× – 6.25×` Lamb (`68 – 112 wu`, target `76 wu = 1.69 m`) |
