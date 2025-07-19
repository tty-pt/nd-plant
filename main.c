#include <string.h>

#include <nd/nd.h>
#include <nd/drink.h>

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
} SPLA;

// instance
typedef struct {
	unsigned plid;
	unsigned size;
} PLA;

struct plant_data {
	unsigned id[3];
	unsigned char n, max;
};

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
	PLANT_MAX,
};

unsigned type_plant, act_chop;
unsigned plant_refs[PLANT_MAX];
unsigned gen_plant_n = 0;

SKEL carrot = {
        .name = "carrot",
        .type = 0, // type later
};

consumable_skel_t carrot_con = { .food = 3 };

DROP carrot_drop = {
        .y = 0,
};

SKEL stick = {
        .name = "stick",
	.max_art = 14,
};

DROP stick_drop = {
        .y = 0,
        .yield = 1,
        .yield_v = 0x3,
};

SKEL tomato = {
        .name = "tomato",
        .type = 0, // type later
};

consumable_skel_t tomato_con = { .food = 4 };

DROP tomato_drop = {
        .y = 3,
};

static inline unsigned plant_skel_add(
		const char *name, unsigned max_art, unsigned fg, unsigned pi_flags,
		char small, char big, 
		int16_t tmp_min, int16_t tmp_max,
		uint16_t rn_min, uint16_t rn_max,
		unsigned y, unsigned drop)
{
	SKEL skel = {
		.max_art = max_art,
		.type = type_plant,
	};
	SPLA spla = {
		.pi = { .fg = fg, .flags = pi_flags, },
		.small = small, .big = big,
		.tmp_min = tmp_min, .tmp_max = tmp_max,
		.rn_min = rn_min, .rn_max = rn_max,
		.y = y,
	};
	unsigned id;

	memcpy((char *) skel.name, name, sizeof(skel.name));
	memcpy(skel.data, &spla, sizeof(spla));

	id = nd_put(HD_SKEL, NULL, &skel);
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
	struct plant_data pd = * (struct plant_data *) bio->raw;

        for (i = 0; i < 3; i++, v >>= 4) {
                n = PLANT_N(pd.n, i);

		if (!n)
			continue;

		OBJ plant;
		PLA *pplant = (PLA *) &plant.data;
		pplant->plid = pd.id[i];
		pplant->size = n;
		object_add(&plant, pd.id[i], where_ref, v);
        }
}

int
on_spawn(unsigned player_ref __attribute__((unused)),
		unsigned where_ref, struct bio bio,
		uint64_t v __attribute__((unused)))
{
	struct plant_data pd = * (struct plant_data *) bio.raw;
	/* &bio->pd, bio->ty, */
	/* bio->tmp, bio->rn); */
	/* struct plant_data epd; */

        if (pd.n)
                _plants_add(where_ref, &bio, v);

	/* plants_noise(&epd, ty, tmp, rn, PLANT_EXTRA); */

        /* if (epd.n) */
                /* _plants_add(where, &epd, tmp); */
	return 0;
}

int on_examine(unsigned player_ref, unsigned ref, unsigned type) {
	OBJ obj;

	if (type != type_plant)
		return 1;

	nd_get(HD_OBJ, &obj, &ref);
	PLA *pthing = (PLA *) &obj.data;
	nd_writef(player_ref, "plant plid %u size %u.\n", pthing->plid, pthing->size);
	return 0;
}

int on_add(unsigned ref, unsigned type, uint64_t v)
{
	OBJ obj;
	SKEL skel;

	if (type != type_plant)
		return 1;

	nd_get(HD_OBJ, &obj, &ref);
	nd_get(HD_SKEL, &skel, &obj.skid);

	object_drop(ref, obj.skid);
	unsigned max = skel.max_art;
	obj.art_id = max ? 1 + (v & 0xf) % max : 0;

	nd_put(HD_OBJ, &ref, &obj);
	return 0;
}

