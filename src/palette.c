/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2002,2003 Xodnizel
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <stdlib.h>
#include <math.h>

#include <string/stdstring.h>
#include <file/file_path.h>
#include <streams/file_stream.h>

#include "fceu-types.h"
#include "fceu-memory.h"
#include "fceu.h"
#include "general.h"
#include "driver.h"

#include "palette.h"
#include "palettes/palettes.h"

static void WritePalette(void);

pal palo[PALETTE_ARRAY_SIZE]; /* global pointer for current active palette */


static const double rtmul[8] = { 1.000, 1.239, 0.794, 1.019, 0.905, 1.023, 0.741, 0.75 };
static const double gtmul[8] = { 1.000, 0.915, 1.086, 0.980, 1.026, 0.908, 0.987, 0.75 };
static const double btmul[8] = { 1.000, 0.743, 0.882, 0.653, 1.277, 0.979, 0.101, 0.75 };

static void WritePalette(void) {
	int x;
	int unvaried = sizeof(unvpalette) / sizeof(unvpalette[0]);

	for (x = 0; x < unvaried; x++)
		FCEUD_SetPalette(x, unvpalette[x].r, unvpalette[x].g, unvpalette[x].b);
	for (x = unvaried; x < 256; x++)
		FCEUD_SetPalette(x, 205, 205, 205);
	for (x = 0; x < 64; x++)
		FCEUD_SetPalette(128 + x, palo[x].r, palo[x].g, palo[x].b);
	SetNESDeemph_OldHacky(0, TRUE);
	for (x = 0; x < PALETTE_ARRAY_SIZE; x++) {
		FCEUD_SetPalette(256 + x, palo[x].r, palo[x].g, palo[x].b);
	}
}

static void GenerateEmphasis(const pal palette[64]) {
	int i, j;

	for (i = 0; i < 64; i++) {
		for (j = 1; j < 8; j++) {
			uint32 r = (uint32)((double)palette[i].r * rtmul[j]);
			uint32 g = (uint32)((double)palette[i].g * gtmul[j]);
			uint32 b = (uint32)((double)palette[i].b * btmul[j]);
			palo[(j << 6) | i].r = (uint8_t)(r > 255 ? 255 : r);
			palo[(j << 6) | i].g = (uint8_t)(g > 255 ? 255 : g);
			palo[(j << 6) | i].b = (uint8_t)(b > 255 ? 255 : b);
		}
	}
}

void FCEU_SetPaletteUser(const pal *palette, const unsigned nEntries) {
	int x;

	for (x = 0; x < nEntries; x++) {
		palo[x].r = palette[x].r;
		palo[x].g = palette[x].g;
		palo[x].b = palette[x].b;
	}

	if (nEntries == 64) {
		GenerateEmphasis(palette);
	}

	WritePalette();
}

void FCEU_SetPalette(void) {
	int which = (GameInfo && (GameInfo->type == GIT_VSUNI)) ? FCEU_VSUniGetPaletteNum() : PAL_NES_DEFAULT;
	switch (which) {
	case PAL_RP2C04_0001: FCEU_SetPaletteUser(rp2c04_0001, 64); break;
	case PAL_RP2C04_0002: FCEU_SetPaletteUser(rp2c04_0002, 64); break;
	case PAL_RP2C04_0003: FCEU_SetPaletteUser(rp2c04_0003, 64); break;
	case PAL_RP2C04_0004: FCEU_SetPaletteUser(rp2c04_0004, 64); break;
	case PAL_RP2C03:      FCEU_SetPaletteUser(     rp2c03, 64); break;
	default:              FCEU_SetPaletteUser(    palette, 64); break;
	}
}

static uint8 lastd = 0;
void SetNESDeemph_OldHacky(uint8 d, int force) {
	double r, g, b;
	int x;

	/* If it's not forced(only forced when the palette changes),
	don't waste cpu time if the same deemphasis bits are set as the last call. */
	if (!force) {
		if (d == lastd)
			return;
	} else {	/* Only set this when palette has changed. */
		r = rtmul[7];
		g = gtmul[7];
		b = btmul[7];

		for (x = 0; x < 0x40; x++) {
			uint8 m = (uint8)((double)palo[x].r * r);
			uint8 n = (uint8)((double)palo[x].g * g);
			uint8 o = (uint8)((double)palo[x].b * b);
			FCEUD_SetPalette(x | 0xC0, m, n, o);
		}
	}

	if (!d) return;	/* No deemphasis, so return. */

	r = rtmul[d];
	g = gtmul[d];
	b = btmul[d];

	for (x = 0; x < 0x40; x++) {
		uint8 m = (uint8)((double)palo[x].r * r);
		uint8 n = (uint8)((double)palo[x].g * g);
		uint8 o = (uint8)((double)palo[x].b * b);
		FCEUD_SetPalette(x | 0x40, m, n, o);
	}

	lastd = d;
}
