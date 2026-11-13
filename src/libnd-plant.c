/* src/libnd-plant.c — nd-plant, ported to libxylem.
 *
 * Owns plants: the ten tree/crop species, their skeletons and drops, map
 * generation (noise, spawn, shuffle), and the carrot/tomato consumables that
 * feed nd-drink.
 *
 * Original: tty-pt/nd-plant @ 373 lines main.c, from the nd-basics
 * superproject.
 *
 * This TU XY_IMPLs on_spawn, on_examine, on_add, on_noise and on_empty_tile.
 * It implements no named service from another module's header, so it needs no
 * *_IMPL guard; plant_tile_t comes from its own nd/plant.h (pure type, no
 * XY_DECL) and consumable_skel_t from nd/drink.h.
 *
 * on_icon does not exist here anymore. nd-plant amends icons through nd-core's
 * decorator table (core_icon_decorate), because XY cannot observe a
 * co-implemented chain; see <nd/core.h> and MODS.md §7.
 *
 * XXH32 for the noise hash. The old module got this from <nd/nd.h>, which is
 * gone; nothing in nd/ exposes it. <xxhash.h> is self-contained for XXH32 (no
 * -lxxhash needed), and the call is unchanged -- the same arrangement
 * nd-stone uses.
 */

#include <ttypt/xy-mod.h>

#include <nd/xy.h>

#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <ttypt/corm.h>

#include <xxhash.h>

#include <nd/plant.h>
#include <nd/drink.h>
#include <nd/core.h>

/* Mirrors CORE_ET_* in nd-view's type.h (the installed nd/core.h predates it). */
#ifndef CORE_ET_ASCII
#define CORE_ET_ASCII 0x80u
#define CORE_ET_SIDE_MASK 0x03u
#endif

#define PLANT_EXTRA 4
#define PLANT_MASK 0x3
#define PLANT_HALF (PLANT_MASK >> 1)
#define PLANTS_SEED 5
#define PLANT_N(pln, i) ((pln >> (i * 2)) & 3)

// skel
typedef struct plant_skel {
	struct print_info pi;
	char const small, big;
	int16_t tmp_min, tmp_max;
	uint16_t rn_min, rn_max;
	unsigned y;
	uint8_t small_art, big_art;
	uint8_t small_col, big_col;
	uint8_t spec_idx;
} SPLA;

_Static_assert(sizeof(SPLA) <= sizeof(((SKEL *)0)->data), "SPLA must fit in SKEL.data");

// instance
typedef struct {
	unsigned plid;
	unsigned size;
} PLA;

enum base_plant {
	PLANT_PINUS_SILVESTRIS,
	PLANT_PSEUDOTSUGA_MENZIESII,
	PLANT_BETULA_PENDULA,
	PLANT_LINUM_USITATISSIMUM,
	PLANT_BETULA_PUBESCENS,
	PLANT_ABIES_ALBA,
	PLANT_ARTHROCEREUS_RONDONIANUS,
	PLANT_ACACIA_SENEGAL,
	PLANT_DAUCUS_CAROTA,
	PLANT_SOLANUM_LYCOPERSICUM,
	PLANT_QUERCUS_ROBUR,
	PLANT_SALIX_BABYLONICA,
	PLANT_ACER_PSEUDOPLATANUS,
	PLANT_OLEA_EUROPAEA,
	PLANT_BELLIS_PERENNIS,
	PLANT_TYPHA_LATIFOLIA,
	PLANT_MAX,
};

static unsigned type_plant, act_chop;
static unsigned plant_refs[PLANT_MAX];
static unsigned gen_plant_n = 0;

static SKEL carrot = {
        .name = "carrot",
        .type = 0, // type later
};

static consumable_skel_t carrot_con = { .food = 3 };

static DROP carrot_drop = {
        .y = 0,
};

static SKEL stick = {
        .name = "stick",
	.max_art = 14,
};

static DROP stick_drop = {
        .y = 0,
        .yield = 1,
        .yield_v = 0x3,
};

static unsigned stick_skel_id = NOTHING;

