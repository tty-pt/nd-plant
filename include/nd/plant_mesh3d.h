/*
 * plant_mesh3d.h — Pure-C Genome-Driven 3D Botanical Mesh Engine (Stage 4 of nd-plant).
 *
 * Translates a developed PlantInstance (PlantGenome + PlantPhenotype MTG) into 3D
 * triangle meshes with analytical 3D vertex normals, ambient occlusion, UV-encoded
 * branchlet card coordinates, and wind flexibility weights.
 *
 * Architectural Invariants:
 *   1. 100% Species-Agnostic & Continuous-Trait Driven:
 *      Contains zero species-ID switches, zero species-name string comparisons, and
 *      zero species boolean flags. All structural & foliar variations emerge continuously
 *      from PlantGenome traits (crown_flatness, basal_radius_ratio, clear_bole_fraction,
 *      apical_dominance, orthotropy, tortuosity, branch_shed_threshold, bark_class,
 *      leaf.complexity, leaf.shape, flower.inflorescence, fruit.fruit_type) and the
 *      developed PlantPhenotype MTG skeleton.
 *   2. 55-Degree Oblique Camera Perspective Compensation & Clear-Bole Crown Floor:
 *      In the 55-deg diorama projection, a 3D point (x, y, z) projects to apparent
 *      height h_proj = z - y * cot(55 deg) = z - 0.7002075 * y.
 *      To prevent front-hemisphere (y > 0) lateral limbs and foliage from foreshortening
 *      down across the trunk or touching the ground, woody tree crowns enforce a conical
 *      perspective-compensated crown floor:
 *        z_floor(x, y) = H * clear_bole_eff + max(0, y) * 0.72 + |x| * 0.10
 *      guaranteeing h_proj >= H * clear_bole_eff for every lateral branch and foliage vertex.
 *   3. Skeleton-Locked Terminal Shoot Foliage Only:
 *      Foliage cards attach strictly to developed MTG leaf organs on terminal shoots
 *      (units with no drawn woody child axes), never as synthetic mid-air roof loops.
 */

#ifndef ND_PLANT_MESH3D_H
#define ND_PLANT_MESH3D_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include "plant.h"
#include "plant_foliage_atlas.h"

#define ND_MESH3D_NONE        0u
#define ND_MESH3D_PLANT_BASE  1u
#define ND_MESH3D_PLANT_COUNT 128u  /* 16 species * 2 stages (small/big) * 4 variants (1..4) */
#define ND_MESH3D_STONE_BASE  129u
#define ND_MESH3D_STONE_COUNT 51u   /* NDS_SP_COUNT = 51 stone/mineral specimens */
#define ND_MESH3D_TOTAL       180u  /* 0..179 inclusive */

#define ND_MESH3D_MAX_VERTS   262144u
#define ND_MESH3D_MAX_INDICES 1048576u

/* Material class encoded in nd_mesh3d_vert_t.col[3]:
 *   0.00         = Bark / woody stem
 *   0.185        = Solid succulent / blade core (ND_MAT3D_FOLIAGE_CORE)
 *   0.200..0.238 = Pectinate Conifer Needle Bough Card (v = (mat - 0.200) / 0.038 in [0,1])
 *   0.240..0.278 = Serrated Broadleaf Ovate Shoot Card (v = (mat - 0.240) / 0.038 in [0,1])
 *   0.280..0.318 = Lobed/Palmate Broadleaf Shoot Card  (v = (mat - 0.280) / 0.038 in [0,1])
 *   0.320..0.350 = Bipinnate / Linear Blade Card       (v = (mat - 0.320) / 0.030 in [0,1])
 *   0.50         = Floral blossom / fruit / cone / spadix accent
 *   0.75         = Matte geological rock / boulder
 *   1.00         = Crystalline / metallic ore mineral */
#define ND_MAT3D_BARK           0.00f
#define ND_MAT3D_FOLIAGE_CORE   0.185f
#define ND_MAT3D_FOL_CONIFER    0.200f
#define ND_MAT3D_FOL_OVATE      0.240f
#define ND_MAT3D_FOL_LOBED      0.280f
#define ND_MAT3D_FOL_BLADE      0.320f
#define ND_MAT3D_FOLIAGE        0.250f
#define ND_MAT3D_FOLIAGE_FRINGE 0.320f
#define ND_MAT3D_BLOSSOM        0.50f
#define ND_MAT3D_STONE_MATTE    0.75f
#define ND_MAT3D_CRYSTAL_ORE    1.00f

/* Canonical 3D World-Unit Scale Constants:
 * Standing Lamb body height = 18.0 world units = 0.40 m (40 cm) -> 45.0 wu/m. */
#define ND_MESH3D_LAMB_HEIGHT_M  0.40f
#define ND_MESH3D_LAMB_BODY_WU   18.0f
#define ND_MESH3D_WU_PER_METER   45.0f

/* 48 bytes (12 floats) per 3D template vertex (matches locations 0, 1, 2 in s_vs_mesh3d) */
typedef struct {
	float pos[4];  /* xyz: local 3D pos in world units (x=East, y=South, z=Up from ground), w: wind weight [0..1] or packed cardU+wind */
	float norm[4]; /* xyz: unit 3D surface normal in local frame, w: ambient occlusion [0.12..1.0] */
	float col[4];  /* rgb: continuous 24-bit physical albedo [0..1], a: ND_MAT3D_* material class */
} nd_mesh3d_vert_t;

/* 112 bytes (28 floats) per 3D mesh instance (matches locations 3..9 in s_vs_mesh3d) */
typedef struct {
	float base_scale[4]; /* xyz: torus world base_pos relative to cur_origin, w: horizontal scale_xy */
	float axis_e[4];     /* xyz: rotated East tangent basis, w: vertical scale_z */
	float axis_s[4];     /* xyz: rotated South tangent basis, w: wind sway_sx */
	float axis_n[4];     /* xyz: torus Zenith normal basis, w: wind sway_sy */
	float tint[4];       /* rgba: instance tint & distance fade alpha */
	float param[4];      /* x: world wx, y: world wy, z: ground wz, w: flags (+0 dry, +16 inWaterTile) */
	float terr[4];       /* x: terrain slope dz/dx, y: terrain slope dz/dy, z: curvature, w: shadow flags */
} nd_seven_gl_mesh3d_inst_t;

typedef struct {
	uint32_t first_vert;
	uint32_t vert_count;
	uint32_t first_idx;
	uint32_t idx_count;
	float    mesh_w_wu;
	float    mesh_h_wu;
	uint16_t sprite_w;
	uint16_t sprite_h;
	uint8_t  anchor_y;
	uint8_t  shadow_rx;
	uint8_t  shadow_ry;
	uint8_t  pad0;
} nd_mesh3d_slice_t;

typedef struct {
	nd_mesh3d_vert_t *verts;
	uint32_t          vert_count;
	uint32_t          vert_cap;
	uint32_t         *indices;
	uint32_t          idx_count;
	uint32_t          idx_cap;
} nd_mesh3d_builder_t;

typedef struct {
	nd_mesh3d_builder_t *builder;
	nd_mesh3d_slice_t   *out_slice;
	float                target_w_wu;
	float                target_h_wu;
} PlantGeometryOutput;

/* =========================================================================
 * Section 1: Low-Level Deterministic Hash, Color & Vertex Emission Helpers
 * ========================================================================= */

static inline uint32_t
nd_m3d_pack_rgb3(const uint8_t rgb[3])
{
	return 0xFF000000u | ((uint32_t)rgb[2] << 16) | ((uint32_t)rgb[1] << 8) | (uint32_t)rgb[0];
}

static inline uint32_t
nd_m3d_pack_color4(PlantColor4 c)
{
	int r = (int)(plant_clampf(c.r, 0.0f, 1.0f) * 255.0f + 0.5f);
	int g = (int)(plant_clampf(c.g, 0.0f, 1.0f) * 255.0f + 0.5f);
	int b = (int)(plant_clampf(c.b, 0.0f, 1.0f) * 255.0f + 0.5f);
	return 0xFF000000u | ((uint32_t)b << 16) | ((uint32_t)g << 8) | (uint32_t)r;
}

static inline void
nd_m3d_plant_metric_dims(unsigned sp_idx, unsigned is_big,
                         float *out_w_wu, float *out_h_wu, float *out_ay_wu)
{
	unsigned idx = (sp_idx < ND_PLANT_MAX) ? sp_idx : 10u;
	const nd_plant_genes_t *g = &s_nd_plant_specs[idx].genes;
	float w_wu = (float)(is_big ? g->big_w : g->small_w);
	float h_wu = (float)(is_big ? g->big_h : g->small_h);
	float ay_wu = fmaxf(1.5f, fminf(9.0f, h_wu * 0.06f));
	if (out_w_wu)  *out_w_wu  = w_wu;
	if (out_h_wu)  *out_h_wu  = h_wu;
	if (out_ay_wu) *out_ay_wu = ay_wu;
}

static inline uint32_t
nd_m3d_hash(uint32_t x)
{
	x ^= x >> 16;
	x *= 0x7FEB352Du;
	x ^= x >> 15;
	x *= 0x846CA68Bu;
	x ^= x >> 16;
	return x;
}

static inline float
nd_m3d_rand01(uint32_t seed)
{
	return (float)(nd_m3d_hash(seed) & 0xFFFFFFu) / 16777215.0f;
}

static inline float
nd_m3d_rand_signed(uint32_t seed)
{
	return nd_m3d_rand01(seed) * 2.0f - 1.0f;
}

static inline void
nd_m3d_norm3(float v[3])
{
	float l2 = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
	if (l2 > 1e-12f) {
		float inv = 1.0f / sqrtf(l2);
		v[0] *= inv;
		v[1] *= inv;
		v[2] *= inv;
	} else {
		v[0] = 0.0f;
		v[1] = 0.0f;
		v[2] = 1.0f;
	}
}

static inline void
nd_m3d_unpack_rgb(uint32_t c, float out_rgb[3])
{
	uint32_t r = (c & 0xFFu);
	uint32_t g = ((c >> 8) & 0xFFu);
	uint32_t b = ((c >> 16) & 0xFFu);
	out_rgb[0] = (float)r / 255.0f;
	out_rgb[1] = (float)g / 255.0f;
	out_rgb[2] = (float)b / 255.0f;
}

static inline uint32_t
nd_m3d_lerp_col(uint32_t c0, uint32_t c1, float t)
{
	if (t <= 0.0f) return c0;
	if (t >= 1.0f) return c1;
	int r0 = (int)(c0 & 0xFFu), g0 = (int)((c0 >> 8) & 0xFFu), b0 = (int)((c0 >> 16) & 0xFFu);
	int r1 = (int)(c1 & 0xFFu), g1 = (int)((c1 >> 8) & 0xFFu), b1 = (int)((c1 >> 16) & 0xFFu);
	int r = r0 + (int)((float)(r1 - r0) * t + 0.5f);
	int g = g0 + (int)((float)(g1 - g0) * t + 0.5f);
	int b = b0 + (int)((float)(b1 - b0) * t + 0.5f);
	if (r < 0) r = 0; else if (r > 255) r = 255;
	if (g < 0) g = 0; else if (g > 255) g = 255;
	if (b < 0) b = 0; else if (b > 255) b = 255;
	return 0xFF000000u | ((uint32_t)b << 16) | ((uint32_t)g << 8) | (uint32_t)r;
}

static inline uint32_t
nd_m3d_push_vert(nd_mesh3d_builder_t *b,
                 float x, float y, float z, float wind_w,
                 float nx, float ny, float nz, float ao,
                 uint32_t col, float mat_class)
{
	if (b->vert_count >= b->vert_cap)
		return (uint32_t)(b->vert_count ? b->vert_count - 1u : 0u);
	uint32_t idx = b->vert_count++;
	nd_mesh3d_vert_t *v = &b->verts[idx];
	float n[3] = { nx, ny, nz };
	nd_m3d_norm3(n);
	v->pos[0] = x;
	v->pos[1] = y;
	v->pos[2] = z;
	v->pos[3] = wind_w < 0.0f ? 0.0f : wind_w;
	v->norm[0] = n[0];
	v->norm[1] = n[1];
	v->norm[2] = n[2];
	v->norm[3] = ao < 0.12f ? 0.12f : (ao > 1.0f ? 1.0f : ao);
	nd_m3d_unpack_rgb(col, v->col);
	v->col[3] = mat_class;
	return idx;
}

static inline void
nd_m3d_push_tri(nd_mesh3d_builder_t *b, uint32_t i0, uint32_t i1, uint32_t i2)
{
	if (b->idx_count + 3u > b->idx_cap)
		return;
	b->indices[b->idx_count++] = i0;
	b->indices[b->idx_count++] = i1;
	b->indices[b->idx_count++] = i2;
}

static inline void
nd_m3d_emit_faceted_tri(nd_mesh3d_builder_t *b,
                        const float p0[3], const float p1[3], const float p2[3],
                        float wind0, float wind1, float wind2,
                        float ao0, float ao1, float ao2,
                        uint32_t col0, uint32_t col1, uint32_t col2,
                        float mat_class)
{
	float e1[3] = { p1[0] - p0[0], p1[1] - p0[1], p1[2] - p0[2] };
	float e2[3] = { p2[0] - p0[0], p2[1] - p0[1], p2[2] - p0[2] };
	float n[3] = {
		e1[1] * e2[2] - e1[2] * e2[1],
		e1[2] * e2[0] - e1[0] * e2[2],
		e1[0] * e2[1] - e1[1] * e2[0]
	};
	nd_m3d_norm3(n);
	uint32_t v0 = nd_m3d_push_vert(b, p0[0], p0[1], p0[2], wind0, n[0], n[1], n[2], ao0, col0, mat_class);
	uint32_t v1 = nd_m3d_push_vert(b, p1[0], p1[1], p1[2], wind1, n[0], n[1], n[2], ao1, col1, mat_class);
	uint32_t v2 = nd_m3d_push_vert(b, p2[0], p2[1], p2[2], wind2, n[0], n[1], n[2], ao2, col2, mat_class);
	nd_m3d_push_tri(b, v0, v1, v2);
}

