#!/usr/bin/env python3
"""
gen_plant_sprite.py

Strictly spec-compliant botanical sprite generator (1 texel = 2.222 cm, 45 texels/m)
using Google AI Ultra (via Antigravity OAuth CLI `agy` or `pi`), enforcing exact
W_tex x H_tex real-pixel dimensions, 1:1 normal registration, and clean RGBA output.
"""

import argparse
import json
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path
import numpy as np
from PIL import Image


TEXELS_PER_METRE = 45.0  # 1 upright lamb = 18 texels = 0.40 m -> 1 texel = 2.222 cm


# ---------------------------------------------------------------------------
# Part 1: Headless Google AI Ultra Agent Execution + Metadata Handshake
# ---------------------------------------------------------------------------

AGENT_PIPELINE_CONTRACT = """
CRITICAL AUTOMATED PIPELINE CONTRACT (MUST FOLLOW EXACTLY):
1. Reference `lamb.png` if present for strict texel-scale and zero-black-outline discipline.
2. Complete Step 1 (Botanical Research) and Step 3 (Exact Texel Calculations: H_tex = round(H * 45), W_tex = round(W * 45)).
3. Write a valid JSON file to `{meta_json}` with these exact keys:
   {{
     "species": "{species}",
     "life_form": "<tree|shrub|herb|grass|reed|fern|clump>",
     "height_m": <float H in metres>,
     "width_m": <float W in metres>,
     "w_tex": <int round(W * 45)>,
     "h_tex": <int round(H * 45)>,
     "has_purple_flowers": <true|false>
   }}
4. Choose the supported `aspect_ratio` ('1:1', '3:4', '2:3', '4:5', '5:4', '4:3', '3:2', '9:16') closest to `w_tex / h_tex` WITHOUT stretching the plant: pad the excess axis with pure `#FF00FF` magenta so the plant's active silhouette itself preserves the exact `W : H` physical aspect ratio!
   - If `max(w_tex, h_tex) <= 180`, explicitly enforce coarse, unmistakably chunky square blocks (`~H_tex` discrete vertical blocks across the plant height) with generous `#FF00FF` margin so the generator does NOT zoom into a 500-pixel macro painting.
   - If `max(w_tex, h_tex) > 220` (tall mature trees), fill ~85-92% of the canvas height cleanly with fine, crisp 16-bit pixel-art clusters so organ widths match `round(d_cm / 2.222)` at `W_tex x H_tex`.
5. Call `generate_image` FIRST for the unlit diffuse albedo sprite on pure `#FF00FF` magenta background and copy/save the unmodified raw output file to:
   `{raw_diffuse}`
6. Call `generate_image` SECOND—passing the first generated image in `image_references`—for the 1:1 registered OpenGL right-handed tangent-space normal map on pure `#FF00FF` magenta background with the SAME `aspect_ratio`, and copy/save the unmodified raw output file to:
   `{raw_normal}`
7. Do NOT resize, crop, or convert `{raw_diffuse}` or `{raw_normal}` yourself.
"""


def slugify(species: str) -> str:
    return re.sub(r"[^a-z0-9]+", "_", species.strip().lower()).strip("_")


