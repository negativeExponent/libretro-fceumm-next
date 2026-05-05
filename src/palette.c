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

#include "fceu-types.h"
#include "fceu-memory.h"
#include "fceu.h"
#include "general.h"
#include "driver.h"

#include "palette.h"
#include "palettes/palettes.h"

static void WritePalette(void);

pal palo[PALETTE_ARRAY_SIZE]; /* global pointer for current active palette */

static const uint16_t rtmul[8] = { 32768 * 1.000, 32768 * 1.239, 32768 * 0.794, 32768 * 1.019, 32768 * 0.905, 32768 * 1.023, 32768 * 0.741, 32768 * 0.750 };
static const uint16_t gtmul[8] = { 32768 * 1.000, 32768 * 0.915, 32768 * 1.086, 32768 * 0.980, 32768 * 1.026, 32768 * 0.908, 32768 * 0.987, 32768 * 0.750 };
static const uint16_t btmul[8] = { 32768 * 1.000, 32768 * 0.743, 32768 * 0.882, 32768 * 0.653, 32768 * 1.277, 32768 * 0.979, 32768 * 0.101, 32768 * 0.750 };

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

static void GenerateEmphasis(pal out[512], const pal base[64]) {
	int i, j;

	for (i = 0; i < 64; i++) {
		for (j = 1; j < 8; j++) {
			double r = base[i].r;
			double g = base[i].g;
			double b = base[i].b;
#if 1
			if ((i & 0x0F) <= 0x0D) {
				if (j & 0x01) {
					g *= 0.75;
					b *= 0.75;
				}
				if (j & 0x02) {
					r *= 0.75;
					b *= 0.75;
				}
				if (j & 0x04) {
					r *= 0.75;
					g *= 0.75;
				}
			}
#else
			r = (base[i].r * rtmul[j]) >> 15;
			g = (base[i].g * gtmul[j]) >> 15;
			b = (base[i].b * btmul[j]) >> 15;
#endif
			out[(j << 6) | i].r = (uint8_t)(r > 255 ? 255 : r);
			out[(j << 6) | i].g = (uint8_t)(g > 255 ? 255 : g);
			out[(j << 6) | i].b = (uint8_t)(b > 255 ? 255 : b);
		}
	}
}

void FCEU_SetPaletteUser(const pal *palette, const unsigned nEntries) {
	uint32_t x;

	for (x = 0; x < nEntries; x++) {
		palo[x].r = palette[x].r;
		palo[x].g = palette[x].g;
		palo[x].b = palette[x].b;
	}

	if (nEntries == 64) {
		GenerateEmphasis(palo, palette);
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

static uint8_t lastd = 0;
void SetNESDeemph_OldHacky(uint8_t d, int force) {
	uint32_t r, g, b;
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
			uint32_t m = (palo[x].r * r) >> 15;
			uint32_t n = (palo[x].g * g) >> 15;
			uint32_t o = (palo[x].b * b) >> 15;
			if(m > 0xff) m = 0xff;
			if(n > 0xff) n = 0xff;
			if(o > 0xff) o = 0xff;
			FCEUD_SetPalette(x | 0xC0, m, n, o);
		}
	}

	if (!d) return;	/* No deemphasis, so return. */

	r = rtmul[d];
	g = gtmul[d];
	b = btmul[d];

	for (x = 0; x < 0x40; x++) {
		uint32_t m = (palo[x].r * r) >> 15;
		uint32_t n = (palo[x].g * g) >> 15;
		uint32_t o = (palo[x].b * b) >> 15;
		if(m > 0xff) m = 0xff;
		if(n > 0xff) n = 0xff;
		if(o > 0xff) o = 0xff;
		FCEUD_SetPalette(x | 0x40, m, n, o);
	}

	lastd = d;
}
