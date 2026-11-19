Create ONE SINGLE, botanically accurate, mature `<SPECIES_NAME>` specimen (tree, shrub, herb, grass, reed, fern, or clump, true to its botanical life-form) as a game sprite along with its matching 1:1 registered tangent-space normal map.
I am providing a reference sprite sheet containing pixel-art lambs (`lamb.png`). A SINGLE PIXEL ("TEXEL") OF AN UPRIGHT LAMB IN THAT IMAGE IS THE ABSOLUTE WORLD-SPACE UNIT OF MEASUREMENT. One pixel block on the lamb MUST equal the exact same real-world physical dimensions as one pixel block on `<SPECIES_NAME>`.

## 1. Botanical Identity & Multi-Scale Online Research
Before generating any image, research authoritative botanical data and reference photos of `<SPECIES_NAME>`:
- True botanical life-form & growth habit (e.g., canopy tree, shrub, herbaceous perennial, rosette, emergent reed/cattail clump, grass, fern). Never force a non-tree into a woody tree shape.
- Representative mature total height $H$ (metres) and total crown/clump width $W$ (metres).
- Exact physical width/thickness $d$ (in centimetres) of its individual organs:
  - Primary trunk, stem, or culm diameter (cm).
  - Branches, stalks, or petioles (cm).
  - Individual leaf blades, leaflets, or needles (width and length in cm).
  - Characteristic flowers, inflorescences, spikes, umbels, cones, pods, or fruit (width and length in cm).

## 2. Camera Perspective & 3/4 Top-Down Oblique Projection (~55° Pitch) — CRITICAL
Match the exact 16-bit RPG top-down oblique (~55° camera pitch looking down and north) projection of the supplied lamb sprites:
- Do NOT draw a flat 0° side-on elevation, and do NOT draw a 90° vertical bird's-eye view.
- Show true 3/4 front-to-back volumetric depth: the upper plane of the canopy, arching clump, or flower tier tilts toward the viewer, with foreground stems/foliage overlapping central and rear layers across an elliptical 3/4 footprint on the ground plane.

## 3. Strict World-Space Texel Lock (1 Lamb Texel = 1 Plant Texel = 2.222 cm) — CRITICAL
In the reference sheet, ONE upright standing lamb is **18 texels tall** and represents **0.40 metres (40 cm)** in the game world.
Therefore, the immutable world-space coordinate grid is:
- **1 texel (1 discrete pixel-art square) = 2.222 cm × 2.222 cm (22.2 mm) in reality.**
- **1 metre = 45 texels** (= 2.5 upright lambs stacked head-to-hoof).
- Anatomical benchmarks on the supplied lamb (use these to visually lock organ sizes!):
  - Lamb eye = `1 × 2 texels` (~2.2 × 4.4 cm)
  - Lamb leg thickness = `2 texels` (~4.4 cm)
  - Lamb head width = `7 texels` (~15.5 cm)
  - Full upright lamb height = `18 texels` (40.0 cm)

Perform and write out ALL of these exact calculations BEFORE generating the image:

### A. Exact Sprite Grid Dimensions (Total Texel Count)
- $\text{Lamb Multiplier} = H / 0.4\text{ m} = H \times 2.5$
- $\text{Grid Height } (H_{\text{tex}}) = \text{round}(H \times 45\text{ texels/m})$
- $\text{Grid Width } (W_{\text{tex}}) = \text{round}(W \times 45\text{ texels/m})$

### B. Anti-Auto-Zoom & Visible Pixel-Block Sizing (CRITICAL FOR IMAGE GENERATION)
Image generators naturally output high-resolution canvases (~1024×1024 px). DO NOT paint fine high-resolution details across a 1024px canvas for small plants!
- The plant MUST be constructed strictly on a virtual grid of **$W_{\text{tex}}$ columns × $H_{\text{tex}}$ rows** of uniform, indivisible, solid-colour square blocks ("texels").
- If $H_{\text{tex}}$ is small (e.g., a $0.8\text{ m}$ herb = $36\text{ texels}$ tall, or a $2.0\text{ m}$ reed = $90\text{ texels}$ tall), EVERY single texel on the canvas must appear as an unmistakably chunky, flat-coloured square block so that counting the discrete vertical stair-steps from the lowest root block to the highest tip block yields **approximately $H_{\text{tex}}$ discrete blocks**—NEVER 300–800 fine pixels!
- Conversely, if $H_{\text{tex}}$ is small ($< 60\text{ texels}$), leave generous `#FF00FF` magenta space around the specimen rather than zooming into a macro botanical close-up.
- If $H_{\text{tex}} > 220\text{ texels}$ (tall mature canopy trees $> 4.9\text{ m}$), fill ~85–92% of the canvas height cleanly using crisp, aliased 16-bit pixel-art clusters whose organ thicknesses strictly obey `round(d / 2.222)` relative to the total $W_{\text{tex}} \times H_{\text{tex}}$ bounding box.

