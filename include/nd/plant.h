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

typedef struct {
	int16_t  tmp_min, tmp_max;
	uint16_t rn_min, rn_max;
	uint8_t  y;
	char     small, big;
	uint8_t  fg;
} nd_plant_spec_t;

/* Canonical 16-species envelope table matching external/nd-plant/data/plants.dsv */
static const nd_plant_spec_t s_nd_plant_specs[ND_PLANT_MAX] = {
	/* 0  pinus sylvestris         */ {  20,  80,  50, 1024,  4, 'x', 'X', 10 },
	/* 1  pseudotsuga menziesii    */ {  25,  95, 180,  350,  1, 't', 'T', 10 },
	/* 2  betula pendula           */ {  30,  90,   0,  341,  4, 'x', 'X',  3 },
	/* 3  linum usitatissimum      */ {  30,  90,  20,  341, 30, 'x', 'X',  3 },
	/* 4  betula pubescens         */ {  40, 130, 400,  900,  4, 'x', 'X',  7 },
	/* 5  abies alba               */ { -40,  75,  80,  300,  4, 'a', 'A', 10 },
	/* 6  arthrocereus rondonianus */ { 100, 190,  10,  180,  4, 'i', 'I', 10 },
	/* 7  acacia senegal           */ {  60, 160,  20,  345,  4, 't', 'T', 11 },
	/* 8  daucus carota            */ {  38, 105,  80,  250,  4, 'x', 'X',  7 },
	/* 9  solanum lycopersicum     */ {  50, 115,  80,  250,  4, 'x', 'X',  1 },
	/* 10 quercus robur            */ {  45, 125,  60,  400,  4, 'x', 'X', 10 },
	/* 11 salix babylonica         */ {  40, 120, 140,  600,  4, 'w', 'W',  2 },
	/* 12 acer pseudoplatanus      */ {  35, 110,  70,  380,  4, 'x', 'X', 10 },
	/* 13 olea europaea            */ {  70, 145,  20,  220,  4, 'v', 'V',  3 },
	/* 14 bellis perennis          */ {  30, 120,  40,  350,  4, 'f', 'F', 15 },
	/* 15 typha latifolia          */ {  25, 125, 160,  600,  4, 'r', 'R',  6 },
};

static inline uint32_t
nd_plant_noise_eval(plant_tile_t *pd, uint32_t he, uint32_t w, uint8_t is_water,
                    uint32_t tm, int16_t tmp, uint16_t rn, const unsigned *refs)
{
	memset(pd, 0, sizeof(*pd));
	if (he < w && is_water)
		return 0;

	uint32_t ty = nd_xxh32_u32(tm, ND_PLANTS_SEED);
	uint32_t v = ty;
	unsigned *idc = pd->id;
	unsigned pdn = 0;

	for (unsigned i = 0; i < ND_PLANT_MAX; i++) {
		if (idc >= &pd->id[3])
			break;
		if (!v)
			v = nd_xxh32_u32(ty, i + 1u);

		const nd_plant_spec_t *pl = &s_nd_plant_specs[i];
		unsigned char cpln = 0;
		if (v < (0xFFFFFFFFu >> pl->y) &&
		    tmp < pl->tmp_max && tmp > pl->tmp_min &&
		    rn < pl->rn_max && rn > pl->rn_min) {
			uint32_t pv = (v >> 1) & ND_PLANT_MASK;
			if (pv != 3u)
				cpln = (unsigned char)pv;
		}
		if (cpln) {
			*idc++ = refs ? refs[i] : i;
			pdn = (pdn << 2) | (cpln & 3u);
		}
		v >>= 8;
	}
	if (idc < &pd->id[3])
		pd->max = (unsigned char)*idc;
	pd->n = (unsigned char)pdn;

	unsigned char apln[3] = {
		(unsigned char)(pd->n & 3u),
		(unsigned char)((pd->n >> 2) & 3u),
		(unsigned char)((pd->n >> 4) & 3u),
	};
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

#endif /* !ND_PLANT_H */