#define STICK_HARVEST_MAX 1024
#define STICK_RESPAWN_COOLDOWN 180 /* 3 minutes */

typedef struct {
	unsigned room;
	uint32_t harvested_at;
} stick_harvest_entry_t;

static stick_harvest_entry_t s_stick_harvest[STICK_HARVEST_MAX];
static size_t s_stick_harvest_n = 0;

static SKEL tomato = {
        .name = "tomato",
        .type = 0, // type later
};

static consumable_skel_t tomato_con = { .food = 4 };

static DROP tomato_drop = {
        .y = 3,
};

/* API. XY_IMPL both defines the function and emits the dispatch adapter, so
 * each name gets exactly one, with its body -- no forward declarations.
 *
 * Order matters below: XY_IMPL emits a definition, so a caller has to come
 * after its callee. plant_skel_add leads because xy_install calls it. */

static inline unsigned plant_skel_add(
		const char *name, unsigned max_art, unsigned fg, unsigned pi_flags,
		char small, char big, 
		int16_t tmp_min, int16_t tmp_max,
		uint16_t rn_min, uint16_t rn_max,
		unsigned y, unsigned drop,
		uint8_t small_art, uint8_t big_art,
		uint8_t small_col, uint8_t big_col)
{
	SKEL skel = {
		.max_art = max_art,
		.type = type_plant,
	};
	int spec_idx = nd_plant_find_spec(name);
	SPLA spla = {
		.pi = { .fg = fg, .flags = pi_flags, },
		.small = small, .big = big,
		.tmp_min = tmp_min, .tmp_max = tmp_max,
		.rn_min = rn_min, .rn_max = rn_max,
		.y = y,
		.small_art = small_art ? small_art : 2,
		.big_art = big_art ? big_art : 2,
		.small_col = small_col,
		.big_col = big_col,
		.spec_idx = (uint8_t)(spec_idx >= 0 ? spec_idx : (int)gen_plant_n),
	};
	unsigned id;

	memcpy((char *) skel.name, name, sizeof(skel.name));
	memcpy(skel.data, &spla, sizeof(spla));

	id = (unsigned)nd_put(HD_SKEL, NULL, &skel);
	if (drop != NOTHING)
		nd_put(HD_ADROP, &id, &drop);

	plant_refs[gen_plant_n] = id;
	gen_plant_n ++;
	return id;
}

/* TODO calculate water needs */

static inline void
_plants_add(unsigned where_ref, struct bio *bio, uint64_t v)
{
	register int i, n;
	plant_tile_t pd = * (plant_tile_t *) bio->raw;

        for (i = 0; i < 3; i++, v >>= 4) {
                n = PLANT_N(pd.n, i);

		if (!n)
			continue;

		OBJ plant;
		PLA *pplant = (PLA *) &plant.data;
		pplant->plid = pd.id[i];
		pplant->size = n;
		uint64_t pv = (v & 0xffu) | ((uint64_t)(n & 3u) << 8) | ((uint64_t)pd.id[i] << 16);
		unsigned ref = object_add(&plant, pd.id[i], where_ref, pv);
		if (ref != NOTHING) {
			OBJ saved;
			if (nd_get(HD_OBJ, &saved, &ref) == 0) {
				PLA *spla = (PLA *) &saved.data;
				spla->plid = pd.id[i];
				spla->size = n;
				nd_put(HD_OBJ, &ref, &saved);
			}
			/* Sticks are placed inside the tree container by on_add() */
		}
        }
}

XY_IMPL(int, on_spawn, unsigned, player_ref,
		unsigned, where_ref, struct bio, bio,
		uint64_t, v)
{
	(void) player_ref;
	(void) v;
	plant_tile_t pd = * (plant_tile_t *) bio.raw;
	/* &bio->pd, bio->ty, */
	/* bio->tmp, bio->rn); */
	/* plant_tile_t epd; */

        if (pd.n)
                _plants_add(where_ref, &bio, v);

	/* plants_noise(&epd, ty, tmp, rn, PLANT_EXTRA); */

        /* if (epd.n) */
                /* _plants_add(where, &epd, tmp); */
	return 0;
}