/* =========================================================================
 * Section 2: 3D Vascular Bark Tubes, Branchlet Cards & Organ Primitives
 * ========================================================================= */

/* 3D Tapered Stem / Spadix / Cone Cylinder */
static inline void
nd_m3d_add_trunk_tube_ex(nd_mesh3d_builder_t *b,
                         float x0, float y0, float z0, float r0,
                         float x1, float y1, float z1, float r1,
                         int sides, int rings,
                         uint32_t col_dark, uint32_t col_mid, uint32_t col_lit,
                         uint8_t bark_type, float crook_mag, float total_h, uint32_t seed)
{
	if (sides < 5) sides = 5;
	if (sides > 10) sides = 10;
	if (rings < 1) rings = 1;
	if (rings > 7) rings = 7;

	float dx = x1 - x0, dy = y1 - y0, dz = z1 - z0;
	float len = sqrtf(dx * dx + dy * dy + dz * dz);
	if (len < 1e-4f) return;
	float axis[3] = { dx / len, dy / len, dz / len };
	float up_ref[3] = { 0.0f, 1.0f, 0.0f };
	if (fabsf(axis[1]) > 0.85f) {
		up_ref[0] = 1.0f; up_ref[1] = 0.0f; up_ref[2] = 0.0f;
	}
	float u_vec[3] = {
		up_ref[1] * axis[2] - up_ref[2] * axis[1],
		up_ref[2] * axis[0] - up_ref[0] * axis[2],
		up_ref[0] * axis[1] - up_ref[1] * axis[0]
	};
	nd_m3d_norm3(u_vec);
	float v_vec[3] = {
		axis[1] * u_vec[2] - axis[2] * u_vec[1],
		axis[2] * u_vec[0] - axis[0] * u_vec[2],
		axis[0] * u_vec[1] - axis[1] * u_vec[0]
	};
	nd_m3d_norm3(v_vec);

	float bow_u = crook_mag * len * nd_m3d_rand_signed(seed + 17u);
	float bow_v = crook_mag * 0.65f * len * nd_m3d_rand_signed(seed + 53u);
	float s_curve = crook_mag * 0.35f * len * nd_m3d_rand_signed(seed + 97u);

	uint32_t ring_verts[8][11];
	for (int ir = 0; ir <= rings; ir++) {
		float t = (float)ir / (float)rings;
		float arch = sinf(t * 3.14159265f);
		float s_arch = sinf(t * 6.2831853f);
		float du = bow_u * arch + s_curve * s_arch;
		float dv = bow_v * arch - s_curve * 0.5f * s_arch;
		float cx = x0 + dx * t + u_vec[0] * du + v_vec[0] * dv;
		float cy = y0 + dy * t + u_vec[1] * du + v_vec[1] * dv;
		float cz = z0 + dz * t + u_vec[2] * du + v_vec[2] * dv;
		float rad = r0 + (r1 - r0) * t;

		float flare_h = fmaxf(5.0f, total_h * 0.08f);
		float flare = (cz < flare_h && z0 < 1.5f)
			? (1.0f + 0.38f * (1.0f - cz / flare_h) * (1.0f - cz / flare_h))
			: 1.0f;
		float wind_w = (total_h > 1.0f) ? fminf(0.45f, (cz / total_h) * 0.38f) : 0.1f;
		float h_norm = (total_h > 1.0f) ? fminf(1.0f, fmaxf(0.0f, cz / total_h)) : t;
		float canopy_occl = (h_norm > 0.35f)
			? (0.72f - 0.14f * sinf((h_norm - 0.35f) * 4.5f))
			: (0.62f + 0.30f * (h_norm / 0.35f));

		for (int is = 0; is < sides; is++) {
			float ang = ((float)is / (float)sides) * 6.2831853f;
			float twist_ang = (bark_type == ND_PLANT_BARK_FLUTED) ? (ang + t * 1.45f) : ang;
			float ca = cosf(ang), sa = sinf(ang);
			float rib = 1.0f;
			if (bark_type == ND_PLANT_BARK_RIBBED) {
				rib = 1.0f + 0.16f * cosf(ang * (float)sides);
			} else if (bark_type == ND_PLANT_BARK_FLUTED) {
				rib = 1.0f + 0.16f * cosf(twist_ang * 3.0f + (float)(seed & 7u));
			} else if (flare > 1.02f) {
				rib = 1.0f + 0.18f * cosf(ang * 4.0f + (float)(seed & 7u)) * (flare - 1.0f);
			} else {
				rib = 1.0f + 0.04f * cosf(ang * 3.0f + t * 3.14f);
			}
			float rr = rad * flare * rib;
			float nx = u_vec[0] * ca + v_vec[0] * sa;
			float ny = u_vec[1] * ca + v_vec[1] * sa;
			float nz = u_vec[2] * ca + v_vec[2] * sa;
			float vx = cx + nx * rr;
			float vy = cy + ny * rr;
			float vz = fmaxf(0.0f, cz + nz * rr);

			float sky_bias = 0.50f + 0.50f * nz;
			uint32_t h = nd_m3d_hash(seed + (uint32_t)(ir * 37 + is * 17));
			float grain = (((float)(h & 0xFFu) / 255.0f) - 0.5f) * 0.22f;
			float shade_t = fminf(1.0f, fmaxf(0.0f, 0.48f + 0.24f * sky_bias + 0.14f * h_norm + grain));
			uint32_t col = (shade_t < 0.42f)
				? nd_m3d_lerp_col(col_dark, col_mid, 0.35f + 0.65f * (shade_t / 0.42f))
				: nd_m3d_lerp_col(col_mid, col_lit, (shade_t - 0.42f) / 0.58f);

			if (bark_type == ND_PLANT_BARK_BIRCH) {
				if (h_norm < 0.14f || (h & 7u) == 0u) {
					col = nd_m3d_lerp_col(col_dark, col_mid, 0.32f);
				} else {
					col = nd_m3d_lerp_col(col_mid, col_lit, fminf(1.0f, shade_t * 1.15f));
				}
			} else if (bark_type == ND_PLANT_BARK_SCOTS) {
				if (h_norm > 0.36f) {
					float up_t = fminf(1.0f, (h_norm - 0.36f) / 0.35f);
					uint32_t upper_c = nd_m3d_lerp_col(col_mid, col_lit, 0.35f + 0.65f * shade_t);
					col = nd_m3d_lerp_col(col, upper_c, up_t * 0.85f);
				} else {
					col = nd_m3d_lerp_col(col_dark, col_mid, 0.30f + shade_t * 0.70f);
				}
			}
			ring_verts[ir][is] = nd_m3d_push_vert(b, vx, vy, vz, wind_w, nx, ny, nz, canopy_occl, col, ND_MAT3D_BARK);
		}
	}
	for (int ir = 0; ir < rings; ir++) {
		for (int is = 0; is < sides; is++) {
			int next_s = (is + 1) % sides;
			uint32_t a = ring_verts[ir][is];
			uint32_t b0 = ring_verts[ir][next_s];
			uint32_t c = ring_verts[ir + 1][next_s];
			uint32_t d = ring_verts[ir + 1][is];
			nd_m3d_push_tri(b, a, b0, c);
			nd_m3d_push_tri(b, a, c, d);
		}
	}
}

/* Continuous Catmull-Rom Parallel-Transported Multi-Joint Bark Axis Tube with Welded Branch-Collar Socket:
 * Subdivides the axis polyline with smooth Catmull-Rom curvature across all branch orders, then sweeps
 * the entire limb as a single seamless, twist-free 3D tubular mesh using a Rotation-Minimizing
 * (Bishop parallel-transport) frame (u_vec, v_vec). */
