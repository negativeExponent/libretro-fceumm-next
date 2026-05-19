/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2005 CaH4e3
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
 *
 * iNES Mapper 238
 * SL12 Protected 3-in-1 mapper hardware (VRC2, MMC3, MMC1)
 * the same as 603-5052 board (TODO: add reading registers, merge)
 *
 * Contra Fighter prot board
 */

#include "mapinc.h"
#include "mmc3.h"

static struct {
	uint8_t reg;
} m238;

static const uint8_t lut[4] = { 0x00, 0x02, 0x02, 0x03 };

static DECLFW(WriteProtection) {
	m238.reg = lut[V & 0x03];
}

static DECLFR(ReadProtection) {
	return m238.reg;
}

static void Power(void) {
	m238.reg = 0;
	MMC3_Power();
	SetWriteHandler(0x4020, 0x5FFF, WriteProtection);
	SetReadHandler(0x4020, 0x5FFF, ReadProtection);
}

void Mapper238_Init(CartInfo *info) {
	MMC3_Init(info, MMC3B, 0, 0);
	info->Power = Power;
	AddExState(&m238.reg, 1, 0, "EXPR");
}
