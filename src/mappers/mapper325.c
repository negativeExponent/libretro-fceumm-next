/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2023-2025-2026 negativeExponent
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

/* NES 2.0 325
 * UNIF UNL-MALISB
 */

#include "mapinc.h"
#include "mmc3.h"

static void SetPRGBank(uint16_t A, uint16_t V) {
	setprg8(A, (V & 0xF3) | ((V & 0x04) << 1) | ((V & 0x08) >> 1));
}

static void SetCHRBank(uint16_t A, uint16_t V) {
	setchr1(A, (V & 0xDD) | ((V & 0x02) << 4) | ((V & 0x20) >> 4));
}

static DECLFW(WriteMMC3) {
	A = (A & ~0x01) | ((A >> 3) & 0x01);
	MMC3_Write(A, V);
}

static void Power(void) {
	MMC3_Power();
	SetWriteHandler(0x8000, 0xFFFF, WriteMMC3);
}

void Mapper325_Init(CartInfo *info) {
	MMC3_Init(info, MMC3B, 0, 0);
	MMC3_pwrap = SetPRGBank;
	MMC3_cwrap = SetCHRBank;
	info->Power = Power;
}
