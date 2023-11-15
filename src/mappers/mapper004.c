/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2023-2024 negativeExponent
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

#include "mapinc.h"
#include "mmc3.h"

void Mapper004_Init(CartInfo *info) {
	int ws = 8;

	if (!info->iNES2) {
		ws = 8; /* set wram to 8K for iNES 1.0 roms aside for Low G Man */
		if ((info->CRC32 == 0x93991433) || (info->CRC32 == 0xaf65aa84)) {
			FCEU_printf(
			    "Low-G-Man can not work normally in the iNES format.\nThis game has been recognized by its CRC32 "
			    "value, and the appropriate changes will be made so it will run.\nIf you wish to hack this game, "
			    "you should use the UNIF format for your hack.\n\n");
			ws = 0;
		}
		if (info->CRC32 == 0x97b6cb19) {
			info->submapper = 4;
		}
	} else {
		ws = (info->PRGRamSize + info->PRGRamSaveSize) / 1024;
	}

	switch (info->submapper) {
	default:
	case 0: MMC3_Init(info, MMC3B, ws, info->battery); break;
	case 1: MMC3_Init(info, MMC6B, ws, info->battery); break;
	case 4: MMC3_Init(info, MMC3A, ws, info->battery); break;
	case 5: Mapper249_Init(info); break;
	}
}
