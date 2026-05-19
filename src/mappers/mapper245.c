/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
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

#include "mapinc.h"
#include "mmc3.h"

static void SetPRG(uint16_t A, uint16_t V) {
	setprg8(A, ((mmc3.reg[0] & 0x02) << 5) | (V & 0x3F));
}

static void SetCHR(uint16_t A, uint16_t V) {
	setchr8(0);
}

static DECLFW(WriteMMC3) {
	if (A & 0x01) {
		mmc3.reg[mmc3.cmd & 0x07] = V;
	} else {
		mmc3.cmd = V;
	}
	MMC3_SyncPRG();
	MMC3_SyncCHR();
}

static void Power(void) {
	MMC3_Power();
	SetWriteHandler(0x8000, 0x9FFF, WriteMMC3);
}

void Mapper245_Init(CartInfo *info) {
	MMC3_Init(info, MMC3B, 8, info->battery);
	info->Power = Power;
	MMC3_pwrap = SetPRG;
	MMC3_cwrap = SetCHR;
}