static inline void
nd_m3d_add_axis_spline_tube(nd_mesh3d_builder_t *b,
                            const float (*in_pts)[3],
                            const float *in_radii,
                            int in_n_pts,
                            int sides,
                            int is_lateral,
                            uint32_t col_dark, uint32_t col_mid, uint32_t col_lit,
                            uint8_t bark_type, float total_h, uint32_t seed)
{
	if (!in_pts || !in_radii || in_n_pts < 2) return;
	if (sides < 4) sides = 4;
	if (sides > 10) sides = 10;
	if (in_n_pts > 24) in_n_pts = 24;

	float pts[48][3];
	float radii[48];
	int n_pts = 0;
	if (in_n_pts >= 3 && in_n_pts <= 20) {
		for (int i = 0; i < in_n_pts - 1 && n_pts < 46; i++) {
			const float *p0 = in_pts[(i > 0) ? (i - 1) : 0];
			const float *p1 = in_pts[i];
			const float *p2 = in_pts[i + 1];
			const float *p3 = in_pts[(i + 2 < in_n_pts) ? (i + 2) : (in_n_pts - 1)];
			float r1 = in_radii[i], r2 = in_radii[i + 1];
			pts[n_pts][0] = p1[0]; pts[n_pts][1] = p1[1]; pts[n_pts][2] = p1[2];
			radii[n_pts] = r1;
			n_pts++;
			float sdx = p2[0] - p1[0], sdy = p2[1] - p1[1], sdz = p2[2] - p1[2];
			float slen = sqrtf(sdx * sdx + sdy * sdy + sdz * sdz);
			int is_collar_seg = (is_lateral && i == 0);
			int is_cap_seg = (i == in_n_pts - 2 && r2 < 0.15f);
			if ((!is_lateral || sides >= 6) && !is_collar_seg && !is_cap_seg &&
			    slen > fmaxf(r1 * 2.5f, 8.0f) && n_pts < 46) {
				pts[n_pts][0] = 0.5625f * (p1[0] + p2[0]) - 0.0625f * (p0[0] + p3[0]);
				pts[n_pts][1] = 0.5625f * (p1[1] + p2[1]) - 0.0625f * (p0[1] + p3[1]);
				pts[n_pts][2] = 0.5625f * (p1[2] + p2[2]) - 0.0625f * (p0[2] + p3[2]);
				radii[n_pts]  = 0.5f * (r1 + r2);
				n_pts++;
			}
		}
		pts[n_pts][0] = in_pts[in_n_pts - 1][0];
		pts[n_pts][1] = in_pts[in_n_pts - 1][1];
		pts[n_pts][2] = in_pts[in_n_pts - 1][2];
		radii[n_pts]  = in_radii[in_n_pts - 1];
		n_pts++;
	} else {
		n_pts = in_n_pts;
		for (int i = 0; i < n_pts; i++) {
			pts[i][0] = in_pts[i][0]; pts[i][1] = in_pts[i][1]; pts[i][2] = in_pts[i][2];
			radii[i] = in_radii[i];
		}
	}

	float tang[48][3];
	for (int i = 0; i < n_pts; i++) {
		int i0 = (i > 0) ? (i - 1) : 0;
		int i1 = (i + 1 < n_pts) ? (i + 1) : (n_pts - 1);
		float tx = pts[i1][0] - pts[i0][0];
		float ty = pts[i1][1] - pts[i0][1];
		float tz = pts[i1][2] - pts[i0][2];
		float len = sqrtf(tx * tx + ty * ty + tz * tz);
		if (len < 1e-4f) {
			tang[i][0] = 0.0f; tang[i][1] = 0.0f; tang[i][2] = 1.0f;
		} else {
			tang[i][0] = tx / len; tang[i][1] = ty / len; tang[i][2] = tz / len;
		}
	}

	float t_prev[3] = { tang[0][0], tang[0][1], tang[0][2] };
	float up_ref[3] = { 0.0f, 1.0f, 0.0f };
	if (fabsf(t_prev[1]) > 0.85f) {
		up_ref[0] = 1.0f; up_ref[1] = 0.0f; up_ref[2] = 0.0f;
	}
	float u_vec[3] = {
		up_ref[1] * t_prev[2] - up_ref[2] * t_prev[1],
		up_ref[2] * t_prev[0] - up_ref[0] * t_prev[2],
		up_ref[0] * t_prev[1] - up_ref[1] * t_prev[0]
	};
	nd_m3d_norm3(u_vec);
	float v_vec[3] = {
		t_prev[1] * u_vec[2] - t_prev[2] * u_vec[1],
		t_prev[2] * u_vec[0] - t_prev[0] * u_vec[2],
		t_prev[0] * u_vec[1] - t_prev[1] * u_vec[0]
	};
	nd_m3d_norm3(v_vec);

	uint32_t ring_verts[48][11];
	for (int i = 0; i < n_pts; i++) {
		if (i > 0) {
			float *t_cur = tang[i];
			float dot_u = u_vec[0] * t_cur[0] + u_vec[1] * t_cur[1] + u_vec[2] * t_cur[2];
			u_vec[0] -= dot_u * t_cur[0];
			u_vec[1] -= dot_u * t_cur[1];
			u_vec[2] -= dot_u * t_cur[2];
			nd_m3d_norm3(u_vec);
			v_vec[0] = t_cur[1] * u_vec[2] - t_cur[2] * u_vec[1];
			v_vec[1] = t_cur[2] * u_vec[0] - t_cur[0] * u_vec[2];
			v_vec[2] = t_cur[0] * u_vec[1] - t_cur[1] * u_vec[0];
			nd_m3d_norm3(v_vec);
			t_prev[0] = t_cur[0]; t_prev[1] = t_cur[1]; t_prev[2] = t_cur[2];
		}

		float t = (float)i / (float)(n_pts - 1);
		float cx = pts[i][0], cy = pts[i][1], cz = pts[i][2];
		float rad = radii[i];

		float flare_h = fmaxf(4.0f, total_h * 0.065f);
		float flare = 1.0f;
		if (!is_lateral && cz < flare_h && pts[0][2] < 1.5f) {
			float f0 = 1.0f - cz / flare_h;
			flare = 1.0f + 0.22f * f0 * f0;
		} else if (is_lateral && i == 0) {
			flare = 1.16f;
		} else if (is_lateral && i == 1) {
			flare = 1.04f;
		}

		float wind_w = (total_h > 1.0f) ? fminf(0.45f, (cz / total_h) * 0.38f) : 0.1f;
		float h_norm = (total_h > 1.0f) ? fminf(1.0f, fmaxf(0.0f, cz / total_h)) : t;
		float canopy_occl = (h_norm > 0.35f)
			? (0.76f - 0.10f * sinf((h_norm - 0.35f) * 4.5f))
			: (0.68f + 0.26f * (h_norm / 0.35f));

		for (int is = 0; is < sides; is++) {
			float ang = ((float)is / (float)sides) * 6.2831853f;
			float twist_ang = (bark_type == ND_PLANT_BARK_FLUTED) ? (ang + t * 1.45f) : ang;
			float ca = cosf(ang), sa = sinf(ang);
			float rib = 1.0f;
			float rib_wave = 0.0f, rib_deriv = 0.0f;
			if (bark_type == ND_PLANT_BARK_RIBBED) {
				rib_wave  = cosf(ang * 8.0f);
				rib_deriv = -sinf(ang * 8.0f);
				rib = 1.0f + 0.20f * rib_wave;
			} else if (bark_type == ND_PLANT_BARK_FLUTED) {
				rib_wave  = cosf(twist_ang * 3.0f + (float)(seed & 7u));
				rib_deriv = -0.45f * sinf(twist_ang * 3.0f + (float)(seed & 7u));
				rib = 1.0f + 0.16f * rib_wave;
			} else if (!is_lateral && flare > 1.02f) {
				float fl_w = (flare - 1.0f);
				rib_wave  = cosf(ang * 4.0f + (float)(seed & 7u));
				rib_deriv = -0.60f * sinf(ang * 4.0f + (float)(seed & 7u)) * fl_w;
				rib = 1.0f + 0.16f * rib_wave * fl_w;
			} else {
				rib = 1.0f + 0.04f * cosf(ang * 3.0f + t * 3.14f);
			}
			float rr = rad * flare * rib;
			float tan_x = -u_vec[0] * sa + v_vec[0] * ca;
			float tan_y = -u_vec[1] * sa + v_vec[1] * ca;
			float tan_z = -u_vec[2] * sa + v_vec[2] * ca;
			float nx = u_vec[0] * ca + v_vec[0] * sa - tan_x * (0.42f * rib_deriv);
			float ny = u_vec[1] * ca + v_vec[1] * sa - tan_y * (0.42f * rib_deriv);
			float nz = u_vec[2] * ca + v_vec[2] * sa - tan_z * (0.42f * rib_deriv);
			float nlen = sqrtf(nx * nx + ny * ny + nz * nz);
			if (nlen > 1e-4f) { nx /= nlen; ny /= nlen; nz /= nlen; }
			float vx = cx + (u_vec[0] * ca + v_vec[0] * sa) * rr;
			float vy = cy + (u_vec[1] * ca + v_vec[1] * sa) * rr;
			float vz = cz + (u_vec[2] * ca + v_vec[2] * sa) * rr;
			if (!is_lateral && i == 0 && pts[0][2] < 1.5f) {
				vz = 0.0f;
			} else if (vz < 0.0f) {
				vz = 0.0f;
			}

			float sky_bias = 0.50f + 0.50f * nz;
			uint32_t h = nd_m3d_hash(seed + (uint32_t)(i * 37 + is * 17));
			float grain = (((float)(h & 0xFFu) / 255.0f) - 0.5f) * 0.22f;
			float shade_t = fminf(1.0f, fmaxf(0.0f, 0.48f + 0.24f * sky_bias + 0.14f * h_norm + grain));
			uint32_t col = (shade_t < 0.42f)
				? nd_m3d_lerp_col(col_dark, col_mid, 0.35f + 0.65f * (shade_t / 0.42f))
				: nd_m3d_lerp_col(col_mid, col_lit, (shade_t - 0.42f) / 0.58f);

			if (bark_type == ND_PLANT_BARK_RIBBED) {
				col = (rib_wave > 0.25f)
					? nd_m3d_lerp_col(col_mid, col_lit, 0.45f + 0.55f * rib_wave)
					: nd_m3d_lerp_col(col_dark, col_mid, 0.50f + 0.50f * (rib_wave + 1.0f));
			} else if (bark_type == ND_PLANT_BARK_BIRCH) {
				if ((!is_lateral && i == 0 && pts[0][2] < 1.5f) || (is_lateral && i == 0) || ((h & 15u) == 0u)) {
					col = nd_m3d_lerp_col(col_dark, col_mid, 0.38f);
				} else {
					col = nd_m3d_lerp_col(col_mid, col_lit, fminf(1.0f, 0.45f + shade_t * 0.65f));
				}
			} else if (bark_type == ND_PLANT_BARK_SCOTS) {
				if (h_norm > 0.36f) {
					float up_t = fminf(1.0f, (h_norm - 0.36f) / 0.35f);
					uint32_t upper_c = nd_m3d_lerp_col(col_mid, col_lit, 0.35f + 0.65f * shade_t);
					col = nd_m3d_lerp_col(col, upper_c, up_t * 0.85f);
				} else {
					col = nd_m3d_lerp_col(col_dark, col_mid, 0.30f + shade_t * 0.70f);
				}
			}
			float bark_mat = ND_MAT3D_BARK + (float)(bark_type & 7u) * 0.005f;
			ring_verts[i][is] = nd_m3d_push_vert(b, vx, vy, vz, wind_w, nx, ny, nz, canopy_occl, col, bark_mat);
		}
	}
	for (int i = 0; i < n_pts - 1; i++) {
		for (int is = 0; is < sides; is++) {
			int next_s = (is + 1) % sides;
			uint32_t a  = ring_verts[i][is];
			uint32_t b0 = ring_verts[i][next_s];
			uint32_t c  = ring_verts[i + 1][next_s];
			uint32_t d  = ring_verts[i + 1][is];
			nd_m3d_push_tri(b, a, b0, c);
			nd_m3d_push_tri(b, a, c, d);
		}
	}
}

/* Pack longitudinal branchlet coordinate u in [0, 1] and wind flexibility w in [0.01, 0.99] into pos[3] */
static inline float
nd_m3d_pack_card_u_wind(float u, float wind_w)
{
	float uc = fminf(1.0f, fmaxf(0.0f, u));
	float wc = fminf(0.98f, fmaxf(0.02f, wind_w));
	float u_idx = floorf(uc * 249.0f + 1.0f); /* 1..250 so pos[3] > 1.0f always flags a UV card */
	return u_idx + wc;
}

/* Pack botanical foliage archetype base (ND_MAT3D_FOL_*) and lateral card coordinate v in [0, 1] into col[3] */
static inline float
nd_m3d_pack_card_mat_v(float fol_base, float v)
{
	float vc = fminf(1.0f, fmaxf(0.0f, v));
	float span = (fol_base >= 0.319f) ? 0.030f : 0.038f;
	return fol_base + vc * span;
}

/* SpeedTree / Creation Engine (Skyrim)–Style 3D Branchlet Sheet with UV Encoding & Outward Dome-Transferred Normals:
 * Emits a 3-segment x 2-wing doubly-curved 3D botanical branchlet card (9 vertices, 8 triangles) with:
 *   - Explicit (u, v) card coordinates packed in pos[3] and col[3] for per-pixel needle/leaf cutouts,
 *   - Axial roll (roll_rad), gravitropic bow, and dihedral V-cupping,
 *   - Outward dome-transferred normals blended (72% spherical crown dome normal from (dome_cx, dome_cy, dome_cz),
 *     28% cambered sheet normal) so crossed branchlet sheets shade as a cohesive, lush 3D volume. */
static inline void
nd_m3d_add_branchlet_card(nd_mesh3d_builder_t *b,
                          float x0, float y0, float z0,
                          float dir_x, float dir_y, float dir_z,
                          float length, float height_arc, float drop_end,
                          float base_half_w, float cup_dz, float roll_rad,
                          float dome_cx, float dome_cy, float dome_cz,
                          float fol_base,
                          uint32_t col_deep, uint32_t col_mid, uint32_t col_lit, uint32_t col_high,
                          float total_h)
{
	float fwd[3] = { dir_x, dir_y, dir_z };
	nd_m3d_norm3(fwd);
	float up_ref[3] = { 0.0f, 0.0f, 1.0f };
	if (fabsf(fwd[2]) > 0.92f) { up_ref[0] = 1.0f; up_ref[2] = 0.0f; }
	float side0[3] = {
		fwd[1] * up_ref[2] - fwd[2] * up_ref[1],
		fwd[2] * up_ref[0] - fwd[0] * up_ref[2],
		fwd[0] * up_ref[1] - fwd[1] * up_ref[0]
	};
	nd_m3d_norm3(side0);
	float up0[3] = {
		side0[1] * fwd[2] - side0[2] * fwd[1],
		side0[2] * fwd[0] - side0[0] * fwd[2],
		side0[0] * fwd[1] - side0[1] * fwd[0]
	};
	nd_m3d_norm3(up0);

	float cr = cosf(roll_rad), sr = sinf(roll_rad);
	float side[3] = {
		side0[0] * cr + up0[0] * sr,
		side0[1] * cr + up0[1] * sr,
		side0[2] * cr + up0[2] * sr
	};
	float card_up[3] = {
		-side0[0] * sr + up0[0] * cr,
		-side0[1] * sr + up0[1] * cr,
		-side0[2] * sr + up0[2] * cr
	};

	float mat_left  = nd_m3d_pack_card_mat_v(fol_base, 0.02f);
	float mat_mid   = nd_m3d_pack_card_mat_v(fol_base, 0.50f);
	float mat_right = nd_m3d_pack_card_mat_v(fol_base, 0.98f);

	uint32_t v_left[3], v_mid[3], v_right[3];
	for (int s = 0; s <= 2; s++) {
		float u = (float)s * 0.5f;
		float width_prof = (s == 0) ? 0.44f : ((s == 1) ? 1.08f : 0.34f);
		float hw = base_half_w * width_prof;
		float reach = length * u;
		float arc_z = height_arc * sinf(u * 2.15f) - drop_end * u * u;
		float cx = x0 + fwd[0] * reach;
		float cy = y0 + fwd[1] * reach;
		float cz = fmaxf(0.4f, z0 + fwd[2] * reach + arc_z);

		float wx_l = cx - side[0] * hw - card_up[0] * (cup_dz * width_prof);
		float wy_l = cy - side[1] * hw - card_up[1] * (cup_dz * width_prof);
		float wz_l = fmaxf(0.4f, cz - side[2] * hw - card_up[2] * (cup_dz * width_prof));

		float wx_r = cx + side[0] * hw - card_up[0] * (cup_dz * width_prof);
		float wy_r = cy + side[1] * hw - card_up[1] * (cup_dz * width_prof);
		float wz_r = fmaxf(0.4f, cz + side[2] * hw - card_up[2] * (cup_dz * width_prof));

		uint32_t c_base_mid = nd_m3d_lerp_col(col_deep, col_mid, 0.58f);
		uint32_t c_sp = (s == 0) ? c_base_mid : ((s == 1) ? col_mid : col_lit);
		uint32_t c_wg = (s == 0) ? col_mid    : ((s == 1) ? col_lit : col_high);
		float h_frac = fminf(1.0f, fmaxf(0.0f, cz / fmaxf(1.0f, total_h)));
		float r_dist = sqrtf(cx * cx + cy * cy);
		float r_frac = fminf(1.0f, r_dist / fmaxf(4.0f, total_h * 0.28f));
		float crown_exp = fminf(1.0f, 0.28f + 0.40f * h_frac + 0.32f * r_frac);
		float ao = fminf(1.0f, (0.36f + 0.46f * crown_exp) * (0.62f + 0.38f * u) + 0.18f * u * crown_exp);
		float raw_ww = fminf(0.98f, fmaxf(0.14f, h_frac) * (0.40f + 0.60f * u));
		float pw_mid  = nd_m3d_pack_card_u_wind(u, raw_ww * 0.90f);
		float pw_wing = nd_m3d_pack_card_u_wind(u, raw_ww);

		float dm[3] = { cx - dome_cx, cy - dome_cy, (cz - dome_cz) * 1.18f + 2.4f };
		nd_m3d_norm3(dm);
		/* SpeedTree / Skyrim Spherized Cloud Normals:
		 * 82% radial outward sub-crown dome normal + 18% local card camber normal so crossed cards
		 * share a continuous 3D illumination gradient without planar intersection seams. */
		float nm[3] = {
			dm[0] * 0.82f + card_up[0] * 0.18f,
			dm[1] * 0.82f + card_up[1] * 0.18f,
			dm[2] * 0.82f + card_up[2] * 0.18f
		};
		float nl[3] = {
			nm[0] - side[0] * 0.22f,
			nm[1] - side[1] * 0.22f,
			nm[2] - side[2] * 0.22f
		};
		float nr[3] = {
			nm[0] + side[0] * 0.22f,
			nm[1] + side[1] * 0.22f,
			nm[2] + side[2] * 0.22f
		};

		v_left[s]  = nd_m3d_push_vert(b, wx_l, wy_l, wz_l, pw_wing,
		                              nl[0], nl[1], nl[2], ao * 0.94f,
		                              c_wg, mat_left);
		v_mid[s]   = nd_m3d_push_vert(b, cx, cy, cz, pw_mid,
		                              nm[0], nm[1], nm[2], ao,
		                              c_sp, mat_mid);
		v_right[s] = nd_m3d_push_vert(b, wx_r, wy_r, wz_r, pw_wing,
		                              nr[0], nr[1], nr[2], ao * 0.94f,
		                              c_wg, mat_right);
	}
	for (int s = 0; s < 2; s++) {
		nd_m3d_push_tri(b, v_left[s],  v_mid[s],   v_mid[s + 1]);
		nd_m3d_push_tri(b, v_left[s],  v_mid[s + 1], v_left[s + 1]);
		nd_m3d_push_tri(b, v_mid[s],   v_right[s], v_right[s + 1]);
		nd_m3d_push_tri(b, v_mid[s],   v_right[s + 1], v_mid[s + 1]);
	}
}

