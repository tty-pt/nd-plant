/* plant.h — nd-plant's cross-module type: the map tile a plant grows on.
 *
 * plant_tile_t is read by nd-mob (spawn density) and the engine's map layer,
 * so it lives here rather than in the provider TU. There are no XY_DECLs in
 * this header; nd-plant implements engine hooks only.
 *
 * NOTE: this is a MODULE-OWNED header, not an engine one. The old location was
 * `include/uapi/plant.h`; the old `~/nd/module.mk` installed it as
 * `$(PREFIX)/include/nd/plant.h`, so `nd/` is this header's home and it is
 * installed here with `FOLDER := nd`.
 */

#ifndef ND_PLANT_H
#define ND_PLANT_H

#include <stdint.h>
#include <string.h>
#include <nd/plant-types.h>

#define ND_PLANT_MAX   16u
#define ND_PLANTS_SEED 5u
#define ND_PLANT_MASK  0x3u
#define ND_PLANT_HALF  (ND_PLANT_MASK >> 1)
#define ND_PLANT_N(pln, i) (((pln) >> ((i) * 2)) & 3u)

#ifndef ND_XXH32_U32_DEFINED
#define ND_XXH32_U32_DEFINED 1
static inline uint32_t
nd_xxh32_u32(uint32_t val, uint32_t seed)
{
	uint32_t h32 = seed + 0x165667B1u + 4u;
	h32 += val * 0xC2B2AE3Du;
	h32 = ((h32 << 17) | (h32 >> 15)) * 0x27D4EB2Fu;
	h32 ^= h32 >> 15;
	h32 *= 0x85EBCA77u;
	h32 ^= h32 >> 13;
	h32 *= 0xC2B2AE3Du;
	h32 ^= h32 >> 16;
	return h32;
}
#endif

/* Ground-plane collision footprint tiers relative to bottom-center (0, 0) anchor */
#define ND_PLANT_COL_NONE 0u /* Walkable ground flora / crop / herb */
#define ND_PLANT_COL_STD  1u /* Standard 1-cell tree trunk / cactus base */
#define ND_PLANT_COL_WIDE 2u /* Wide mature trunk base */

/* Packed per-slot wire byte (nd_map_tile_t.plant_id[s]):
 * bits 0..4: 1-based species index (1..16, 0 = none)
 * bit 5:     growth stage (0 = small, 1 = big)
 * bit 6:     art variant index (0..1 -> variants 1..2) */
#define ND_PLANT_PACK_ID(idx, big, var0) \
	((uint8_t)((((idx) + 1u) & 0x1Fu) | ((big) ? 0x20u : 0u) | (((var0) & 1u) << 6)))
#define ND_PLANT_UNPACK_PRESENT(pid) (((pid) & 0x1Fu) != 0u && (((pid) & 0x1Fu) <= ND_PLANT_MAX))
#define ND_PLANT_UNPACK_IDX(pid)     ((unsigned)(((pid) & 0x1Fu) ? (((pid) & 0x1Fu) - 1u) : 0u))
#define ND_PLANT_UNPACK_BIG(pid)     ((unsigned)(((pid) >> 5) & 1u))
#define ND_PLANT_UNPACK_VAR(pid)     ((unsigned)((((pid) >> 6) & 1u) + 1u))

typedef struct {
	int16_t     tmp_min, tmp_max;
	uint16_t    rn_min, rn_max;
	uint8_t     y;
	char        small, big;
	uint8_t     fg;
	uint8_t     small_art, big_art;
	uint8_t     small_col, big_col;
	const char *name;
	const char *slug;
} nd_plant_spec_t;

