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

static uint8 reg;
static uint8 dipsw;

static void SetCHRBank_mmc3(uint16 A, uint16 V) {
	uint16 base = reg << ((A & 0x1000) ? 4 : 8);

	setchr1(A, (base & 0x100) | (V & 0xFF));
}

static DECLFW(WriteReg) {
	if (A & 0x100) {
		reg = V;
		MMC3_SyncCHR();
	}
}

static DECLFR(ReadDIP) {
	if (A & 0x100) {
		return dipsw;
	}
	return CartBR(A);
}

static void Power(void) {
	reg = 0;
	dipsw = 1; /* chinese is default */
	MMC3_Power();
	SetWriteHandler(0x4100, 0x4FFF, WriteReg);
	SetReadHandler(0x4100, 0x4FFF, ReadDIP);
}

static void Reset(void) {
	reg = 0;
	dipsw ^= 1;
	MMC3_Reset();
}

void Mapper012_Init(CartInfo *info) {
	if (info->submapper == 1) {
		Mapper006_Init(info);
	} else {
		int ws = (info->PRGRamSize + info->PRGRamSaveSize) / 1024;
		MMC3_Init(info, MMC3A, ws ? ws : 8, info->battery);
		MMC3_cwrap = SetCHRBank_mmc3;

		info->Power = Power;
		info->Reset = Reset;
		AddExState(&reg, 1, 0, "EXPR");
		AddExState(&dipsw, 1, 0, "DPSW");
	}
}
