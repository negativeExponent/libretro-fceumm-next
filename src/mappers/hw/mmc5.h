/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2025 negativeExponent
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
	uint8 prgMode;
	uint8 chrMode;
	uint8 extMode;

	uint8 prg[5];
	uint16 chr[12];
	uint8 chrLast;
	uint8 wramProtect[2];
	uint8 nmt;

	struct {
		uint8 enabled;
		uint8 pending;
		uint8 scanlineCounter;
		uint8 scanlineTarget;
		uint8 inFrame;
	} irq;

	uint8 mul[2];

	uint8 fillTable[1024];
	uint8 exRam[1024];
	uint8 fillTile;
	uint8 fillColor;
	uint8 batteryFlag;
} MMC5;

extern MMC5 mmc5;

void MMC5_Init(CartInfo *info, int wsize, int battery);

#endif /* _MMC5_H */