XY_IMPL(int, on_examine, unsigned, player_ref, unsigned, ref, unsigned, type)
{
	OBJ obj;

	if (type != type_plant)
		return 1;

	nd_get(HD_OBJ, &obj, &ref);
	PLA *pthing = (PLA *) &obj.data;
	nd_printf(player_ref, "plant plid %u size %u.\n", pthing->plid, pthing->size);
	return 0;
}

XY_IMPL(int, on_add, unsigned, ref, unsigned, type, uint64_t, v)
{
	OBJ obj;
	SKEL skel;

	if (type != type_plant)
		return 1;

	nd_get(HD_OBJ, &obj, &ref);
	nd_get(HD_SKEL, &skel, &obj.skid);

	PLA *pla = (PLA *) &obj.data;
	SPLA *spla = (SPLA *) &skel.data;
	if (pla->size == 0 && ((v >> 8) & 3u) != 0) {
		pla->size = (unsigned)((v >> 8) & 3u);
		pla->plid = (unsigned)(v >> 16);
	}

	object_drop(ref, obj.skid);
	unsigned stage_max = (pla->size > PLANT_HALF) ? spla->big_art : spla->small_art;
	if (!stage_max)
		stage_max = 2;
	obj.art_id = 1 + (unsigned)((v & 0xf) % stage_max);

	nd_put(HD_OBJ, &ref, &obj);
	return 0;
}

static int
plant_art_decorate(unsigned ref, unsigned type, char *buf, size_t len)
{
	OBJ obj;
	SKEL skel;

	if (type != type_plant || !buf || !len)
		return 0;
	if (nd_get(HD_OBJ, &obj, &ref) != 0)
		return 0;
	if (nd_get(HD_SKEL, &skel, &obj.skid) != 0)
		return 0;

	PLA *pla = (PLA *) &obj.data;
	SPLA *spla = (SPLA *) &skel.data;
	const char *stage = (pla->size > PLANT_HALF) ? "big" : "small";
	unsigned stage_max = (pla->size > PLANT_HALF) ? spla->big_art : spla->small_art;
	if (!stage_max)
		stage_max = 2;
	unsigned art_id = obj.art_id ? ((obj.art_id - 1u) % stage_max) + 1u : 1u;

	char slug[32];
	size_t i = 0;
	for (; i < sizeof(slug) - 1 && skel.name[i]; i++)
		slug[i] = (skel.name[i] == ' ') ? '_' : skel.name[i];
	slug[i] = '\0';

	snprintf(buf, len, "plant/%s/%s/%u.png", slug, stage, art_id);
	return 1;
}

/* The on_icon co-implementation is now a decorator. nd-core owns on_icon and
 * runs the registered decorators in order; this one replaces the glyph with
 * the plant's growth stage and adds the chop action. Registered in xy_install
 * below. */
static struct icon
plant_icon_decorate(struct icon i, unsigned ref, unsigned type,
	unsigned player_ref)
{
	OBJ obj;
	SKEL skel;
	PLA *pla = (PLA *) &obj.data;
	SPLA *spla = (SPLA *) &skel.data;

	(void) player_ref;
	if (type != type_plant)
		return i;

	nd_get(HD_OBJ, &obj, &ref);
	nd_get(HD_SKEL, &skel, &obj.skid);

	i.actions &= ~ACT_GET;
	i.actions |= act_chop;
	i.pi = spla->pi;
	i.ch = pla->size > PLANT_HALF ? spla->big : spla->small;

	return i;
}

static struct bio
plant_noise_decorate(struct bio r, uint32_t he, uint32_t w, uint32_t tm, uint32_t cl)
{
	plant_tile_t pd;
	(void) cl;
	r.ty = nd_plant_noise_eval(&pd, he, w, r.is_water, tm, r.tmp, r.rn, plant_refs);
	memcpy(r.raw, &pd, sizeof(pd));
	return r;
}