def run_ultra_agent(
    species: str,
    template_path: Path,
    lamb_path: Path | None,
    raw_diffuse: Path,
    raw_normal: Path,
    meta_json: Path,
    runner: str = "agy",
    model: str | None = None,
    extra_feedback: str = "",
) -> dict:
    if not template_path.exists():
        raise FileNotFoundError(f"Prompt template not found: {template_path}")

    prompt_body = template_path.read_text(encoding="utf-8")
    if "<SPECIES_NAME>" in prompt_body:
        prompt_body = prompt_body.replace("<SPECIES_NAME>", species)
    else:
        prompt_body += f"\n\nSPECIES_NAME = {species}\n"

    if lamb_path and lamb_path.exists():
        prompt_body = f"@{lamb_path.as_posix()}\n" + prompt_body

    full_prompt = prompt_body + "\n" + AGENT_PIPELINE_CONTRACT.format(
        species=species,
        meta_json=meta_json.as_posix(),
        raw_diffuse=raw_diffuse.as_posix(),
        raw_normal=raw_normal.as_posix(),
    )
    if extra_feedback:
        full_prompt += f"\n\nRETRY SCALE CORRECTION:\n{extra_feedback}\n"

    env = dict(os.environ)
    if runner == "agy":
        if not shutil.which("agy"):
            raise RuntimeError("`agy` CLI not found in PATH. Install it or pass --runner pi.")
        # Prevent GEMINI_API_KEY from hijacking Google AI Ultra OAuth billing
        env.pop("GEMINI_API_KEY", None)
        cmd = ["agy", "-p", full_prompt, "--dangerously-skip-permissions"]
        if model:
            cmd.extend(["--model", model])
    elif runner == "pi":
        if not shutil.which("pi"):
            raise RuntimeError("`pi` CLI not found in PATH.")
        pi_model = model or "antigravity-acp/gemini-3.8-flash"
        cmd = ["pi", "-p", "--model", pi_model, full_prompt]
    else:
        raise ValueError(f"Unsupported runner: {runner}")

    print(f"[ultra] Running {runner} for '{species}' via Google AI Ultra OAuth...")
    proc = subprocess.run(cmd, env=env, text=True)
    if proc.returncode != 0:
        raise RuntimeError(f"{runner} exited with non-zero code {proc.returncode}")

    if not raw_diffuse.exists():
        raise FileNotFoundError(f"Missing raw diffuse output: {raw_diffuse}")
    if not meta_json.exists():
        raise FileNotFoundError(f"Missing botanical metadata JSON: {meta_json}")

    return load_and_verify_meta(meta_json)


def load_and_verify_meta(meta_json: Path) -> dict:
    meta = json.loads(meta_json.read_text(encoding="utf-8"))
    h_m = float(meta["height_m"])
    w_m = float(meta["width_m"])
    if h_m <= 0 or w_m <= 0:
        raise ValueError(f"Invalid botanical dimensions in {meta_json}: H={h_m}m, W={w_m}m")

    # Enforce strict arithmetic lock: 1 m = 45 texels (1 texel = 2.222 cm)
    exact_h_tex = max(1, int(round(h_m * TEXELS_PER_METRE)))
    exact_w_tex = max(1, int(round(w_m * TEXELS_PER_METRE)))

    if meta.get("h_tex") != exact_h_tex or meta.get("w_tex") != exact_w_tex:
        print(
            f"[spec-lock] Corrected agent texel rounding ({meta.get('w_tex')}x{meta.get('h_tex')}) "
            f"-> exact ({exact_w_tex}x{exact_h_tex}) for W={w_m}m, H={h_m}m"
        )
    meta["w_tex"] = exact_w_tex
    meta["h_tex"] = exact_h_tex
    meta["lamb_multiplier"] = round(h_m / 0.40, 3)
    return meta


# ---------------------------------------------------------------------------
# Part 2: Chroma Keying, FFT Block Extraction & Direct World-Space Sampling
# ---------------------------------------------------------------------------

def sanitize_corner(im: np.ndarray, size: int = 80) -> np.ndarray:
    """Clear bottom-right generator watermark inside empty #FF00FF margin."""
    out = im.copy()
    if out.shape[0] > size * 2 and out.shape[1] > size * 2:
        out[-size:, -size:] = [255, 0, 255]
    return out


def compute_raw_fg_mask(im: np.ndarray, dist_thresh: float = 95.0, purple_bg_thresh: float = 52.0) -> np.ndarray:
    R = im[:, :, 0].astype(np.float64)
    G = im[:, :, 1].astype(np.float64)
    B = im[:, :, 2].astype(np.float64)
    purple_excess = np.minimum(R - G, B - G)
    dist_mag = np.sqrt((R - 255.0) ** 2 + G ** 2 + (B - 255.0) ** 2)
    return ~((dist_mag < dist_thresh) | (purple_excess > purple_bg_thresh))


