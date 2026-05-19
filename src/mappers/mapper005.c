/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2002 Xodnizel
 *  Copyright (C) 2023-2024-2026 negativeExponent
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

/* None of this code should use any of the iNES bank switching wrappers. */

#include "mapinc.h"
#include "mmc5.h"

void Mapper005_Init(CartInfo *info) {
	WRAMSIZE = 64;
	if (info->iNES2) {
		WRAMSIZE = (info->PRGRamSize + info->PRGRamSaveSize) / 1024;
	}
	MMC5_Init(info, WRAMSIZE, info->battery);
}

/* ELROM seems to have 0KB of WRAM
 * EKROM seems to have 8KB of WRAM, battery-backed
 * ETROM seems to have 16KB of WRAM, battery-backed
 * EWROM seems to have 32KB of WRAM, battery-backed
 */

void ELROM_Init(CartInfo *info) {
	MMC5_Init(info, 0, 0);
}

void EKROM_Init(CartInfo *info) {
	MMC5_Init(info, 8, info->battery);
}

void ETROM_Init(CartInfo *info) {
	MMC5_Init(info, 16, info->battery);
}

void EWROM_Init(CartInfo *info) {
	MMC5_Init(info, 32, info->battery);
}