static sic_str_t
plant_empty_tile_decorate(view_tile_t t, unsigned side, sic_str_t ss)
{
	char *b = ss.str;
	plant_tile_t pd = * (plant_tile_t *) t.raw;

	side &= CORE_ET_SIDE_MASK;

	if (PLANT_N(pd.n, side)) {
		SKEL skel;
		nd_get(HD_SKEL, &skel, &pd.id[side]);
		SPLA *pl = (SPLA *) &skel.data;

		if (pl->pi.flags & BOLD)
			b = stpcpy(b, ANSI_BOLD);
		b = stpcpy(b, ansi_fg[pl->pi.fg]);
		*b++ = PLANT_N(pd.n, side) > PLANT_HALF
			? pl->big : pl->small;
		b = stpcpy(b, ansi_fg[pl->pi.fg]);
		if (pl->pi.flags & BOLD)
			b = stpcpy(b, ANSI_RESET_BOLD);
		*b = '\0';

		if (t.room != NOTHING && t.room != 0 && stick_skel_id != NOTHING) {
			unsigned c_hd = nd_table_handle(HD_CONTENTS);
			if (c_hd) {
				unsigned c = corm_iter(c_hd, &t.room, CM_RANGE);
				const void *kp, *vp;
				int has_stick = 0;
				unsigned tree_in_room = NOTHING;
				while (corm_next(&kp, &vp, c)) {
					unsigned thing_ref = *(const unsigned *)vp;
					OBJ obj;
					if (nd_get(HD_OBJ, &obj, &thing_ref) == 0) {
						if (obj.type == type_plant || obj.skid == pd.id[side]) {
							if (tree_in_room == NOTHING)
								tree_in_room = thing_ref;
							unsigned tc = corm_iter(c_hd, &thing_ref, CM_RANGE);
							const void *tkp, *tvp;
							while (corm_next(&tkp, &tvp, tc)) {
								unsigned sub_ref = *(const unsigned *)tvp;
								OBJ sub_obj;
								if (nd_get(HD_OBJ, &sub_obj, &sub_ref) == 0) {
									if (sub_obj.skid == stick_skel_id || !strcmp(sub_obj.name, "stick")) {
										has_stick = 1;
										break;
									}
								}
							}
							corm_fin(tc);
							if (has_stick)
								break;
						} else if (obj.skid == stick_skel_id || !strcmp(obj.name, "stick")) {
							has_stick = 1;
							break;
						}
					}
				}
				corm_fin(c);
				if (!has_stick) {
					uint32_t now = (uint32_t)time(NULL);
					int found = -1;
					for (size_t i = 0; i < s_stick_harvest_n; i++) {
						if (s_stick_harvest[i].room == t.room) {
							found = (int)i;
							break;
						}
					}
					if (found < 0) {
						if (s_stick_harvest_n < STICK_HARVEST_MAX) {
							s_stick_harvest[s_stick_harvest_n].room = t.room;
							s_stick_harvest[s_stick_harvest_n].harvested_at = now;
							s_stick_harvest_n++;
						}
					} else if (now - s_stick_harvest[found].harvested_at >= STICK_RESPAWN_COOLDOWN) {
						/* Replenish sticks inside tree container (replenishes even in claimed rooms) */
						unsigned target = (tree_in_room != NOTHING) ? tree_in_room : t.room;
						object_drop(target, pd.id[side]);
						s_stick_harvest[found] = s_stick_harvest[--s_stick_harvest_n];
					}
				}
			}
		}
	} else {
		*b++ = ' ';
		*b = '\0';
	}
	return ss;
}

struct plant_drop_ctx {
	unsigned stick;
	unsigned carrot;
	unsigned tomato;
};