def find_active_bbox(fg_mask: np.ndarray) -> tuple[int, int, int, int]:
    """Return tight (x0, y0, x1_excl, y1_excl) of foreground plant pixels."""
    ys, xs = np.where(fg_mask)
    if len(xs) == 0:
        raise ValueError("No non-magenta plant pixels detected in diffuse image!")
    return int(xs.min()), int(ys.min()), int(xs.max()) + 1, int(ys.max()) + 1


def detect_coarse_period(profile: np.ndarray, min_p: float = 2.5, max_p: float = 32.0) -> float:
    centered = profile - profile.mean()
    spectrum = np.abs(np.fft.rfft(centered))
    freqs = np.fft.rfftfreq(len(centered))
    valid = (freqs >= 1.0 / max_p) & (freqs <= 1.0 / min_p)
    if not np.any(valid):
        raise ValueError("Could not detect a valid pixel grid frequency.")
    return float(1.0 / freqs[valid][np.argmax(spectrum[valid])])


def fit_subpixel_grid(profile: np.ndarray, p_est: float) -> tuple[float, float]:
    best_score, best_p, best_o = -1.0, p_est, 0.0
    for half_win, p_steps, o_steps in [(0.15, 301, 100), (0.015, 301, 200)]:
        p_min = max(2.0, best_p - half_win)
        p_max = best_p + half_win
        for p in np.linspace(p_min, p_max, p_steps):
            for o in np.linspace(0.0, p, o_steps, endpoint=False):
                n_steps = int((len(profile) - 1 - o) / p) + 1
                if n_steps < 4:
                    continue
                pos = o + np.arange(n_steps) * p
                pos = pos[(pos >= 0.0) & (pos < len(profile) - 1.0001)]
                i0 = np.floor(pos).astype(np.int32)
                frac = pos - i0
                score = float(((1.0 - frac) * profile[i0] + frac * profile[i0 + 1]).mean())
                if score > best_score:
                    best_score, best_p, best_o = score, float(p), float(o)
    return best_p, best_o + 0.5


def build_cell_bounds(length: int, pitch: float, first_boundary: float, min_sliver: float = 1.8) -> list[float]:
    b0 = first_boundary % pitch
    edges = [0.0] if b0 >= min_sliver else []
    pos = b0
    while pos < length - min_sliver:
        edges.append(pos)
        pos += pitch
    edges.append(float(length))
    if edges[0] > 0.0:
        edges[0] = 0.0
    return edges


def sample_core_from_bounds(im: np.ndarray, x_bounds: np.ndarray | list[float], y_bounds: np.ndarray | list[float]) -> np.ndarray:
    """
    Sample ONLY the tight central core of each cell via channel-wise median.
    Works identically for both FFT block intervals (Regime A) and exact
    W_tex x H_tex bounding-box subdivisions (Regime B) with zero bilinear blur.
    """
    H, W, _ = im.shape
    ny = len(y_bounds) - 1
    nx = len(x_bounds) - 1
    out = np.zeros((ny, nx, 3), dtype=np.uint8)

    for r in range(ny):
        span_y = y_bounds[r + 1] - y_bounds[r]
        yc = 0.5 * (y_bounds[r] + y_bounds[r + 1]) - 0.5
        if span_y >= 3.5:
            y0 = max(0, int(np.floor(yc)))
            y1 = min(H, y0 + 2)
        else:
            y0 = min(H - 1, max(0, int(np.round(yc))))
            y1 = y0 + 1

        for c in range(nx):
            span_x = x_bounds[c + 1] - x_bounds[c]
            xc = 0.5 * (x_bounds[c] + x_bounds[c + 1]) - 0.5
            if span_x >= 3.5:
                x0 = max(0, int(np.floor(xc)))
                x1 = min(W, x0 + 2)
            else:
                x0 = min(W - 1, max(0, int(np.round(xc))))
                x1 = x0 + 1

            patch = im[y0:y1, x0:x1].reshape(-1, 3)
            out[r, c] = np.round(np.median(patch, axis=0)).astype(np.uint8)

    return out