/* Canonical 16-species envelope table matching external/nd-plant/data/plants.dsv */
static const nd_plant_spec_t s_nd_plant_specs[ND_PLANT_MAX] = {
	/* 0  */ {  10,  78,  50, 1024,  2, 'x', 'X', 10, 2, 2, 1, 1, "pinus sylvestris",         "pinus_sylvestris" },
	/* 1  */ {  15,  92, 160,  380,  2, 't', 'T', 10, 2, 2, 1, 1, "pseudotsuga menziesii",    "pseudotsuga_menziesii" },
	/* 2  */ {  25, 116,  30,  385,  2, 'x', 'X',  3, 2, 2, 1, 1, "betula pendula",           "betula_pendula" },
	/* 3  */ {  30,  95,  20,  341,  6, 'x', 'X',  3, 2, 2, 0, 0, "linum usitatissimum",      "linum_usitatissimum" },
	/* 4  */ {  35, 125, 240,  900,  2, 'x', 'X',  7, 2, 2, 1, 1, "betula pubescens",         "betula_pubescens" },
	/* 5  */ { -40,  72,  80,  320,  2, 'a', 'A', 10, 2, 2, 1, 2, "abies alba",               "abies_alba" },
	/* 6  */ { 105, 190,  10,  175,  3, 'i', 'I', 10, 2, 2, 1, 1, "arthrocereus rondonianus", "arthrocereus_rondonianus" },
	/* 7  */ { 115, 168,  40,  290,  2, 't', 'T', 11, 2, 2, 1, 1, "acacia senegal",           "acacia_senegal" },
	/* 8  */ {  38, 105,  80,  250,  4, 'x', 'X',  7, 2, 2, 0, 0, "daucus carota",            "daucus_carota" },
	/* 9  */ {  50, 115,  80,  250,  4, 'x', 'X',  1, 2, 2, 0, 0, "solanum lycopersicum",     "solanum_lycopersicum" },
	/* 10 */ {  45, 125,  60,  410,  1, 'x', 'X', 10, 2, 2, 1, 2, "quercus robur",            "quercus_robur" },
	/* 11 */ {  40, 122, 150,  600,  2, 'w', 'W',  2, 2, 2, 1, 2, "salix babylonica",         "salix_babylonica" },
	/* 12 */ {  38, 122,  70,  410,  1, 'x', 'X', 10, 2, 2, 1, 2, "acer pseudoplatanus",      "acer_pseudoplatanus" },
	/* 13 */ {  75, 148,  20,  230,  3, 'v', 'V',  3, 2, 2, 1, 2, "olea europaea",            "olea_europaea" },
	/* 14 */ {  30, 120,  40,  360,  4, 'f', 'F', 15, 2, 2, 0, 0, "bellis perennis",          "bellis_perennis" },
	/* 15 */ {  25, 125, 180,  600,  4, 'r', 'R',  6, 2, 2, 0, 0, "typha latifolia",          "typha_latifolia" },
};

static inline int
nd_plant_find_spec(const char *name)
{
	if (!name || !name[0])
		return -1;
	for (unsigned i = 0; i < ND_PLANT_MAX; i++) {
		if (!strcmp(s_nd_plant_specs[i].name, name) ||
		    !strcmp(s_nd_plant_specs[i].slug, name))
			return (int)i;
	}
	return -1;
}