### C. Mandatory Organ-Level Texel Clamping (Zero Cartoon Bloating)
Convert every organ's real width/thickness $d$ (cm) into lamb-equivalent texels: $\text{Organ Texels} = d / 2.222\text{ cm}$:
- **$d < 1.5\text{ cm}$ (`< 0.7 texels` — fine twigs, leaflets, needles, tiny florets):** NEVER draw individually with outlines. Merge into textured foliage clusters/umbel pads, OR draw thin solitary stalks as a **1-texel-wide** chain of single blocks.
- **$1.5\text{ cm} \le d \le 3.0\text{ cm}$ (`~1 texel` — narrow grass/sedge/cattail blades, slender stems, thin branches, small flower spikes, berries, pods, or cones):** Strictly **1 texel wide** (half the width of a lamb's leg!). NEVER put dark outline blocks on either side of a 1-texel organ (1 left outline + 1 fill + 1 right outline = 3 texels = $6.7\text{ cm}$ fencepost!).
- **1-Texel Accent-Organ Rule (Flowers, Spikes, Catkins, Berries, Pods, Cones with $d \le 3.0\text{ cm}$):**
  - Must be drawn as a **1-texel-thin (`1 px` wide)** dot or single-file chain of blocks—NEVER as a 2–3 texel wide blob, banana, or corn-cob.
  - **NEVER shade horizontally across the width of a 1-texel organ** (adding left highlight + midtone + right shadow inflates a 1-texel $2.2\text{ cm}$ organ into a 3-texel $6.7\text{ cm}$ cylinder!). Vary colour/value **only along its length** (tip-to-base), never across its width.
  - Keep small flowers/fruits sparse and partially nestled into the foliage texture rather than pasted on top like oversized ornaments.
- **$3.0\text{ cm} < d \le 10\text{ cm}$ (`2 to 4 texels` — cattail spikes, medium flower heads, wrist-thick limbs):** Strictly `round(d / 2.222)` texels wide **TOTAL (including edge pixels)**. Compare directly against the lamb's `2-texel` leg and `7-texel` head: nothing narrower than $15\text{ cm}$ in nature may ever approach the width of a lamb's head!
- **$d > 10\text{ cm}$ (`5+ texels` — mature tree trunks, primary boughs):** Strictly `round(d / 2.222)` texels wide.

## 4. Outline Discipline & Unlit Diffuse Albedo Shading (3D Day/Night & Normal Map Ready) — CRITICAL
- **ZERO `#000000` BLACK CARTOON OUTLINES:** Look at `lamb.png`: thin features are NOT wrapped in thick pure-black borders. Never wrap leaves, stems, spikes, or branches in black outlines. Separate overlapping 1–2 texel blades/stems solely via subtle local value/hue contrast (ambient occlusion), using dark desaturated species-tinted outer pixels only where a thick silhouette meets the background without inflating texel width.
- **UNLIT FLAT-DIFFUSE (ALBEDO) ONLY:** Do NOT bake directional sunlight (no top-left highlights or bottom-right cast shadows).
- **OMNIDIRECTIONAL AMBIENT OCCLUSION (AO) ONLY:** Darken only deep interior crevices, basal shoot bases, overlapping foliage undersides, and branch crotches so dynamic 3D day/night lights and point lights work cleanly with the normal map.
- Crisp, discrete, flat-coloured pixel blocks only: zero antialiasing, zero gradients, zero painterly/vector brushstrokes.

## 5. Composition & Pure Magenta Chroma-Key Background (`#FF00FF`)
- Render ONE complete specimen (or one natural single-crown clump for rhizomatous/tufted species) with clean basal contact (no grass turf, mud, water, soil patches, cast ground shadows, text, borders, or lambs in the output).
- **SOLID `#FF00FF` BACKGROUND:** Every background pixel outside the plant AND inside interior sky gaps between stems/branches must be pure flat `#FF00FF` (`RGB(255, 0, 255)`) with 100% hard-aliased razor-sharp block edges (zero pink feathering/antialiasing). Do NOT draw a checkerboard pattern.
- **STRICT `W_tex : H_tex` BOUNDING-BOX ASPECT RATIO:** Choose the supported image aspect ratio closest to `W_tex : H_tex` without stretching the specimen; pad the excess canvas axis with `#FF00FF` so the plant's active non-magenta bounding box itself strictly preserves `W : H` in metres.
- Never render any text or hex codes inside the image.

## 6. Tangent-Space Normal Map Specification (Paired `_n` Normal Map)
- **1:1 TEXEL REGISTRATION:** Every non-magenta block must align 1:1 with the diffuse sprite's exact texel grid; all background and interior sky-gap blocks MUST remain pure `#FF00FF` with zero black borders, zero dark indigo gap fill, and zero antialiased edges.
- **OPENGL RIGHT-HANDED TANGENT SPACE** (`RGB` $\to \vec{N} = \text{normalize}(\text{RGB}/127.5 - 1.0)$):
  - Red ($+X$ Right): `0` = left, `128` = centre, `255` = right.
  - Green ($+Y$ Up): `0` = underside overhang, `128` = vertical, `255` = sunward top surface.
  - Blue ($+Z$ Viewer): `128–255` (flat neutral = `#8080FF`).
- **TWO-SCALE 3/4 RELIEF (~55° Camera Pitch):** Combine macro 3/4 clump/crown volume with individual 3D curvature across trunks, foliage pads, spikes, and arching 1–2 texel leaf blades.
- **PER-TIER LOCAL DOME NORMALS (DO NOT CONFUSE ALBEDO AO WITH DOWNWARD `-Y` NORMALS) — CRITICAL:**
  - Rear, lower, or interior foliage pads that appear darker in the diffuse sprite are darker due to ambient occlusion (shadow), **NOT** because the entire pad faces downward!
  - **Every individual foliage pad, sub-crown, or leaf cluster—whether foreground, background, upper, or lower—must have its own complete local convex 3D dome normal gradient:**
    - **Upper 60–70% of EVERY pad** faces skyward/viewer (`+Y` and `+Z`: bright cyan/mint-green, `G = 180–255, B = 180–255`).
    - **Left-to-right across EVERY pad** rolls smoothly from `-X` (`R < 100`, cyan-blue) on the left to `+X` (`R > 160`, warm green/pink-blue) on the right.
    - **ONLY the lower 20–30% bottom underside fringe of each pad** faces downward (`-Y`: deep purple/indigo, `G = 0–80`).
  - Never paint an entire rear or lower foliage pad uniform dark purple.

SPECIES_NAME = <SPECIES_NAME>
