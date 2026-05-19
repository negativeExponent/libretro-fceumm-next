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

static struct {
	uint8_t reg;
} m254;

static DECLFR(ReadWRAM) {
	if (m254.reg == TRUE) {
		return cpu.openbus ^ (m254.reg * 0x80);
	}
	return CartBR(A);
}

static DECLFW(WriteWRAM) {
	if (V & 0x01) {
		m254.reg = FALSE;
	}
	CartBW(A, V);
}

static void Reset(void) {
	m254.reg = TRUE;
	MMC3_SyncCHR();
	MMC3_SyncPRG();
}

static void Power(void) {
	m254.reg = 0x80;
	MMC3_Power();
	SetReadHandler(0x6000, 0x7FFF, ReadWRAM);
	SetWriteHandler(0x6000, 0x7FFF, WriteWRAM);
}

void Mapper254_Init(CartInfo *info) {
	MMC3_Init(info, MMC3B, 8, info->battery);
	info->Power = Power;
	info->Reset = Reset;
	AddExState(&m254.reg, 1, 0, "EXPR");
}