static inline uint32_t
nd_plant_noise_eval(plant_tile_t *pd, uint32_t he, uint32_t w, uint8_t is_water,
                    uint32_t tm, int16_t tmp, uint16_t rn, const unsigned *refs)
{
	memset(pd, 0, sizeof(*pd));
	if (he < w && is_water)
		return 0;

	uint32_t ty = nd_xxh32_u32(tm ^ he, ND_PLANTS_SEED);
	unsigned cand_id[ND_PLANT_MAX];
	unsigned char cand_sz[ND_PLANT_MAX];
	uint32_t cand_prio[ND_PLANT_MAX];
	unsigned cand_n = 0;

	for (unsigned i = 0; i < ND_PLANT_MAX; i++) {
		const nd_plant_spec_t *pl = &s_nd_plant_specs[i];
		if (tmp <= pl->tmp_min || tmp >= pl->tmp_max ||
		    rn <= pl->rn_min || rn >= pl->rn_max)
			continue;

		uint32_t sv = nd_xxh32_u32(ty, i + 1u);
		if (sv >= (0xFFFFFFFFu >> pl->y))
			continue;

		uint32_t pv = (sv >> 5) & ND_PLANT_MASK;
		unsigned char cpln = 0;
		if (pl->big_col > 0) {
			/* Trees: 2/3 mature canopy (cpln=2), 1/3 sapling (cpln=1) when pv != 0 */
			if (pv == 3u)
				cpln = 2u;
			else
				cpln = (unsigned char)pv;
		} else {
			if (pv != 3u)
				cpln = (unsigned char)pv;
		}
		if (!cpln)
			continue;

		uint32_t prio = nd_xxh32_u32(sv, tm ^ (i * 0x9E3779B9u));
		/* Favor canopy trees in forest slots while keeping understory diversity */
		if (pl->big_col > 0)
			prio |= 0x80000000u;

		cand_id[cand_n] = refs ? refs[i] : i;
		cand_sz[cand_n] = cpln;
		cand_prio[cand_n] = prio;
		cand_n++;
	}

	/* Select up to 3 highest-priority matching species for the tile's 3 slots */
	unsigned take = (cand_n < 3u) ? cand_n : 3u;
	unsigned char apln[3] = { 0, 0, 0 };
	for (unsigned s = 0; s < take; s++) {
		unsigned best = s;
		for (unsigned j = s + 1u; j < cand_n; j++) {
			if (cand_prio[j] > cand_prio[best])
				best = j;
		}
		if (best != s) {
			unsigned tid = cand_id[s]; cand_id[s] = cand_id[best]; cand_id[best] = tid;
			unsigned char tsz = cand_sz[s]; cand_sz[s] = cand_sz[best]; cand_sz[best] = tsz;
			uint32_t tpr = cand_prio[s]; cand_prio[s] = cand_prio[best]; cand_prio[best] = tpr;
		}
		pd->id[s] = cand_id[s];
		apln[s] = (unsigned char)(cand_sz[s] & 3u);
	}
	if (take < 3u)
		pd->max = (unsigned char)take;

	uint32_t shuf_v = ~(ty >> 8);
	for (unsigned char i = 1; i < 3; i++) {
		if (shuf_v & i)
			continue;
		unsigned aux = pd->id[i - 1];
		pd->id[i - 1] = pd->id[i];
		pd->id[i] = aux;
		unsigned char aux_n = apln[i - 1];
		apln[i - 1] = apln[i];
		apln[i] = aux_n;
	}
	pd->n = (unsigned char)(apln[0] | (apln[1] << 2) | (apln[2] << 4));
	return ty;
}

static inline int
nd_plant_slot_glyph(const plant_tile_t *pd, unsigned side, uint8_t *out_glyph, uint8_t *out_fg)
{
	if (!pd || side >= 3)
		return 0;
	unsigned char sz = (unsigned char)ND_PLANT_N(pd->n, side);
	if (!sz || pd->id[side] >= ND_PLANT_MAX)
		return 0;
	const nd_plant_spec_t *pl = &s_nd_plant_specs[pd->id[side]];
	if (out_glyph) *out_glyph = (uint8_t)(sz > ND_PLANT_HALF ? pl->big : pl->small);
	if (out_fg)    *out_fg    = pl->fg;
	return 1;
}

static inline int
nd_plant_slot_meta(const plant_tile_t *pd, unsigned side, int32_t rx, int32_t ry,
                   int spec_idx_override, uint8_t *out_plant_id, uint8_t *out_col_mask)
{
	if (!pd || side >= 3)
		return 0;
	unsigned char sz = (unsigned char)ND_PLANT_N(pd->n, side);
	if (!sz)
		return 0;
	int idx = (spec_idx_override >= 0) ? spec_idx_override :
	          (pd->id[side] < ND_PLANT_MAX ? (int)pd->id[side] : -1);
	if (idx < 0 || idx >= (int)ND_PLANT_MAX)
		return 0;
	const nd_plant_spec_t *sp = &s_nd_plant_specs[idx];
	unsigned is_big = (sz > ND_PLANT_HALF) ? 1u : 0u;
	unsigned max_v = is_big ? sp->big_art : sp->small_art;
	uint32_t vhash = nd_xxh32_u32((uint32_t)(rx * 73856093 ^ ry * 19349663 ^ (int32_t)side), ND_PLANTS_SEED);
	unsigned var0 = (max_v > 1u) ? (vhash % max_v) : 0u;
	if (out_plant_id)
		*out_plant_id = ND_PLANT_PACK_ID((unsigned)idx, is_big, var0);
	if (out_col_mask)
		*out_col_mask = is_big ? sp->big_col : sp->small_col;
	return 1;
}

#endif /* !ND_PLANT_H */