static int
plant_row(const char *const *f, size_t n, void *user)
{
	static const struct { const char *n; unsigned v; } colors[] = {
		{ "black", BLACK }, { "red", RED }, { "yellow", YELLOW },
		{ "green", GREEN }, { "cyan", CYAN }, { "blue", BLUE },
		{ "magenta", MAGENTA }, { "white", WHITE },
	};
	const struct plant_drop_ctx *drops = (const struct plant_drop_ctx *)user;
	if (n < 12)
		return 0;

	const char *name = f[0];
	unsigned max_art = (unsigned)atoi(f[1]);
	unsigned fg = GREEN;
	for (size_t i = 0; i < sizeof(colors) / sizeof(colors[0]); i++)
		if (!strcmp(colors[i].n, f[2]))
			fg = colors[i].v;

	unsigned pi_flags = atoi(f[3]) ? BOLD : 0;
	char small = f[4][0];
	char big = f[5][0];
	int16_t tmp_min = (int16_t)atoi(f[6]);
	int16_t tmp_max = (int16_t)atoi(f[7]);
	uint16_t rn_min = (uint16_t)atoi(f[8]);
	uint16_t rn_max = (uint16_t)atoi(f[9]);
	unsigned y = (unsigned)atoi(f[10]);

	unsigned drop = NOTHING;
	if (!strcmp(f[11], "stick"))
		drop = drops->stick;
	else if (!strcmp(f[11], "carrot"))
		drop = drops->carrot;
	else if (!strcmp(f[11], "tomato"))
		drop = drops->tomato;

	int spec_idx = nd_plant_find_spec(name);
	uint8_t small_art = (n > 12) ? (uint8_t)atoi(f[12]) : (spec_idx >= 0 ? s_nd_plant_specs[spec_idx].small_art : 2);
	uint8_t big_art   = (n > 13) ? (uint8_t)atoi(f[13]) : (spec_idx >= 0 ? s_nd_plant_specs[spec_idx].big_art   : 2);
	uint8_t small_col = (n > 14) ? (uint8_t)atoi(f[14]) : (spec_idx >= 0 ? s_nd_plant_specs[spec_idx].small_col : 1);
	uint8_t big_col   = (n > 15) ? (uint8_t)atoi(f[15]) : (spec_idx >= 0 ? s_nd_plant_specs[spec_idx].big_col   : 1);

	plant_skel_add(name, max_art, fg, pi_flags, small, big,
			tmp_min, tmp_max, rn_min, rn_max, y, drop,
			small_art, big_art, small_col, big_col);
	return 0;
}

XY_MODULE_API void
xy_install(void)
{
	/* Order matches the original: the chop action first, then the type,
	 * consumables, drops and species. */
	act_chop = (unsigned)action_register("chop", "🪓");

	unsigned consumable_type, other_type;

	type_plant = (unsigned)nd_put(HD_TYPE, NULL, "plant");
	memcpy(&carrot.data, &carrot_con, sizeof(carrot_con));
	memcpy(&tomato.data, &tomato_con, sizeof(tomato_con));

	nd_get(HD_RTYPE, &consumable_type, "consumable");
	tomato.type = carrot.type = consumable_type;

	carrot_drop.skel = (unsigned)nd_put(HD_SKEL, NULL, &carrot);
	unsigned carrot_drop_ref = (unsigned)nd_put(HD_DROP, NULL, &carrot_drop);

	tomato_drop.skel = (unsigned)nd_put(HD_SKEL, NULL, &tomato);
	unsigned tomato_drop_ref = (unsigned)nd_put(HD_DROP, NULL, &tomato_drop);

	unsigned stick_drop_ref = NOTHING;
	if (!nd_get(HD_RTYPE, &other_type, "other")) {
		stick.type = other_type;
		stick.max_art = 14;
		stick_drop.skel = (unsigned)nd_put(HD_SKEL, NULL, &stick);
		stick_drop_ref = (unsigned)nd_put(HD_DROP, NULL, &stick_drop);
		stick_skel_id = stick_drop.skel;
	}

	struct plant_drop_ctx drops = {
		.stick = stick_drop_ref,
		.carrot = carrot_drop_ref,
		.tomato = tomato_drop_ref,
	};
	if (nd_table_load("nd-plant", "plants", plant_row, &drops) < 0)
		WARN("nd-plant: plants.dsv not found, no plants loaded\n");

	core_noise_decorate(plant_noise_decorate);
	core_empty_tile_decorate(plant_empty_tile_decorate);
	core_icon_decorate(plant_icon_decorate);
	core_art_decorate(plant_art_decorate);
}