def resample_discrete_nearest(arr: np.ndarray, target_w: int, target_h: int) -> np.ndarray:
    """Center-sampled nearest-texel lock onto exact (target_h, target_w) with zero antialiasing."""
    src_h, src_w = arr.shape[:2]
    if (src_w, src_h) == (target_w, target_h):
        return arr
    r_idx = np.clip(np.floor((np.arange(target_h) + 0.5) * src_h / target_h).astype(np.int32), 0, src_h - 1)
    c_idx = np.clip(np.floor((np.arange(target_w) + 0.5) * src_w / target_w).astype(np.int32), 0, src_w - 1)
    return arr[np.ix_(r_idx, c_idx)]


def despill_foreground(
    g_diff: np.ndarray,
    fg_mask: np.ndarray,
    despill_thresh: float = 6.0,
    preserve_purple: bool = False,
) -> np.ndarray:
    if preserve_purple:
        return g_diff.copy()

    R = g_diff[:, :, 0].astype(np.float64)
    G = g_diff[:, :, 1].astype(np.float64)
    B = g_diff[:, :, 2].astype(np.float64)
    purple_excess = np.minimum(R - G, B - G)

    despilled = g_diff.astype(np.float64).copy()
    spill_mask = fg_mask & (purple_excess > despill_thresh)

    bright_spill = spill_mask & (G >= 105.0)
    despilled[bright_spill, 0] = G[bright_spill] + np.minimum(6.0, (R[bright_spill] - G[bright_spill]) * 0.15)
    despilled[bright_spill, 2] = G[bright_spill] + np.minimum(4.0, (B[bright_spill] - G[bright_spill]) * 0.10)

    dark_spill = spill_mask & (G < 105.0)
    despilled[dark_spill, 0] = np.clip(R[dark_spill] - 0.65 * purple_excess[dark_spill], G[dark_spill], 255.0)
    despilled[dark_spill, 2] = np.minimum(B[dark_spill], G[dark_spill] * 0.82)

    return np.clip(np.round(despilled), 0, 255).astype(np.uint8)


