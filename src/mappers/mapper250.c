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

#include "mapinc.h"
#include "mmc3.h"

static void SetPRGBank_mmc3(uint16_t A, uint16_t V) {
	setprg8(A, V & 0x3F);
}

static void SetCHRBank_mmc3(uint16_t A, uint16_t V) {
	setchr1(A, V & 0xFF);
}

static DECLFW(WriteMMC3) {
	MMC3_Write(((A & 0xE000) | ((A & 0x400) >> 10)), (A & 0xFF));
}

static void Power(void) {
	MMC3_Power();
	SetWriteHandler(0x8000, 0xFFFF, WriteMMC3);
}

void Mapper250_Init(CartInfo *info) {
	MMC3_Init(info, MMC3B, 8, info->battery);
	MMC3_pwrap = SetPRGBank_mmc3;
	MMC3_cwrap = SetCHRBank_mmc3;
	info->Power = Power;
}
