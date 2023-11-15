#ifndef _FCEU_PALETTE_H
#define _FCEU_PALETTE_H

#define PALETTE_ARRAY_SIZE 0x200 /* rgb palette size */

#define PAL_NES_DEFAULT    0
#define PAL_RP2C04_0001    1
#define PAL_RP2C04_0002    2
#define PAL_RP2C04_0003    3
#define PAL_RP2C04_0004    4
#define PAL_RP2C03         5

typedef struct {
	uint8 r, g, b;
} pal;

extern pal palo[PALETTE_ARRAY_SIZE];

#endif