def pad_transparent_rgb(rgb: np.ndarray, fg_mask: np.ndarray, iterations: int = 2) -> np.ndarray:
    padded = np.zeros_like(rgb)
    padded[fg_mask] = rgb[fg_mask]
    filled = fg_mask.copy()
    shifts = [(-1, 0), (1, 0), (0, -1), (0, 1), (-1, -1), (-1, 1), (1, -1), (1, 1)]

    for _ in range(iterations):
        acc = np.zeros((rgb.shape[0], rgb.shape[1], 3), dtype=np.int32)
        count = np.zeros((rgb.shape[0], rgb.shape[1]), dtype=np.int32)
        for dy, dx in shifts:
            sm = np.roll(np.roll(filled, dy, axis=0), dx, axis=1)
            sr = np.roll(np.roll(padded, dy, axis=0), dx, axis=1)
            if dy == -1: sm[-1, :] = False
            elif dy == 1: sm[0, :] = False
            if dx == -1: sm[:, -1] = False
            elif dx == 1: sm[:, 0] = False
            target = (~filled) & sm
            acc[target] += sr[target]
            count[target] += 1
        newly = (~filled) & (count > 0)
        if not np.any(newly):
            break
        padded[newly] = (acc[newly] // count[newly, None]).astype(np.uint8)
        filled |= newly
    return padded


def process_normal_map(g_norm: np.ndarray, fg_mask: np.ndarray, min_nz: float = 0.08) -> np.ndarray:
    dist_mag_norm = np.sqrt(((g_norm.astype(np.float64) - np.array([255.0, 0.0, 255.0])) ** 2).sum(axis=2))
    missing = fg_mask & (dist_mag_norm < 75.0)
    repaired = g_norm.copy()
    if missing.any():
        valid = fg_mask & (~missing)
        vy, vx = np.where(valid)
        for r, c in zip(*np.where(missing)):
            if len(vy) > 0:
                d2 = (vy - r) ** 2 + (vx - c) ** 2
                idx = int(np.argmin(d2))
                repaired[r, c] = g_norm[vy[idx], vx[idx]] if d2[idx] <= 9 else [128, 128, 255]
            else:
                repaired[r, c] = [128, 128, 255]

    fg_rgb = repaired[fg_mask].astype(np.float64)
    nx_v = fg_rgb[:, 0] / 127.5 - 1.0
    ny_v = fg_rgb[:, 1] / 127.5 - 1.0
    nz_v = np.maximum(min_nz, fg_rgb[:, 2] / 127.5 - 1.0)
    lengths = np.sqrt(nx_v ** 2 + ny_v ** 2 + nz_v ** 2)
    return np.clip(
        np.round((np.stack([nx_v / lengths, ny_v / lengths, nz_v / lengths], axis=1) + 1.0) * 127.5),
        0, 255
    ).astype(np.uint8)


def extract_world_locked_rgba(
    diffuse_path: Path,
    normal_path: Path | None,
    out_prefix: Path,
    w_tex: int,
    h_tex: int,
    has_purple_flowers: bool = False,
) -> tuple[int, int]:
    """
    Reconstructs clean 1:1 RGBA diffuse + normal PNGs strictly locked to (w_tex, h_tex).
    Returns (painted_block_w, painted_block_h) detected prior to exact grid lock for diagnostics.
    """
    purple_bg_thresh = 110.0 if has_purple_flowers else 52.0
    im_diff = sanitize_corner(np.array(Image.open(diffuse_path).convert("RGB")))
    Hd, Wd, _ = im_diff.shape

    im_norm = None
    if normal_path and normal_path.exists():
        im_norm = sanitize_corner(np.array(Image.open(normal_path).convert("RGB")))

    raw_fg = compute_raw_fg_mask(im_diff, purple_bg_thresh=purple_bg_thresh)
    x0, y0, x1, y1 = find_active_bbox(raw_fg)

    # Normalized [0..1] active plant bounding box so normal map registers even if raw dims differ
    u0, u1 = x0 / Wd, x1 / Wd
    v0, v1 = y0 / Hd, y1 / Hd

    # Branch by physical world-space texel count:
    # Regime A (<= 220 texels): coarse painted pixel blocks (> ~4.5 canvas px/block) -> FFT block lock first
    # Regime B (> 220 texels, tall mature trees): fine pixel clusters -> subdivide active bbox directly into w_tex x h_tex
    expected_pitch = min((x1 - x0) / max(1, w_tex), (y1 - y0) / max(1, h_tex))
    use_fft_regime = (max(w_tex, h_tex) <= 220) and (expected_pitch >= 3.2)

    if use_fft_regime:
        pad = 20
        rx0, ry0 = max(0, x0 - pad), max(0, y0 - pad)
        rx1, ry1 = min(Wd, x1 + pad), min(Hd, y1 + pad)
        dx_roi = np.abs(np.diff(im_diff[ry0:ry1, rx0:rx1].astype(np.float64), axis=1)).max(axis=2).mean(axis=0)
        dy_roi = np.abs(np.diff(im_diff[ry0:ry1, rx0:rx1].astype(np.float64), axis=0)).max(axis=2).mean(axis=1)

        px, bx0_roi = fit_subpixel_grid(dx_roi, detect_coarse_period(dx_roi))
        py, by0_roi = fit_subpixel_grid(dy_roi, detect_coarse_period(dy_roi))
        x_bounds = build_cell_bounds(Wd, px, rx0 + bx0_roi)
        y_bounds = build_cell_bounds(Hd, py, ry0 + by0_roi)

        g_diff_blocks = sample_core_from_bounds(im_diff, x_bounds, y_bounds)
        fg_blocks = compute_raw_fg_mask(g_diff_blocks, purple_bg_thresh=purple_bg_thresh)
        bx0, by0, bx1, by1 = find_active_bbox(fg_blocks)

        diff_active = g_diff_blocks[by0:by1, bx0:bx1]
        fg_active = fg_blocks[by0:by1, bx0:bx1]
        painted_h, painted_w = fg_active.shape

        # Lock active painted block grid strictly onto exact world-space (w_tex, h_tex)
        g_diff_locked = resample_discrete_nearest(diff_active, w_tex, h_tex)
        fg_locked = resample_discrete_nearest(fg_active.astype(np.uint8), w_tex, h_tex).astype(bool)

        if im_norm is not None:
            Hn, Wn, _ = im_norm.shape
            nx_bounds = [b * (Wn / Wd) for b in x_bounds]
            ny_bounds = [b * (Hn / Hd) for b in y_bounds]
            g_norm_blocks = sample_core_from_bounds(im_norm, nx_bounds, ny_bounds)[by0:by1, bx0:bx1]
            g_norm_locked = resample_discrete_nearest(g_norm_blocks, w_tex, h_tex)
        else:
            g_norm_locked = None
    else:
        # Regime B: Subdivide the exact active plant bounding box into w_tex columns x h_tex rows
        x_bounds = np.linspace(x0, x1, w_tex + 1)
        y_bounds = np.linspace(y0, y1, h_tex + 1)
        g_diff_locked = sample_core_from_bounds(im_diff, x_bounds, y_bounds)
        fg_locked = compute_raw_fg_mask(g_diff_locked, purple_bg_thresh=purple_bg_thresh)
        painted_w, painted_h = w_tex, h_tex

        if im_norm is not None:
            Hn, Wn, _ = im_norm.shape
            nx_bounds = np.linspace(u0 * Wn, u1 * Wn, w_tex + 1)
            ny_bounds = np.linspace(v0 * Hn, v1 * Hn, h_tex + 1)
            g_norm_locked = sample_core_from_bounds(im_norm, nx_bounds, ny_bounds)
        else:
            g_norm_locked = None

    g_diff_clean = despill_foreground(g_diff_locked, fg_locked, preserve_purple=has_purple_flowers)
    alpha = np.where(fg_locked, 255, 0).astype(np.uint8)

    out_prefix.parent.mkdir(parents=True, exist_ok=True)
    diff_rgba = np.zeros((h_tex, w_tex, 4), dtype=np.uint8)
    diff_rgba[:, :, :3] = pad_transparent_rgb(g_diff_clean, fg_locked, iterations=2)
    diff_rgba[:, :, 3] = alpha
    Image.fromarray(diff_rgba, mode="RGBA").save(f"{out_prefix}.png", format="PNG", optimize=True)

    if g_norm_locked is not None:
        fg_norm_u8 = process_normal_map(g_norm_locked, fg_locked)
        norm_rgba = np.zeros((h_tex, w_tex, 4), dtype=np.uint8)
        norm_rgba[~fg_locked, :3] = [128, 128, 255]  # Neutral +Z normal in empty alpha=0 space
        norm_rgba[fg_locked, :3] = fg_norm_u8
        norm_rgba[:, :, 3] = alpha
        Image.fromarray(norm_rgba, mode="RGBA").save(f"{out_prefix}_n.png", format="PNG", optimize=True)

    return painted_w, painted_h


def main() -> int:
    ap = argparse.ArgumentParser(description="Strictly spec-compliant 1:1 world-space RGBA botanical sprite pipeline.")
    ap.add_argument("species", help="Scientific species name, e.g. 'Betula pendula' or 'Daucus carota'")
    ap.add_argument("-t", "--template", type=Path, default=Path("prompt_template.md"), help="Botanical spec markdown file")
    ap.add_argument("-l", "--lamb", type=Path, default=Path("./../nd-seven/resources/lamb.png"), help="Reference lamb.png sprite")
    ap.add_argument("-o", "--out-dir", type=Path, default=Path("assets/plants"), help="Output directory for RGBA PNGs")
    ap.add_argument("--runner", choices=["agy", "pi"], default="agy", help="Ultra-authenticated CLI (default: agy)")
    ap.add_argument("--model", default=None, help="Optional model override (e.g. antigravity/gemini-4-argon)")
    ap.add_argument("--max-retries", type=int, default=1, help="Auto-retry if Regime A painted block count drifts > 60%% from H_tex")
    ap.add_argument("--reprocess-only", action="store_true", help="Skip AI generation and re-extract existing _raw files")
    args = ap.parse_args()

    slug = slugify(args.species)
    staging = args.out_dir / "_raw"
    staging.mkdir(parents=True, exist_ok=True)
    raw_diff = staging / f"{slug}_raw_diffuse.jpg"
    raw_norm = staging / f"{slug}_raw_normal.jpg"
    meta_json = staging / f"{slug}_meta.json"
    out_prefix = args.out_dir / slug

    if args.reprocess_only:
        meta = load_and_verify_meta(meta_json)
        extract_world_locked_rgba(
            raw_diff, raw_norm, out_prefix, meta["w_tex"], meta["h_tex"], bool(meta.get("has_purple_flowers", False))
        )
        print(f"[done] Reprocessed {out_prefix}.png (+ _n.png) at exact {meta['w_tex']}x{meta['h_tex']} px.")
        return 0

    feedback = ""
    for attempt in range(args.max_retries + 1):
        meta = run_ultra_agent(
            species=args.species,
            template_path=args.template,
            lamb_path=args.lamb if args.lamb.exists() else None,
            raw_diffuse=raw_diff,
            raw_normal=raw_norm,
            meta_json=meta_json,
            runner=args.runner,
            model=args.model,
            extra_feedback=feedback,
        )

        w_tex, h_tex = meta["w_tex"], meta["h_tex"]
        painted_w, painted_h = extract_world_locked_rgba(
            diffuse_path=raw_diff,
            normal_path=raw_norm,
            out_prefix=out_prefix,
            w_tex=w_tex,
            h_tex=h_tex,
            has_purple_flowers=bool(meta.get("has_purple_flowers", False)),
        )

        # In Regime A (small plants), verify the AI didn't paint a 150-block macro close-up for a 36-texel herb
        ratio_h = painted_h / max(1, h_tex)
        if max(w_tex, h_tex) <= 220 and (ratio_h > 1.60 or ratio_h < 0.60) and attempt < args.max_retries:
            feedback = (
                f"Previous generation painted ~{painted_w}x{painted_h} discrete pixel blocks, "
                f"which violates the target {w_tex}x{h_tex} texel grid ({meta['width_m']}m x {meta['height_m']}m). "
                f"Draw MUCH {'larger/chunkier pixel blocks with wider #FF00FF margins' if ratio_h > 1.6 else 'finer pixel blocks'} "
                f"so the plant spans ~{h_tex} discrete vertical blocks!"
            )
            print(f"[retry {attempt+1}/{args.max_retries}] {feedback}")
            raw_diff.unlink(missing_ok=True)
            raw_norm.unlink(missing_ok=True)
            continue

        print(
            f"[done] {args.species} ({meta['width_m']}m x {meta['height_m']}m = {meta['lamb_multiplier']}x lamb) -> "
            f"exact {w_tex}x{h_tex} real RGBA px saved to {out_prefix}.png & {out_prefix}_n.png"
        )
        break

    return 0


if __name__ == "__main__":
    sys.exit(main())