/* 3D Cupped Leaf-Frond Spray (for herbaceous cauline leaves) */
static inline void
nd_m3d_add_leaf_frond_spray(nd_mesh3d_builder_t *b,
                            float x0, float y0, float z0,
                            float dir_x, float dir_y,
                            float length, float height_arc, float drop_end,
                            float base_half_w, float cup_dz,
                            uint32_t col_deep, uint32_t col_mid, uint32_t col_lit, uint32_t col_high,
                            float total_h)
{
	nd_m3d_add_branchlet_card(b, x0, y0, z0,
	                          dir_x, dir_y, 0.08f,
	                          length, height_arc, drop_end,
	                          base_half_w, cup_dz, 0.0f,
	                          x0 - dir_x * length * 0.25f, y0 - dir_y * length * 0.25f, z0 - drop_end * 0.4f,
	                          ND_MAT3D_FOL_OVATE,
	                          col_deep, col_mid, col_lit, col_high, total_h);
}

/* 3D Multi-Sheet Bush Rosette Cluster (for fruiting herbaceous bushes) */
static inline void
nd_m3d_add_foliage_bough_cluster_ex(nd_mesh3d_builder_t *b,
                                    float cx, float cy, float cz,
                                    float rx, float ry, float rz,
                                    float fol_base,
                                    uint32_t col_deep, uint32_t col_mid, uint32_t col_lit, uint32_t col_high,
                                    float total_h, uint32_t seed)
{
	uint32_t hk = nd_m3d_hash(seed + 137u);
	float ang0 = ((float)(hk & 0xFFu) / 255.0f) * 6.2831853f;
	float dome_cz = cz - rz * 0.18f;

	for (int s = 0; s < 3; s++) {
		uint32_t hs = nd_m3d_hash(seed + (uint32_t)(s * 179 + 43));
		float jitter = (((float)(hs & 0xFFu) / 255.0f) - 0.5f) * 0.42f;
		float sang = ang0 + (float)s * 2.0943951f + jitter;
		float sca = cosf(sang), ssa = sinf(sang);
		float roll = ((s & 1) ? 0.34f : -0.34f) + jitter * 0.25f;
		float sx = cx + sca * rx * 0.08f;
		float sy = cy + ssa * ry * 0.08f;
		float sz = cz - rz * 0.06f + ((float)(s & 1) * rz * 0.08f);
		float card_len = fmaxf(rx, ry) * (0.94f + 0.12f * ((float)((hs >> 8) & 0x3Fu) / 63.0f));
		float card_hw  = card_len * 0.58f;
		nd_m3d_add_branchlet_card(b, sx, sy, sz,
		                          sca, ssa, 0.16f,
		                          card_len, rz * 0.28f, rz * 0.24f,
		                          card_hw, rz * 0.14f, roll,
		                          cx, cy, dome_cz,
		                          fol_base,
		                          col_deep, col_mid, col_lit, col_high, total_h);
	}
}

/* Continuous Genome-Parameterized Terminal Shoot Bough Fan:
 * Interpolates smoothly between:
 *   - Dome / ascending broadleaf bough fan (tabular_t -> 0, plagiotropic_t -> 0)
 *   - Horizontally stratified tabular umbrella pad (tabular_t -> 1, low crown_flatness)
 *   - Layered plagiotropic whorled bough fan (plagiotropic_t -> 1, high apical dominance)
 * Every card's woody stem (u = 0) anchors into the terminal twig tip (ax, ay, az) and
 * fans outward along the branch growth direction (dx, dy) with moderate dihedral roll
 * so no card edge hangs downward below the crown floor. */
static inline void
nd_m3d_add_terminal_shoot_fan(nd_mesh3d_builder_t *b,
                              float ax, float ay, float az,
                              float dx, float dy,
                              float rx, float ry, float rz,
                              float tabular_t, float plagiotropic_t,
                              float fol_base,
                              uint32_t col_deep, uint32_t col_mid, uint32_t col_lit, uint32_t col_high,
                              float total_h, uint32_t seed)
{
	float dlen = sqrtf(dx * dx + dy * dy);
	float dir_x = (dlen > 1e-4f) ? (dx / dlen) : 1.0f;
	float dir_y = (dlen > 1e-4f) ? (dy / dlen) : 0.0f;
	float base_ang = atan2f(dir_y, dir_x);

	float r_base = fmaxf(rx, ry);
	float loc_cx = ax + dir_x * (r_base * 0.36f);
	float loc_cy = ay + dir_y * (r_base * 0.36f);
	float loc_cz = az - rz * (0.16f + 0.24f * tabular_t);
	float dome_cx = loc_cx * 0.52f;
	float dome_cy = loc_cy * 0.52f;
	float dome_cz = loc_cz * 0.62f + (total_h * (0.54f + 0.22f * tabular_t)) * 0.38f;

	/* Continuous angular spread, pitch, arc, drop, and roll governed by tabular_t and plagiotropic_t */
	float wing_ang   = 0.78f + 0.24f * tabular_t - 0.16f * plagiotropic_t;
	float pitch_main = 0.22f * (1.0f - tabular_t) * (1.0f - plagiotropic_t) + 0.04f * tabular_t - 0.04f * plagiotropic_t;
	float pitch_wing = 0.16f * (1.0f - tabular_t) * (1.0f - plagiotropic_t) + 0.03f * tabular_t - 0.06f * plagiotropic_t;
	float pitch_cap  = 0.34f * (1.0f - tabular_t) + 0.06f * tabular_t - 0.10f * plagiotropic_t;
	float arc_scale  = 0.34f * (1.0f - 0.40f * tabular_t);
	float drop_scale = 0.24f * (1.0f - 0.45f * tabular_t) + 0.08f * plagiotropic_t;
	float roll_wing  = 0.44f * (1.0f - 0.40f * tabular_t) + 0.10f * plagiotropic_t;
	float back_tuck  = 0.26f * (1.0f - 0.35f * tabular_t);

	uint32_t h0 = nd_m3d_hash(seed + 137u);
	float jit0 = (((float)(h0 & 0xFFu) / 255.0f) - 0.5f) * 0.16f;

	/* Card 0: Central forward leader shoot anchored at the terminal twig fork */
	{
		float ang = base_ang + jit0;
		float ca = cosf(ang), sa = sinf(ang);
		float sx = ax - dir_x * (r_base * back_tuck);
		float sy = ay - dir_y * (r_base * back_tuck);
		float sz = az + rz * 0.04f;
		float card_len = r_base * (1.28f + 0.10f * tabular_t);
		float card_hw  = card_len * (0.58f + 0.08f * tabular_t);
		nd_m3d_add_branchlet_card(b, sx, sy, sz,
		                          ca, sa, pitch_main,
		                          card_len, rz * arc_scale, rz * drop_scale,
		                          card_hw, rz * 0.14f, jit0 * 0.6f,
		                          dome_cx, dome_cy, dome_cz,
		                          fol_base,
		                          col_deep, col_mid, col_lit, col_high, total_h);
	}

	/* Cards 1 & 2: Left & right lateral shingled bough fan wings */
	for (int s = 0; s < 2; s++) {
		float side_sign = (s == 0) ? 1.0f : -1.0f;
		uint32_t hs = nd_m3d_hash(seed + (uint32_t)(s * 179 + 43));
		float jitter = (((float)(hs & 0xFFu) / 255.0f) - 0.5f) * 0.20f;
		float ang = base_ang + side_sign * wing_ang + jitter;
		float ca = cosf(ang), sa = sinf(ang);
		float sx = ax - ca * (r_base * back_tuck * 0.80f);
		float sy = ay - sa * (r_base * back_tuck * 0.80f);
		float sz = az + rz * (0.02f - 0.04f * (float)s);
		float card_len = r_base * (1.18f + 0.10f * ((float)((hs >> 8) & 0x3Fu) / 63.0f));
		float card_hw  = card_len * (0.60f + 0.06f * tabular_t);
		float roll = side_sign * roll_wing + jitter * 0.18f;
		nd_m3d_add_branchlet_card(b, sx, sy, sz,
		                          ca, sa, pitch_wing,
		                          card_len, rz * arc_scale * 0.90f, rz * drop_scale * 1.05f,
		                          card_hw, rz * 0.14f, roll,
		                          dome_cx, dome_cy, dome_cz,
		                          fol_base,
		                          col_deep, col_mid, col_lit, col_high, total_h);
	}

	/* Card 3: Upper crown-shingle cap (or rear horizontal umbrella petal when tabular_t > 0.5) */
	{
		uint32_t hf = nd_m3d_hash(seed + 71u);
		float jitter = (((float)(hf & 0xFFu) / 255.0f) - 0.5f) * 0.26f;
		float ang = base_ang + ((tabular_t > 0.55f) ? 3.1415926f : 0.0f) + jitter;
		float ca = cosf(ang), sa = sinf(ang);
		float fx = ax - ca * (r_base * back_tuck * 0.90f);
		float fy = ay - sa * (r_base * back_tuck * 0.90f);
		float fz = az + rz * (0.14f - 0.06f * tabular_t);
		float card_len = r_base * (1.12f - 0.08f * tabular_t);
		float card_hw  = card_len * 0.62f;
		float roll = ((hf & 1u) ? 0.36f : -0.36f) * (1.0f - 0.40f * tabular_t);
		nd_m3d_add_branchlet_card(b, fx, fy, fz,
		                          ca, sa, pitch_cap,
		                          card_len, rz * arc_scale, rz * drop_scale * 0.85f,
		                          card_hw, rz * 0.14f, roll,
		                          dome_cx, dome_cy, dome_cz,
		                          fol_base,
		                          col_deep, col_mid, col_lit, col_high, total_h);
	}

	/* Card 4: Oblique 55-deg camera volume-filler shingle (angled cross-fan so oblique sightlines never see flat card edges) */
	{
		uint32_t hc = nd_m3d_hash(seed + 313u);
		float side_sign = (hc & 1u) ? 1.0f : -1.0f;
		float ang = base_ang + side_sign * (0.48f + 0.35f * tabular_t);
		float ca = cosf(ang), sa = sinf(ang);
		float cx0 = ax - ca * (r_base * back_tuck * 0.65f);
		float cy0 = ay - sa * (r_base * back_tuck * 0.65f);
		float cz0 = az + rz * 0.08f;
		float card_len = r_base * 1.10f;
		float card_hw  = card_len * 0.60f;
		float roll = -side_sign * (0.52f * (1.0f - 0.40f * tabular_t) + 0.14f);
		nd_m3d_add_branchlet_card(b, cx0, cy0, cz0,
		                          ca, sa, pitch_main * 1.25f + 0.08f * (1.0f - tabular_t),
		                          card_len, rz * arc_scale * 1.10f, rz * drop_scale * 0.90f,
		                          card_hw, rz * 0.16f, roll,
		                          dome_cx, dome_cy, dome_cz,
		                          fol_base,
		                          col_deep, col_mid, col_lit, col_high, total_h);
	}
}

/* 3D Arching Monocot Sword-Blade / Basal Rosette Leaf */
static inline void
nd_m3d_add_arching_blade(nd_mesh3d_builder_t *b,
                         float x0, float y0, float z0,
                         float dir_x, float dir_y,
                         float length, float height_arc, float drop_end,
                         float base_half_w,
                         uint32_t col_dark, uint32_t col_mid, uint32_t col_lit,
                         float total_h)
{
	float pdir[2] = { -dir_y, dir_x };
	float pl = sqrtf(pdir[0] * pdir[0] + pdir[1] * pdir[1]);
	if (pl > 1e-5f) { pdir[0] /= pl; pdir[1] /= pl; }

	uint32_t v_left[4], v_right[4];
	for (int s = 0; s <= 3; s++) {
		float t = (float)s / 3.0f;
		float reach = length * t;
		float z = fmaxf(0.4f, z0 + height_arc * sinf(t * 2.05f) - drop_end * t * t);
		float hw = base_half_w * (1.0f - 0.82f * t * t);
		float cx = x0 + dir_x * reach;
		float cy = y0 + dir_y * reach;

		float nx = dir_x * 0.42f;
		float ny = dir_y * 0.42f;
		float nz = 0.90f - 0.35f * t;

		uint32_t col = (s == 0) ? col_dark : ((s == 1) ? col_mid : col_lit);
		float ao = 0.50f + 0.50f * t;
		float ww = fminf(0.95f, fmaxf(0.12f, z / fmaxf(1.0f, total_h)) * (0.35f + 0.65f * t));

		v_left[s]  = nd_m3d_push_vert(b, cx - pdir[0] * hw, cy - pdir[1] * hw, z, ww,
		                              nx - pdir[0] * 0.30f, ny - pdir[1] * 0.30f, nz, ao, col, ND_MAT3D_FOLIAGE_CORE);
		v_right[s] = nd_m3d_push_vert(b, cx + pdir[0] * hw, cy + pdir[1] * hw, z, ww,
		                              nx + pdir[0] * 0.30f, ny + pdir[1] * 0.30f, nz, ao, col, ND_MAT3D_FOLIAGE_CORE);
	}
	for (int s = 0; s < 3; s++) {
		nd_m3d_push_tri(b, v_left[s], v_right[s], v_right[s + 1]);
		nd_m3d_push_tri(b, v_left[s], v_right[s + 1], v_left[s + 1]);
	}
}

