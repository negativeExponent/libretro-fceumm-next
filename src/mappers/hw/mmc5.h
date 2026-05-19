/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2025-2026 negativeExponent
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

#ifndef _MMC5_H
#define _MMC5_H

typedef struct __MMC5 {
	uint8_t prgMode;
	uint8_t chrMode;
	uint8_t extMode;

	uint8_t prg[5];
	uint16_t chr[12];
	uint8_t chrLast;
	uint8_t wramProtect[2];
	uint8_t nmt;

	struct {
		uint8_t enabled;
		uint8_t pending;
		uint8_t scanlineCounter;
		uint8_t scanlineTarget;
		uint8_t inFrame;
	} irq;

	uint8_t mul[2];

	uint8_t fillTable[1024];
	uint8_t exRam[1024];
	uint8_t fillTile;
	uint8_t fillColor;
	uint8_t batteryFlag;
} MMC5;

extern MMC5 mmc5;

DECLFW(Mapper5_write);
DECLFW(MMC5_ExRAMWr);

void MMC5_Init(CartInfo *info, int wsize, int battery);

#endif /* _MMC5_H */