struct icon on_icon(struct icon i, unsigned ref, unsigned type) {
	OBJ obj;
	SKEL skel;
	PLA *pla = (PLA *) &obj.data;
	SPLA *spla = (SPLA *) &skel.data;

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

static inline unsigned char
plant_noise(unsigned *plid, coord_t tmp, ucoord_t rn, uint32_t v, unsigned plant_ref)
{
	SKEL skel;
	nd_get(HD_SKEL, &skel, &plant_ref);
	SPLA *pl = (SPLA *) &skel.data;

        if (v >= (NOISE_MAX >> pl->y))
                return 0;
	/* if (((v >> 6) ^ (v >> 3) ^ v) & 1) */
	/* 	return 0; */

	if (tmp < pl->tmp_max && tmp > pl->tmp_min
	    && rn < pl->rn_max && rn > pl->rn_min) {
		*plid = plant_ref;
		v = (v >> 1) & PLANT_MASK;
		if (v == 3)
			v = 0;
                return v;
	}

	return 0;
}

static inline void
plants_noise(struct plant_data *pd, uint32_t ty, coord_t tmp, ucoord_t rn, unsigned n)
{
	uint32_t v = ty;
	register int cpln;
	unsigned *idc = pd->id;
	unsigned ref;
	unsigned pdn = 0;

	for (int i = 0; i < PLANT_MAX; i++) {
		if (idc >= &pd->id[n]) {
			break;
		}

		ref = plant_refs[i];

		if (!v)
			v = XXH32((const char *) &ty, sizeof(ty), ref);

		cpln = plant_noise(idc, tmp, rn, v, ref);

		if (cpln) {
			pdn = (pdn << 2) | (cpln & 3);
			idc++;
		}

		v >>= 8;
	}
	pd->max = *idc;
	pd->n = pdn;
}

static void
plants_shuffle(struct plant_data *pd, morton_t v)
{
        unsigned char apln[3] = {
                pd->n & 3,
                (pd->n >> 2) & 3,
                (pd->n >> 4) & 3,
        };
	register unsigned char i;
	unsigned aux, pdn = 0;

        for (i = 1; i <= 3; i++) {
		pdn = pdn << 2;

                if (v & i)
                        continue;

                aux = pd->id[i - 1];
                pd->id[i - 1] = pd->id[i];
                pd->id[i] = aux;

                aux = apln[i - 1];
                apln[i - 1] = apln[i];
                apln[i] = aux;
        }

	pd->n = apln[0] | (apln[1] << 2) | (apln[2] << 4);
}

struct bio on_noise(struct bio r, uint32_t he, uint32_t w, uint32_t tm, uint32_t cl __attribute__((unused))) {
	struct plant_data pd;
	pd.max = 0;
	if (he > w) {
		r.ty = XXH32(&tm, sizeof(uint32_t), PLANTS_SEED);
		plants_noise(&pd, r.ty, r.tmp, r.rn, 3);
		plants_shuffle(&pd, ~(r.ty >> 8));
	} else {
		memset(pd.id, 0, 3);
		pd.n = 0;
	}

	memcpy(r.raw, &pd, sizeof(pd));
	return r;
}

sic_str_t on_empty_tile(view_tile_t t, unsigned side, sic_str_t ss) {
	char *b = ss.str;
	struct plant_data pd = * (struct plant_data *) t.raw;

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
	} else
		*b++ = ' ';
	*b = '\0';
	return ss;
}

void mod_open(void *arg __attribute__((unused))) {
	type_plant = nd_put(HD_TYPE, NULL, "plant");

	act_chop = action_register("chop", "🪓");
}

void mod_install(void *arg) {
	unsigned consumable_type, other_type;

	mod_open(arg);

	memcpy(&carrot.data, &carrot_con, sizeof(carrot_con));
	memcpy(&tomato.data, &tomato_con, sizeof(tomato_con));

	nd_get(HD_RTYPE, &consumable_type, "consumable");
	tomato.type = carrot.type = consumable_type;

	carrot_drop.skel = nd_put(HD_SKEL, NULL, &carrot);
	unsigned carrot_drop_ref = nd_put(HD_DROP, NULL, &carrot_drop);

	tomato_drop.skel = nd_put(HD_SKEL, NULL, &tomato);
	unsigned tomato_drop_ref = nd_put(HD_DROP, NULL, &tomato_drop);

	nd_get(HD_RTYPE, &other_type, "other");
	stick.type = other_type;
	stick_drop.skel = nd_put(HD_SKEL, NULL, &stick);
	unsigned stick_drop_ref = nd_put(HD_DROP, NULL, &stick_drop);

	plant_skel_add("pinus sylvestris", 19, GREEN, BOLD, 'x', 'X', 30, 70, 50, 1024, 4, stick_drop_ref);
	plant_skel_add("pseudotsuga menziesii", 21, GREEN, BOLD, 't', 'T', 32, 100, 180, 350, 1, stick_drop_ref);
	plant_skel_add("betula pendula", 14, YELLOW, 0, 'x', 'X', 30, 86, 0, 341, 4, stick_drop_ref);
	plant_skel_add("linum usitatissimum", 7, YELLOW, 0, 'x', 'X', 30, 86, 20, 341, 30, stick_drop_ref);
	plant_skel_add("betula pubescens", 0, WHITE, 0, 'x', 'X', 50, 146, 500, 900, 4, stick_drop_ref);
	plant_skel_add("abies alba", 10, GREEN, BOLD, 'a', 'A', -40, 86, 100, 200, 4, stick_drop_ref);
	plant_skel_add("arthrocereus rondonianus", 9, GREEN, BOLD, 'i', 'I', 110, 190, 10, 180, 4, stick_drop_ref);
	plant_skel_add("acacia senegal", 12, GREEN, BOLD, 't', 'T', 40, 150, 20, 345, 4, stick_drop_ref);
	plant_skel_add("daucus carota", 6, WHITE, 0, 'x', 'X', 38, 96, 100, 200, 4, carrot_drop_ref);
	plant_skel_add("solanum lycopersicum", 11, RED, 0, 'x', 'X', 50, 98, 100, 200, 4, tomato_drop_ref);
}