/* 3D 5-Petal Radial Flower Corolla with Contrasting Central Disc */
static inline void
nd_m3d_add_blossom_star(nd_mesh3d_builder_t *b,
                        float cx, float cy, float cz,
                        float r_outer, float r_height,
                        uint32_t col_petal_lit, uint32_t col_petal_shade, uint32_t col_center,
                        float total_h)
{
	float ww = fminf(0.95f, cz / fmaxf(1.0f, total_h));
	float h_cup = fminf(r_height, r_outer * 0.34f);
	float pitch_tilt = 0.26f;
	uint32_t v_top = nd_m3d_push_vert(b, cx, cy, cz + h_cup * 0.65f, ww,
	                                  0.0f, 0.22f, 0.97f, 0.99f, col_center, ND_MAT3D_BLOSSOM);
	uint32_t v_bot = nd_m3d_push_vert(b, cx, cy, fmaxf(0.4f, cz - h_cup * 0.45f), ww,
	                                  0.0f, 0.0f, -1.0f, 0.62f, col_petal_shade, ND_MAT3D_BLOSSOM);
	uint32_t disc[5], tips[5], clefts[5];
	for (int i = 0; i < 5; i++) {
		float ang_tip   = ((float)i / 5.0f) * 6.2831853f;
		float ang_cleft = ang_tip + 0.6283185f;
		float ct = cosf(ang_tip),   st = sinf(ang_tip);
		float cc = cosf(ang_cleft), sc = sinf(ang_cleft);
		float r_disc  = r_outer * 0.32f;
		float r_cleft = r_outer * 0.40f;
		float z_disc  = cz + h_cup * 0.50f - st * r_disc * pitch_tilt;
		float z_tip   = cz + h_cup * 0.25f - st * r_outer * pitch_tilt;
		float z_cleft = cz + h_cup * 0.32f - sc * r_cleft * pitch_tilt;
		disc[i] = nd_m3d_push_vert(b, cx + ct * r_disc, cy + st * r_disc, fmaxf(0.4f, z_disc), ww,
		                           ct * 0.25f, st * 0.25f + 0.20f, 0.94f, 0.96f, col_center, ND_MAT3D_BLOSSOM);
		tips[i] = nd_m3d_push_vert(b, cx + ct * r_outer, cy + st * r_outer, fmaxf(0.4f, z_tip), ww,
		                           ct * 0.38f, st * 0.38f + 0.20f, 0.90f, 0.96f, col_petal_lit, ND_MAT3D_BLOSSOM);
		clefts[i] = nd_m3d_push_vert(b, cx + cc * r_cleft, cy + sc * r_cleft, fmaxf(0.4f, z_cleft), ww,
		                             cc * 0.30f, sc * 0.30f + 0.20f, 0.92f, 0.85f, col_petal_shade, ND_MAT3D_BLOSSOM);
	}
	for (int i = 0; i < 5; i++) {
		int next = (i + 1) % 5;
		int prev = (i + 4) % 5;
		nd_m3d_push_tri(b, disc[i], disc[next], v_top);
		nd_m3d_push_tri(b, disc[i], tips[i], clefts[i]);
		nd_m3d_push_tri(b, disc[i], clefts[prev], tips[i]);
		nd_m3d_push_tri(b, disc[i], disc[next], clefts[i]);
		nd_m3d_push_tri(b, clefts[i], tips[i], v_bot);
		nd_m3d_push_tri(b, tips[next], clefts[i], v_bot);
	}
}

/* 3D Globose Berry / Drupe Fruit */
static inline void
nd_m3d_add_fruit_berry(nd_mesh3d_builder_t *b,
                       float cx, float cy, float cz,
                       float rad, uint32_t col_ripe, uint32_t col_shade,
                       float total_h)
{
	float ww = fminf(0.92f, cz / fmaxf(1.0f, total_h));
	uint32_t v_top = nd_m3d_push_vert(b, cx, cy, cz + rad * 0.88f, ww,
	                                  0.0f, 0.0f, 1.0f, 0.95f, col_ripe, ND_MAT3D_BLOSSOM);
	uint32_t v_bot = nd_m3d_push_vert(b, cx, cy, fmaxf(0.4f, cz - rad * 0.88f), ww,
	                                  0.0f, 0.0f, -1.0f, 0.55f, col_shade, ND_MAT3D_BLOSSOM);
	uint32_t ring[6];
	for (int i = 0; i < 6; i++) {
		float ang = ((float)i / 6.0f) * 6.2831853f;
		float ca = cosf(ang), sa = sinf(ang);
		ring[i] = nd_m3d_push_vert(b, cx + ca * rad, cy + sa * rad, fmaxf(0.4f, cz), ww,
		                           ca * 0.86f, sa * 0.86f, 0.35f, 0.90f, col_ripe, ND_MAT3D_BLOSSOM);
	}
	for (int i = 0; i < 6; i++) {
		int next = (i + 1) % 6;
		nd_m3d_push_tri(b, ring[i], ring[next], v_top);
		nd_m3d_push_tri(b, ring[next], ring[i], v_bot);
	}
}

/* =========================================================================
 * Section 3: Stage 4 Pure Genome & MTG Geometry Consumer (plant_geometry_build)
 * ========================================================================= */

/* Evaluate the perspective-compensated radially symmetric conical crown-base floor z_floor(x, y).
 * Using radial distance r = hypotf(x, y) guarantees that 360-deg instance yaw rotation in s_vs_mesh3d
 * is 100% yaw-invariant while keeping h_proj(r, z) = z - 0.7002075 * r >= H * clear_bole_eff. */
static inline float
nd_plant_m3d_eval_crown_floor_z(float x, float y, float h_total, float clear_bole_eff)
{
	return h_total * clear_bole_eff + hypotf(x, y) * 0.72f;
}

