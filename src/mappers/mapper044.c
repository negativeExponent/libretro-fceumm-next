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
	uint8_t base = (mmc3.wram << 4) & 0x70;
	uint8_t mask = ((mmc3.wram & 0x06) == 0x06) ? 0x1F : 0x0F;

	setprg8(A, (base & ~mask) | (V & mask));
}

static void SetCHR(uint16_t A, uint16_t V) {
	uint16_t base = (mmc3.wram << 7) & 0x380;
	uint16_t mask = ((mmc3.wram & 0x06) == 0x06) ? 0xFF : 0x7F;

	setchr1(A, (base & ~mask) | (V & mask));
}

static DECLFW(WriteOuter) {
	switch (A & 0xE001) {
	case 0xA001:
		mmc3.wram = V;
		MMC3_SyncPRG();
		MMC3_SyncCHR();
		break;
	default:
		MMC3_Write(A, V);
		break;
	}
}

static void Power(void) {
	MMC3_Power();
	SetWriteHandler(0xA000, 0xBFFF, WriteOuter);
}

void Mapper044_Init(CartInfo *info) {
	MMC3_Init(info, MMC3B, 8, info->battery);
	MMC3_cwrap = SetCHR;
	MMC3_pwrap = SetPRG;
	info->Power = Power;
}
