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

typedef struct {
	unsigned id[3];
	unsigned char n, max;
} plant_tile_t;

#endif /* !ND_PLANT_H */