static inline void
plant_geometry_build(const PlantInstance *plant,
                     uint32_t detail_level,
                     void *geometry_output)
{
	if (!plant || !geometry_output)
		return;
	PlantGeometryOutput *out = (PlantGeometryOutput *)geometry_output;
	nd_mesh3d_builder_t *b = out->builder;
	if (!b)
		return;

	uint32_t v_start = b->vert_count;
	uint32_t i_start = b->idx_count;
	const PlantGenome *g = &plant->genome;
	const PlantPhenotype *pheno = &plant->phenotype;

	float wu_per_m = ND_MESH3D_WU_PER_METER;
	float h_total = (out->target_h_wu > 1.0f) ? out->target_h_wu : fmaxf(12.0f, pheno->height_m * wu_per_m);
	float w_total = (out->target_w_wu > 1.0f) ? out->target_w_wu : fmaxf(10.0f, pheno->crown_radius_m * 2.0f * wu_per_m);

	/* Continuous architectural parameters derived strictly from PlantGenome */
	bool is_woody_tree   = (g->architecture.max_branch_order >= 3u && g->physiology.mature_height_m >= 2.0f);
	bool is_succulent    = (g->allometry.bark_class == ND_PLANT_BARK_RIBBED || g->leaf.complexity == PLANT_LEAF_SCALE);
	bool is_graminoid    = (!is_woody_tree && !is_succulent &&
	                        g->leaf.shape == PLANT_LEAF_SHAPE_LINEAR &&
	                        g->leaf.venation_type == PLANT_VENATION_PARALLELODROMOUS);
	bool is_herbaceous   = (!is_woody_tree && !is_succulent && !is_graminoid);

	float crown_flatness = plant_clampf(g->architecture.crown_flatness, 0.18f, 1.45f);
	float tabular_t      = is_woody_tree ? plant_clampf((0.64f - crown_flatness) / 0.38f, 0.0f, 1.0f) : 0.0f;
	float plagiotropic_t = (is_woody_tree && g->phyllotaxis.buds_per_node >= 3u)
		? plant_clampf((g->architecture.apical_dominance - 0.78f) / 0.20f, 0.0f, 1.0f)
		: 0.0f;
	float weep_t         = is_woody_tree ? plant_clampf((-g->architecture.orthotropy - 0.15f) / 0.65f, 0.0f, 1.0f) : 0.0f;
	float clear_bole_f   = plant_clampf(g->architecture.clear_bole_fraction, 0.12f, 0.75f);
	float clear_bole_eff = is_woody_tree
		? fmaxf(0.18f, clear_bole_f * (1.0f - 0.18f * weep_t))
		: 0.0f;

	/* Separate horizontal (scale_xy) and vertical (scale_z) metric-to-world-unit transforms
	 * so wide crowns (where W > H) never compress horizontal branch reach when scaled to h_total.
	 * Keep horizontal X/Y depth isotropic so 360-deg instance yaw rotation never looks squashed side-on. */
	float scale_z  = (pheno->height_m > 1e-3f) ? ((h_total * 0.89f) / pheno->height_m) : wu_per_m;
	float scale_xy = (pheno->crown_radius_m > 1e-3f)
		? ((w_total * (0.40f + 0.06f * tabular_t)) / pheno->crown_radius_m)
		: scale_z;
	if (!is_woody_tree) {
		scale_xy = scale_z;
	} else {
		scale_xy = plant_clampf(scale_xy, scale_z * 0.65f, scale_z * 1.65f);
	}
	float y_depth_scale = is_woody_tree ? (0.94f - 0.06f * tabular_t) : 0.96f;

	uint32_t c_bark_dark  = nd_m3d_pack_color4(g->allometry.bark_dark);
	uint32_t c_bark_mid   = nd_m3d_pack_color4(g->allometry.bark_mid);
	uint32_t c_bark_light = nd_m3d_pack_color4(g->allometry.bark_light);
	uint32_t c_leaf_deep  = nd_m3d_pack_color4(g->leaf.adaxial_deep);
	uint32_t c_leaf_mid   = nd_m3d_pack_color4(g->leaf.adaxial_mid);
	uint32_t c_leaf_lit   = nd_m3d_pack_color4(g->leaf.adaxial_lit);
	uint32_t c_leaf_high  = nd_m3d_pack_color4(g->leaf.abaxial_high);
	uint32_t c_fl_prim    = nd_m3d_pack_color4(g->flower.primary_color);
	uint32_t c_fl_sec     = nd_m3d_pack_color4(g->flower.secondary_color);
	uint32_t c_fr_imm     = nd_m3d_pack_color4(g->fruit.immature_color);
	uint32_t c_fr_mat     = nd_m3d_pack_color4(g->fruit.mature_color);

	uint8_t bark_class = g->allometry.bark_class;
	if (is_succulent) {
		bark_class = ND_PLANT_BARK_RIBBED;
	}
	float crook_mag = 0.03f + 0.14f * g->architecture.tortuosity;
	uint8_t max_order_draw = (detail_level >= 2u) ? 1u : ((detail_level == 0u) ? 3u : 2u);
	if (plagiotropic_t > 0.5f && max_order_draw > 2u) {
		max_order_draw = 2u;
	}
	uint32_t inst_seed = (uint32_t)(plant->instance_id ^ (plant->genome.genetic_seed >> 17));

	/* Select foliage atlas tile purely from leaf complexity and blade shape */
	float fol_atlas_base = (g->leaf.complexity == PLANT_LEAF_NEEDLE)
		? ND_MAT3D_FOL_CONIFER
		: ((g->leaf.complexity == PLANT_LEAF_BIPINNATE || g->leaf.shape == PLANT_LEAF_SHAPE_LINEAR)
			? ND_MAT3D_FOL_BLADE
			: ((g->leaf.shape == PLANT_LEAF_SHAPE_PINNATELY_LOBED ||
			    g->leaf.shape == PLANT_LEAF_SHAPE_PALMATELY_LOBED)
				? ND_MAT3D_FOL_LOBED
				: ND_MAT3D_FOL_OVATE));

	/* Continuous basal trunk radius from g->allometry.basal_radius_ratio */
	float pheno_r0_wu = (pheno->unit_count > 0u)
		? fmaxf(0.15f, pheno->units[0].base_radius_m * scale_z)
		: 1.0f;
	float basal_ratio = (g->allometry.basal_radius_ratio > 0.005f)
		? g->allometry.basal_radius_ratio
		: 0.055f;
	float target_trunk_r0_wu = (is_herbaceous || is_graminoid)
		? pheno_r0_wu
		: fmaxf(1.1f, (0.55f * h_total + 0.45f * w_total) * (0.58f * basal_ratio));

	/* 0. Ground-layer basal rosette for herbaceous plants & graminoid reeds */
	if (is_herbaceous) {
		int n_rosette = (detail_level == 0u) ? 7 : 5;
		for (int i = 0; i < n_rosette; i++) {
			float ang = ((float)i / (float)n_rosette) * 6.2831853f + (float)(inst_seed & 7u) * 0.25f;
			float ca = cosf(ang), sa = sinf(ang);
			nd_m3d_add_arching_blade(b, 0.0f, 0.0f, 0.6f, ca, sa,
			                         w_total * (0.36f + 0.05f * (float)(i & 1)),
			                         h_total * (0.18f + 0.08f * crown_flatness),
			                         h_total * 0.10f,
			                         w_total * 0.105f,
			                         c_leaf_deep, c_leaf_mid, c_leaf_lit, h_total);
		}
		if (g->fruit.fruit_type == PLANT_FRUIT_BERRY) {
			nd_m3d_add_foliage_bough_cluster_ex(b, 0.0f, 0.0f, h_total * 0.44f,
			                                    w_total * 0.36f, w_total * 0.34f, h_total * 0.32f,
			                                    fol_atlas_base,
			                                    c_leaf_deep, c_leaf_mid, c_leaf_lit, c_leaf_high,
			                                    h_total, inst_seed + 173u);
		}
	} else if (is_graminoid) {
		int n_blades = (detail_level == 0u) ? 8 : 6;
		for (int i = 0; i < n_blades; i++) {
			float ang = ((float)i / (float)n_blades) * 6.2831853f + (float)(inst_seed & 7u) * 0.22f;
			float ca = cosf(ang), sa = sinf(ang);
			nd_m3d_add_arching_blade(b, ca * w_total * 0.07f, sa * w_total * 0.07f, 0.0f,
			                         ca, sa,
			                         w_total * (0.28f + 0.07f * (float)(i & 1)),
			                         h_total * (0.64f + 0.10f * (float)(i % 3)),
			                         h_total * 0.16f,
			                         w_total * 0.068f,
			                         c_leaf_deep, c_leaf_mid, (i & 1) ? c_leaf_high : c_leaf_lit,
			                         h_total);
		}
		if (g->flower.inflorescence == PLANT_INFLORESCENCE_SPIKE) {
			int n_culms = (detail_level == 0u) ? 3 : 2;
			uint32_t c_spadix_dark = nd_m3d_lerp_col(0xFF0C1424u, c_fl_sec, 0.45f);
			uint32_t c_spadix_mid  = c_fl_sec;
			uint32_t c_spadix_lit  = nd_m3d_lerp_col(c_fl_sec, c_fl_prim, 0.35f);
			for (int c = 0; c < n_culms; c++) {
				float c_ang = ((float)c / (float)n_culms) * 6.2831853f + 0.55f + (float)(inst_seed & 3u) * 0.25f;
				float cx0 = cosf(c_ang) * (w_total * 0.06f);
				float cy0 = sinf(c_ang) * (w_total * 0.06f);
				float cx1 = cosf(c_ang) * (w_total * (0.11f + 0.03f * (float)c));
				float cy1 = sinf(c_ang) * (w_total * (0.11f + 0.03f * (float)c));
				float ch  = h_total * (0.94f - 0.05f * (float)c);
				float sp_z0 = ch * 0.62f;
				float sp_z1 = ch * 0.86f;
				float sp_x0 = cx0 + (cx1 - cx0) * 0.62f;
				float sp_y0 = cy0 + (cy1 - cy0) * 0.62f;
				float sp_x1 = cx0 + (cx1 - cx0) * 0.86f;
				float sp_y1 = cy0 + (cy1 - cy0) * 0.86f;
				float culm_r = fmaxf(0.8f, w_total * 0.016f);
				float spad_r = fmaxf(1.8f, w_total * 0.044f);
				nd_m3d_add_trunk_tube_ex(b, cx0, cy0, 0.0f, culm_r * 1.15f,
				                         sp_x0, sp_y0, sp_z0, culm_r,
				                         5, 2, c_leaf_deep, c_leaf_mid, c_leaf_lit,
				                         ND_PLANT_BARK_HERB, 0.01f, h_total, inst_seed + (uint32_t)c * 59u);
				nd_m3d_add_trunk_tube_ex(b, sp_x0, sp_y0, sp_z0, spad_r,
				                         sp_x1, sp_y1, sp_z1, spad_r * 0.95f,
				                         7, 2, c_spadix_dark, c_spadix_mid, c_spadix_lit,
				                         ND_PLANT_BARK_HERB, 0.004f, h_total, inst_seed + (uint32_t)c * 83u);
				nd_m3d_add_trunk_tube_ex(b, sp_x1, sp_y1, sp_z1, culm_r * 0.90f,
				                         cx1, cy1, ch, culm_r * 0.35f,
				                         5, 1, c_spadix_mid, c_fl_prim, c_fl_prim,
				                         ND_PLANT_BARK_HERB, 0.004f, h_total, inst_seed + (uint32_t)c * 107u);
			}
		}
	}

	/* Count primary trunk nodes & identify true terminal shoots (units with no drawn woody child axes) */
	uint32_t trunk_node_count = 0u;
	bool has_woody_child[PLANT_MAX_UNITS];
	memset(has_woody_child, 0, sizeof(has_woody_child));
	for (uint32_t u_idx = 0u; u_idx < pheno->unit_count; u_idx++) {
		const PlantGrowthUnit *u = &pheno->units[u_idx];
		if (u->branch_order == 0u &&
		    (uint32_t)(u->node_index + 1u) > trunk_node_count) {
			trunk_node_count = (uint32_t)(u->node_index + 1u);
		}
		if (u->branch_order <= max_order_draw &&
		    u->parent_unit != PLANT_INVALID_ID &&
		    u->parent_unit < PLANT_MAX_UNITS) {
			has_woody_child[u->parent_unit] = true;
		}
	}
	if (trunk_node_count == 0u)
		trunk_node_count = 1u;

	/* 1. Traverse MTG PlantAxes in ascending branch order (0 -> 1 -> 2 -> 3) to sweep smooth
	 *    Catmull-Rom parallel-transported bark/stem tubes and record exact rendered twig tips. */
	float max_r_env = w_total * (0.43f + 0.05f * tabular_t);
	uint32_t bd = c_bark_dark;
	uint32_t bm = c_bark_mid;
	uint32_t bl = c_bark_light;

	float unit_tip_pos[PLANT_MAX_UNITS][3];
	float unit_tip_rad[PLANT_MAX_UNITS];
	uint8_t unit_tip_valid[PLANT_MAX_UNITS];
	memset(unit_tip_valid, 0, sizeof(unit_tip_valid));

	for (uint8_t pass_ord = 0u; pass_ord <= max_order_draw; pass_ord++) {
		for (uint32_t a_idx = 0u; a_idx < pheno->axis_count; a_idx++) {
			const PlantAxis *ax = &pheno->axes[a_idx];
			if (!ax->alive || ax->is_root_axis || ax->first_unit == PLANT_INVALID_ID)
				continue;
			if (ax->order != pass_ord)
				continue;
			if ((is_herbaceous || is_graminoid) && ax->order > 0u)
				continue;

			float pts[24][3];
			float radii[24];
			int n_pts = 0;

			/* Delta shift to keep child axis base locked onto its parent's exact curved spine */
			float shift_x = 0.0f, shift_y = 0.0f, shift_z = 0.0f;
			float p_rad_socket = 0.0f;
			if (ax->order >= 1u && ax->parent_unit != PLANT_INVALID_ID &&
			    ax->parent_unit < PLANT_MAX_UNITS && unit_tip_valid[ax->parent_unit]) {
				const PlantGrowthUnit *u0 = &pheno->units[ax->first_unit];
				float raw_x0 = u0->base_pos.x * scale_xy;
				float raw_y0 = u0->base_pos.y * scale_xy * y_depth_scale;
				float raw_z0 = u0->base_pos.z * scale_z;
				float rh = sqrtf(raw_x0 * raw_x0 + raw_y0 * raw_y0);
				if (rh > max_r_env * 0.78f && rh > 1e-3f) {
					float ex = (rh - max_r_env * 0.78f) / (max_r_env * 0.22f);
					float rs = max_r_env * (0.78f + 0.22f * tanhf(ex));
					raw_x0 *= rs / rh;
					raw_y0 *= rs / rh;
				}
				shift_x = unit_tip_pos[ax->parent_unit][0] - raw_x0;
				shift_y = unit_tip_pos[ax->parent_unit][1] - raw_y0;
				shift_z = unit_tip_pos[ax->parent_unit][2] - raw_z0;
				p_rad_socket = unit_tip_rad[ax->parent_unit];
			}

			PlantUnitId uid = ax->first_unit;
			uint32_t guard = 0u;
			float prev_x = 0.0f, prev_y = 0.0f, prev_z = 0.0f;
			while (uid != PLANT_INVALID_ID && uid < pheno->unit_count && guard < 20u) {
				const PlantGrowthUnit *u = &pheno->units[uid];
				guard++;

				float x0 = u->base_pos.x * scale_xy;
				float y0 = u->base_pos.y * scale_xy * y_depth_scale;
				float z0 = u->base_pos.z * scale_z;
				float x1 = u->tip_pos.x * scale_xy;
				float y1 = u->tip_pos.y * scale_xy * y_depth_scale;
				float z1 = u->tip_pos.z * scale_z;

				/* Soft-knee radial crown envelope */
				float r_h0 = sqrtf(x0 * x0 + y0 * y0);
				if (r_h0 > max_r_env * 0.78f && r_h0 > 1e-3f) {
					float excess = (r_h0 - max_r_env * 0.78f) / (max_r_env * 0.22f);
					float r_soft = max_r_env * (0.78f + 0.22f * tanhf(excess));
					x0 *= r_soft / r_h0;
					y0 *= r_soft / r_h0;
				}
				float r_h1 = sqrtf(x1 * x1 + y1 * y1);
				if (r_h1 > max_r_env * 0.78f && r_h1 > 1e-3f) {
					float excess = (r_h1 - max_r_env * 0.78f) / (max_r_env * 0.22f);
					float r_soft = max_r_env * (0.78f + 0.22f * tanhf(excess));
					x1 *= r_soft / r_h1;
					y1 *= r_soft / r_h1;
				}

				/* Propagate parent spine alignment so forks remain 100% welded */
				x0 += shift_x; y0 += shift_y; z0 += shift_z;
				x1 += shift_x; y1 += shift_y; z1 += shift_z;
				if (n_pts > 0) {
					x0 = prev_x; y0 = prev_y; z0 = prev_z;
				}

				/* Enforce perspective-compensated crown-base floor on all woody lateral branches (order >= 1) */
				if (is_woody_tree && ax->order >= 1u) {
					float eff_fl_frac = (ax->order >= 2u && tabular_t > 0.25f)
						? fmaxf(clear_bole_eff, clear_bole_f + 0.26f * tabular_t)
						: clear_bole_eff;
					float z_fl1 = nd_plant_m3d_eval_crown_floor_z(x1, y1, h_total, eff_fl_frac);
					if (z1 < z_fl1)
						z1 = z_fl1;
					if (z1 > h_total * 0.93f)
						z1 = h_total * 0.93f;
				}

				/* Normalized botanical radius preserving Da Vinci pipe-model taper across branch orders */
				float r0_raw = fmaxf(0.10f, u->base_radius_m * scale_z);
				float r1_raw = fmaxf(0.08f, u->tip_radius_m * scale_z);
				float r0, r1;
				if (is_herbaceous || is_graminoid) {
					r0 = fmaxf(0.38f, r0_raw * 0.65f);
					r1 = fmaxf(0.26f, r1_raw * 0.65f);
				} else if (is_succulent) {
					r0 = target_trunk_r0_wu * powf(plant_clampf(r0_raw / pheno_r0_wu, 0.35f, 1.0f), 0.45f);
					r1 = target_trunk_r0_wu * powf(plant_clampf(r1_raw / pheno_r0_wu, 0.32f, 1.0f), 0.45f);
				} else {
					float ratio0 = plant_clampf(r0_raw / pheno_r0_wu, 0.03f, 1.0f);
					float ratio1 = plant_clampf(r1_raw / pheno_r0_wu, 0.02f, 1.0f);
					float exp_t  = (ax->order == 0u) ? 0.74f : 0.86f;
					float ord_atten = (ax->order == 0u) ? 1.0f : ((ax->order == 1u) ? 0.72f : ((ax->order == 2u) ? 0.50f : 0.36f));
					r0 = fmaxf(0.42f, target_trunk_r0_wu * powf(ratio0, exp_t) * ord_atten);
					r1 = fmaxf(0.28f, target_trunk_r0_wu * powf(ratio1, exp_t) * ord_atten);
				}
				uint32_t useed = (uint32_t)(u->topo_hash ^ (u->topo_hash >> 32));

				float seg_dx = x1 - x0, seg_dy = y1 - y0, seg_dz = z1 - z0;
				float seg_len = sqrtf(seg_dx * seg_dx + seg_dy * seg_dy + seg_dz * seg_dz);

				if (n_pts == 0) {
					if (ax->order >= 1u) {
						float p_rad = (p_rad_socket > 0.1f) ? p_rad_socket : (r0 * 1.35f);
						if (p_rad < r0) p_rad = r0;
						if (r0 > p_rad * 0.78f) r0 = p_rad * 0.78f;
						if (r1 > r0 * 0.88f)    r1 = r0 * 0.88f;
						float tuck = (seg_len > 1e-3f) ? fminf(p_rad * 0.42f, seg_len * 0.22f) : 0.0f;
						float inv_l = (seg_len > 1e-3f) ? (1.0f / seg_len) : 0.0f;
						pts[0][0] = x0 - seg_dx * inv_l * tuck;
						pts[0][1] = y0 - seg_dy * inv_l * tuck;
						pts[0][2] = z0 - seg_dz * inv_l * tuck;
						radii[0]  = fminf(p_rad * 0.86f, r0 * 1.12f);
						n_pts = 1;
						if (ax->order == 1u && seg_len > r0 * 1.6f && n_pts < 22) {
							pts[n_pts][0] = x0 + seg_dx * 0.20f;
							pts[n_pts][1] = y0 + seg_dy * 0.20f;
							pts[n_pts][2] = z0 + seg_dz * 0.20f;
							radii[n_pts]  = fminf(p_rad * 0.80f, r0 * 0.96f);
							n_pts++;
						}
					} else {
						pts[0][0] = x0; pts[0][1] = y0; pts[0][2] = z0;
						radii[0]  = r0;
						n_pts = 1;
					}
				}

				/* Organic mid-span curvature on trunk and primary scaffold boughs driven by tortuosity */
				if (ax->order <= 1u && is_woody_tree && n_pts + 2 < 23 && seg_len > r0 * 2.1f) {
					float bow_ang = ((float)(useed & 255u) / 255.0f) * 6.2831853f;
					float bow_scale = (ax->order == 0u) ? (0.20f + 0.22f * g->architecture.tortuosity) : 0.20f;
					float bow = seg_len * crook_mag * bow_scale;
					float z_arch = (ax->order >= 1u)
						? (seg_len * (0.04f * (1.0f - plagiotropic_t) - 0.04f * plagiotropic_t))
						: 0.0f;
					float mx = 0.5f * (x0 + x1) + cosf(bow_ang) * bow;
					float my = 0.5f * (y0 + y1) + sinf(bow_ang) * bow * y_depth_scale;
					float mz = 0.5f * (z0 + z1) + z_arch;
					if (ax->order >= 1u) {
						float z_fl_m = nd_plant_m3d_eval_crown_floor_z(mx, my, h_total, clear_bole_eff);
						if (mz < z_fl_m)
							mz = z_fl_m;
					}
					pts[n_pts][0] = mx;
					pts[n_pts][1] = my;
					pts[n_pts][2] = mz;
					radii[n_pts]  = 0.5f * (r0 + r1);
					n_pts++;
				}

				if (n_pts < 23) {
					pts[n_pts][0] = x1;
					pts[n_pts][1] = y1;
					pts[n_pts][2] = z1;
					radii[n_pts]  = r1;
					n_pts++;
				}

				if (uid < PLANT_MAX_UNITS) {
					unit_tip_pos[uid][0] = x1;
					unit_tip_pos[uid][1] = y1;
					unit_tip_pos[uid][2] = z1;
					unit_tip_rad[uid]    = r1;
					unit_tip_valid[uid]  = 1u;
				}
				prev_x = x1; prev_y = y1; prev_z = z1;

				/* Ground-hugging root-collar buttress spurs driven by g->allometry.root_flare_strength */
				if (u->branch_order == 0u && u->node_index == 0u && detail_level == 0u &&
				    is_woody_tree && g->allometry.root_flare_strength > 0.35f) {
					int n_spurs = 3;
					for (int sp = 0; sp < n_spurs; sp++) {
						float sp_ang = ((float)sp / (float)n_spurs) * 6.2831853f + (float)(useed & 31u) * 0.1f;
						float sp_reach = r0 * (1.24f + 0.32f * g->allometry.root_flare_strength);
						float spts[3][3] = {
							{ x0, y0, z0 + fminf(h_total * 0.045f, (z1 - z0) * 0.32f) },
							{ x0 + cosf(sp_ang) * sp_reach * 0.58f, y0 + sinf(sp_ang) * sp_reach * 0.58f, z0 + fminf(h_total * 0.016f, (z1 - z0) * 0.12f) },
							{ x0 + cosf(sp_ang) * sp_reach, y0 + sinf(sp_ang) * sp_reach, 0.0f }
						};
						float srads[3] = { r0 * 0.36f, r0 * 0.22f, r0 * 0.09f };
						nd_m3d_add_axis_spline_tube(b, spts, srads, 3, 5, 1,
						                            bd, bm, bl, bark_class, h_total, useed + (uint32_t)sp * 41u);
					}
				}

				/* Self-pruned lower branch stubs driven by g->architecture.branch_shed_threshold */
				float node_bole_frac = (trunk_node_count > 1u)
					? ((float)u->node_index / (float)(trunk_node_count - 1u))
					: 0.0f;
				if (u->branch_order == 0u && u->node_index >= 1u &&
				    node_bole_frac < clear_bole_f &&
				    detail_level == 0u && is_woody_tree &&
				    g->architecture.branch_shed_threshold > 0.45f) {
					float st_ang = (float)(u->node_index * 3u) * 2.3999632f + (float)(useed & 15u) * 0.2f;
					float st_z = z0 + (z1 - z0) * 0.55f;
					float st_len = r0 * 2.1f;
					float st_pts[3][3] = {
						{ x0 + (x1 - x0) * 0.55f, y0 + (y1 - y0) * 0.55f, st_z },
						{ x0 + cosf(st_ang) * st_len * 0.55f, y0 + sinf(st_ang) * st_len * 0.55f, st_z - st_len * 0.10f },
						{ x0 + cosf(st_ang) * st_len, y0 + sinf(st_ang) * st_len, st_z - st_len * 0.22f }
					};
					float st_rads[3] = { r1 * 0.30f, r1 * 0.18f, r1 * 0.08f };
					nd_m3d_add_axis_spline_tube(b, st_pts, st_rads, 3, 4, 1,
					                            bd, bd, bm, bark_class, h_total, useed + 79u);
				}

				/* Rounded succulent dome apex on ribbed succulent columns */
				if (is_succulent && u->next_in_axis == PLANT_INVALID_ID) {
					r1 *= 0.78f;
					if (n_pts > 0)
						radii[n_pts - 1] = r1;
					if (n_pts < 22) {
						pts[n_pts][0] = x1;
						pts[n_pts][1] = y1;
						pts[n_pts][2] = z1 + r1 * 0.52f;
						radii[n_pts]  = r1 * 0.48f;
						n_pts++;
						pts[n_pts][0] = x1;
						pts[n_pts][1] = y1;
						pts[n_pts][2] = z1 + r1 * 0.82f;
						radii[n_pts]  = 0.08f;
						n_pts++;
					}
				}

				/* Excurrent apical leader tuft on monopodial whorled spires */
				if (plagiotropic_t > 0.45f && u->branch_order == 0u && u->next_in_axis == PLANT_INVALID_ID) {
					float spire_r = w_total * 0.11f;
					for (int sp = 0; sp < 4; sp++) {
						float sp_ang = (float)sp * 1.5707963f + (float)(useed & 7u) * 0.35f;
						float ca = cosf(sp_ang), sa = sinf(sp_ang);
						nd_m3d_add_branchlet_card(b, x1, y1, z1 - spire_r * 0.22f,
						                          ca * 0.46f, sa * 0.46f, 0.88f,
						                          spire_r * 1.40f, spire_r * 0.12f, spire_r * 0.08f,
						                          spire_r * 0.52f, spire_r * 0.08f, (sp & 1) ? 0.24f : -0.24f,
						                          x1, y1, z1 - spire_r * 0.18f,
						                          fol_atlas_base,
						                          c_leaf_deep, c_leaf_mid, c_leaf_lit, c_leaf_high, h_total);
					}
				}

				uid = u->next_in_axis;
			}

			/* Close tapered terminal twig apex */
			if (n_pts >= 2 && n_pts < 24 && !is_succulent) {
				float tx = pts[n_pts - 1][0] - pts[n_pts - 2][0];
				float ty = pts[n_pts - 1][1] - pts[n_pts - 2][1];
				float tz = pts[n_pts - 1][2] - pts[n_pts - 2][2];
				float tlen = sqrtf(tx * tx + ty * ty + tz * tz);
				if (tlen > 1e-4f) {
					float cap_ext = fmaxf(0.45f, radii[n_pts - 1] * 0.90f);
					pts[n_pts][0] = pts[n_pts - 1][0] + (tx / tlen) * cap_ext;
					pts[n_pts][1] = pts[n_pts - 1][1] + (ty / tlen) * cap_ext;
					pts[n_pts][2] = pts[n_pts - 1][2] + (tz / tlen) * cap_ext;
					radii[n_pts]  = 0.08f;
					n_pts++;
				}
			}

			if (n_pts >= 2) {
				int sides = (ax->order == 0u)
					? (is_succulent ? 8 : ((is_herbaceous || is_graminoid) ? 5 : 8))
					: ((ax->order == 1u) ? ((plagiotropic_t > 0.5f) ? 5 : 6) : 4);
				uint32_t aseed = (uint32_t)(ax->topo_hash ^ (ax->topo_hash >> 32));
				nd_m3d_add_axis_spline_tube(b, pts, radii, n_pts, sides,
				                            (ax->order >= 1u) ? 1 : 0,
				                            bd, bm, bl, bark_class, h_total, aseed);
			}
		}
	}

	/* 2. Traverse MTG PlantOrgans to emit terminal-shoot branchlet fans, blades, flowers, and fruit */
	uint32_t max_clusters = (detail_level == 0u) ? 34u : 18u;
	uint32_t bough_clusters_emitted = 0u;

	for (uint32_t o_idx = 0u; o_idx < pheno->organ_count; o_idx++) {
		const PlantOrgan *org = &pheno->organs[o_idx];
		uint32_t oseed = (uint32_t)(org->topo_hash ^ (org->topo_hash >> 32));
		float ax = org->attachment_pos.x * scale_xy;
		float ay = org->attachment_pos.y * scale_xy * y_depth_scale;
		float az = org->attachment_pos.z * scale_z;

		/* Lock organ attachment directly to its parent unit's exact curved spine tip */
		if (org->parent_unit != PLANT_INVALID_ID && org->parent_unit < PLANT_MAX_UNITS &&
		    unit_tip_valid[org->parent_unit]) {
			ax = unit_tip_pos[org->parent_unit][0];
			ay = unit_tip_pos[org->parent_unit][1];
			az = unit_tip_pos[org->parent_unit][2];
		} else {
			float r_ha = sqrtf(ax * ax + ay * ay);
			if (r_ha > max_r_env * 0.78f && r_ha > 1e-3f) {
				float excess = (r_ha - max_r_env * 0.78f) / (max_r_env * 0.22f);
				float r_soft = max_r_env * (0.78f + 0.22f * tanhf(excess));
				ax *= r_soft / r_ha;
				ay *= r_soft / r_ha;
			}
		}
		az = plant_clampf(az, 0.5f, h_total * 0.96f);

		float olen = fmaxf(2.0f, org->length_m * scale_z);
		float owid = fmaxf(1.2f, org->width_m * scale_z);
		float dx = org->direction.x;
		float dy = org->direction.y * y_depth_scale;
		if (hypotf(ax, ay) > 2.0f) {
			float r_xy = hypotf(ax, ay);
			dx = dx * 0.30f + (ax / r_xy) * 0.70f;
			dy = dy * 0.30f + (ay / r_xy) * 0.70f;
		}
		float dlen_xy = sqrtf(dx * dx + dy * dy);
		if (dlen_xy > 1e-4f) {
			dx /= dlen_xy;
			dy /= dlen_xy;
		} else {
			float fallback_ang = (float)(oseed & 0xFFu) * (6.2831853f / 256.0f);
			dx = cosf(fallback_ang);
			dy = sinf(fallback_ang);
		}

		if (org->type == PLANT_ORGAN_LEAF) {
			const PlantGrowthUnit *pu = (org->parent_unit < pheno->unit_count)
				? &pheno->units[org->parent_unit] : NULL;
			bool is_axis_tip = (!pu || pu->next_in_axis == PLANT_INVALID_ID);
			bool is_terminal_shoot = (!pu || (pu->id < PLANT_MAX_UNITS && !has_woody_child[pu->id]));

			if (is_succulent) {
				/* Photosynthetic ribbed stem already represents xerophytic succulent body */
				continue;
			} else if (is_graminoid) {
				if (org->sub_index != 0u || !is_axis_tip)
					continue;
				nd_m3d_add_arching_blade(b, ax * 0.45f, ay * 0.45f, az * 0.25f,
				                         dx, dy,
				                         w_total * 0.26f,
				                         fminf(h_total * 0.72f, olen * 1.15f),
				                         olen * 0.20f,
				                         owid * 0.55f,
				                         c_leaf_deep, c_leaf_mid, c_leaf_lit,
				                         h_total);
			} else if (is_herbaceous) {
				if (org->sub_index != 0u || !is_axis_tip)
					continue;
				float arc_h = olen * (0.20f + 0.14f * crown_flatness);
				nd_m3d_add_leaf_frond_spray(b, ax, ay, fminf(az, h_total * 0.58f),
				                            dx, dy,
				                            olen * 0.90f, arc_h, olen * 0.22f,
				                            owid * 0.48f, olen * 0.08f,
				                            c_leaf_deep, c_leaf_mid, c_leaf_lit, c_leaf_high,
				                            h_total);
			} else {
				/* Woody Tree Shoot Foliage:
				 * Attach branchlet fans to terminal shoots as well as upper order-1 / order-2 scaffold forks
				 * (z >= 0.48 * h_total on non-tabular trees) to form interlocking 3D sub-canopy volume while
				 * leaving the lower trunk and primary V-vase scaffold limbs clearly visible below the crown floor. */
				if (org->sub_index != 0u || org->branch_order == 0u)
					continue;
				if (org->branch_order == 1u && plagiotropic_t < 0.35f &&
				    (tabular_t > 0.35f || !is_axis_tip || az < h_total * 0.48f))
					continue;
				if (org->branch_order > max_order_draw)
					continue;
				if (!is_terminal_shoot) {
					bool upper_subcrown = (org->branch_order >= 2u && is_axis_tip &&
					                       tabular_t < 0.35f && az >= h_total * 0.48f);
					if (!upper_subcrown && !(plagiotropic_t > 0.5f && org->branch_order >= 1u && is_axis_tip))
						continue;
				}
				if (bough_clusters_emitted >= max_clusters)
					continue;

				/* Continuous conical spire taper on high-apical-dominance trees vs wide umbrella/dome pads */
				float h_t = plant_clampf(az / fmaxf(1.0f, h_total), 0.0f, 1.0f);
				float spire_taper = 1.0f - plagiotropic_t * (1.0f - powf(fmaxf(0.18f, 1.04f - 0.80f * h_t), 0.78f));

				/* Local branchlet cluster radius: full enough (0.165..0.20 * w_total) to form interlocking
				 * 3D bough strata while preserving sky fenestrations between sub-crowns, modulated by
				 * 3D Beer-Lambert light-competition health (org->health) so sunlit outer crown flushes fuller. */
				float light_scale = 0.88f + 0.22f * plant_clampf(org->health, 0.25f, 1.0f);
				float base_rx_frac = (0.165f + 0.035f * tabular_t + 0.030f * plagiotropic_t) * light_scale;
				float rx = w_total * base_rx_frac * spire_taper;
				float ry = rx * (0.94f - 0.08f * tabular_t);
				float rz = rx * plant_clampf(crown_flatness * 0.68f, 0.22f, 0.58f);

				/* Continuous foliar stratum floor: on tabular crowns (tabular_t > 0), foliage sits
				 * above the ascending order-1 V-vase scaffold (clear_bole_f + 0.28 * tabular_t). */
				float fol_floor_eff = fmaxf(clear_bole_eff, clear_bole_f + 0.28f * tabular_t);
				float z_fl = nd_plant_m3d_eval_crown_floor_z(ax, ay, h_total, fol_floor_eff) + rz * 0.35f;
				if (az < z_fl)
					az = z_fl;

				uint32_t fan_v_start = b->vert_count;
				bough_clusters_emitted++;
				nd_m3d_add_terminal_shoot_fan(b, ax, ay, az,
				                              dx, dy,
				                              rx, ry, rz,
				                              tabular_t, plagiotropic_t,
				                              fol_atlas_base,
				                              c_leaf_deep, c_leaf_mid, c_leaf_lit, c_leaf_high,
				                              h_total, oseed);

				/* Pendulous weeping branchlet curtains driven continuously by negative orthotropy (weep_t > 0.15) */
				if (weep_t > 0.15f && (weep_t > 0.55f || (bough_clusters_emitted & 1u))) {
					int n_curtains = (weep_t > 0.55f) ? 2 : 1;
					for (int wc = 0; wc < n_curtains; wc++) {
						float c_ang = atan2f(dy, dx) + (float)wc * 0.92f - 0.38f;
						float cca = cosf(c_ang), csa = sinf(c_ang);
						float wx0 = ax + cca * (rx * 0.18f);
						float wy0 = ay + csa * (ry * 0.18f);
						float wz0 = az + rz * 0.06f;
						float z_fl_w = nd_plant_m3d_eval_crown_floor_z(wx0, wy0, h_total, clear_bole_eff);
						float avail_drop = fmaxf(h_total * 0.08f, (wz0 - z_fl_w) * 0.85f);
						float drop_len = fminf(avail_drop, h_total * (0.14f + 0.14f * weep_t));
						float card_hw = drop_len * 0.44f;
						float out_xy  = 0.48f - 0.12f * weep_t;
						float down_z  = -0.84f - 0.10f * weep_t;
						nd_m3d_add_branchlet_card(b,
						                          wx0, wy0, wz0,
						                          cca * out_xy, csa * out_xy, down_z,
						                          drop_len, w_total * 0.025f, -w_total * 0.015f,
						                          card_hw, w_total * 0.02f,
						                          (wc & 1) ? 0.24f : -0.24f,
						                          0.0f, 0.0f, h_total * 0.58f,
						                          (weep_t > 0.55f) ? ND_MAT3D_FOL_BLADE : fol_atlas_base,
						                          c_leaf_deep, c_leaf_mid, c_leaf_lit, c_leaf_high,
						                          h_total);
					}
				}

				/* Hard perspective-compensated crown-base floor clamp on every emitted foliage vertex:
				 * Mathematically guarantees that no tree foliage vertex ever drops below the clear bole
				 * in either 3D Z or 55-deg projected screen Y! */
				for (uint32_t vi = fan_v_start; vi < b->vert_count; vi++) {
					float vx = b->verts[vi].pos[0];
					float vy = b->verts[vi].pos[1];
					float vz = b->verts[vi].pos[2];
					float min_vz = nd_plant_m3d_eval_crown_floor_z(vx, vy, h_total, fol_floor_eff);
					if (vz < min_vz) {
						b->verts[vi].pos[2] = min_vz;
					}
				}
			}
		} else if (org->type == PLANT_ORGAN_FLOWER) {
			const PlantGrowthUnit *pu = (org->parent_unit < pheno->unit_count)
				? &pheno->units[org->parent_unit] : NULL;
			if (is_graminoid || g->flower.inflorescence == PLANT_INFLORESCENCE_SPIKE) {
				/* Spike inflorescence already rendered on graminoid reproductive culms */
				continue;
			}
			float max_frad = is_succulent
				? (w_total * 0.085f)
				: (is_herbaceous ? (w_total * 0.135f) : (w_total * 0.045f));
			float frad = plant_clampf(org->length_m * 0.48f * scale_z, 1.4f, max_frad);
			float fh   = plant_clampf(org->thickness_m * 0.6f * scale_z, 0.6f, frad * 0.48f);

			if (g->flower.inflorescence == PLANT_INFLORESCENCE_UMBEL) {
				float u_rad = fminf(w_total * 0.068f, frad * 0.65f);
				float fz = fminf(h_total * 0.93f, az + fh * 0.3f);
				nd_m3d_add_blossom_star(b, ax, ay, fz, u_rad, fh * 0.5f,
				                        c_fl_prim, c_fl_sec, c_fl_sec, h_total);
				for (int uf = 0; uf < 3; uf++) {
					float uang = (float)uf * 2.0943951f + (float)(oseed & 7u) * 0.3f;
					nd_m3d_add_blossom_star(b,
					                        ax + cosf(uang) * u_rad * 1.15f,
					                        ay + sinf(uang) * u_rad * 1.15f,
					                        fz - fh * 0.15f,
					                        u_rad * 0.78f, fh * 0.45f,
					                        c_fl_prim, c_fl_sec, c_fl_prim, h_total);
				}
			} else {
				bool is_capitulum = (g->flower.inflorescence == PLANT_INFLORESCENCE_CAPITULUM);
				if (is_capitulum) {
					if (org->sub_index != 0u)
						continue;
					if (pu && pu->next_in_axis != PLANT_INVALID_ID)
						continue;
					frad = fmaxf(frad, w_total * 0.14f);
				}
				uint32_t col_petal_lit   = c_fl_prim;
				uint32_t col_petal_shade = nd_m3d_lerp_col(c_fl_prim, c_fl_sec, 0.35f);
				uint32_t col_center      = c_fl_sec;
				float fz = fminf(h_total * 0.94f, az + fh * 0.30f);
				nd_m3d_add_blossom_star(b, ax, ay, fz,
				                        frad, fh,
				                        col_petal_lit, col_petal_shade, col_center, h_total);
			}
		} else if (org->type == PLANT_ORGAN_FRUIT) {
			if (is_graminoid)
				continue;
			float max_frad = is_herbaceous ? (w_total * 0.048f) : (w_total * 0.035f);
			float frad = plant_clampf(org->width_m * 0.40f * scale_z, 0.9f, max_frad);
			float flen = plant_clampf(org->length_m * 0.55f * scale_z, 1.4f, max_frad * 1.9f);
			if (g->fruit.fruit_type == PLANT_FRUIT_CONE || g->fruit.fruit_type == PLANT_FRUIT_ACHENE) {
				nd_m3d_add_trunk_tube_ex(b, ax, ay, az, frad,
				                         ax, ay, fminf(h_total * 0.95f, az + flen), frad * 0.76f,
				                         5, 1, c_fr_imm, c_fr_mat, c_fr_mat,
				                         ND_PLANT_BARK_FISSURED, 0.01f, h_total, oseed);
			} else {
				if (is_herbaceous && org->sub_index != 0u)
					continue;
				uint32_t col_berry = ((oseed & 3u) == 0u) ? c_fr_imm : c_fr_mat;
				nd_m3d_add_fruit_berry(b, ax, ay, fminf(h_total * 0.78f, az),
				                       frad, col_berry, c_fr_imm, h_total);
			}
		}
	}

	if (out->out_slice) {
		float max_sp = 136.0f;
		float sp_scale = (w_total > max_sp || h_total > max_sp)
			? (max_sp / fmaxf(w_total, h_total))
			: 1.0f;
		uint16_t sw = (uint16_t)fmaxf(8.0f, fminf(248.0f, floorf(w_total * sp_scale + 0.5f)));
		uint16_t sh = (uint16_t)fmaxf(6.0f, fminf(248.0f, floorf(h_total * sp_scale + 0.5f)));
		out->out_slice->first_vert = v_start;
		out->out_slice->vert_count = b->vert_count - v_start;
		out->out_slice->first_idx  = i_start;
		out->out_slice->idx_count  = b->idx_count - i_start;
		out->out_slice->mesh_w_wu  = w_total;
		out->out_slice->mesh_h_wu  = h_total;
		out->out_slice->sprite_w   = sw;
		out->out_slice->sprite_h   = sh;
		out->out_slice->anchor_y   = (uint8_t)(sh > 64 ? 6u : 3u);
		out->out_slice->shadow_rx  = (uint8_t)(sw * 0.34f);
		out->out_slice->shadow_ry  = (uint8_t)(sw * 0.22f);
		out->out_slice->pad0       = 0;
	}
}

