#ifndef PLANT_H
#define PLANT_H

#include <nd/azoth.h>
#include <stdint.h>

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

#endif