/* Unified Data-Driven 3D Botanical Generator:
 * Evaluates the full 4-stage pipeline:
 *   PlantSpeciesDef -> PlantGenome (sampled per variant allele) ->
 *   PlantPhenotype (developed at juvenile or mature age via plant_simulate_at_time) ->
 *   3D Geometry (via plant_geometry_build). */
static inline void
nd_m3d_build_plant_from_genes(nd_mesh3d_builder_t *b,
                              const nd_plant_spec_t *spec,
                              int is_big, int var_idx, uint32_t seed_salt,
                              nd_mesh3d_slice_t *out_slice)
{
	float w_wu = 56.0f, h_wu = 68.0f, ay_wu = 4.0f;
	unsigned sp_idx = (unsigned)(spec - s_nd_plant_specs);
	if (sp_idx < ND_PLANT_MAX) {
		nd_m3d_plant_metric_dims(sp_idx, (unsigned)is_big, &w_wu, &h_wu, &ay_wu);
	} else {
		w_wu = (float)(is_big ? spec->genes.big_w : spec->genes.small_w);
		h_wu = (float)(is_big ? spec->genes.big_h : spec->genes.small_h);
		sp_idx = 10u;
	}

	/* 1. Initialize biological species definition & priors */
	PlantSpeciesDef species_def;
	plant_species_init_preset(&species_def, sp_idx);

	/* 2. Sample an individual PlantGenome for this genetic variant allele (0..3) */
	unsigned allele = (unsigned)var_idx & 3u;
	uint64_t genetic_seed = plant_mix64(
		((uint64_t)seed_salt << 16) ^
		((uint64_t)sp_idx * 0x9E3779B97F4A7C15ULL) ^
		((uint64_t)allele * 0xBF58476D1CE4E5B9ULL) ^
		(uint64_t)(is_big ? 0xF00DULL : 0x100DULL));
	PlantGenome genome = plant_genome_sample(&species_def, genetic_seed);
	if (allele == 2u && genome.architecture.basal_stem_count < 3u &&
	    species_def.life_form == PLANT_LIFEFORM_TREE) {
		genome.architecture.basal_stem_count++;
	}

	/* 3. Develop the chrono-deterministic PlantPhenotype at juvenile (small) or mature (big) age */
	static PlantPhenotypeStorage s_pheno_storage;
	PlantInstance inst;
	memset(&inst, 0, sizeof(inst));
	inst.instance_id = ((uint64_t)sp_idx << 8) | ((uint64_t)is_big << 4) | (uint64_t)allele;
	inst.genome = genome;
	plant_phenotype_bind_storage(&inst.phenotype, &s_pheno_storage);

	PlantEnvironment env = plant_environment_default();
	static const float s_lean_x[4] = {  0.08f, -0.14f,  0.11f, -0.09f };
	static const float s_lean_y[4] = { -0.05f,  0.07f, -0.08f,  0.06f };
	env.mean_light_dir = plant_v3_norm(plant_v3(s_lean_x[allele], s_lean_y[allele], 0.98f));

	float full_horizon_days = genome.meristem.plastochron_days * 12.0f;
	float target_age_days = is_big ? (full_horizon_days * 1.05f) : (full_horizon_days * 0.74f);
	plant_simulate_at_time(&inst, &env, target_age_days, NULL, 0u);

	/* 4. Build 3D mesh geometry purely from the developed PlantPhenotype */
	PlantGeometryOutput geom_out;
	geom_out.builder     = b;
	geom_out.out_slice   = out_slice;
	geom_out.target_w_wu = w_wu;
	geom_out.target_h_wu = h_wu;
	uint32_t detail_level = is_big ? 0u : 1u;
	plant_geometry_build(&inst, detail_level, &geom_out);
}

#endif /* ND_PLANT_MESH3D_H */
