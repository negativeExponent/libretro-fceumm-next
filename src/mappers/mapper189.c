/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2005 CaH4e3
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

/* INES Mapper 189
 * Thunder Warrior - 轰天至尊 from TXC Corporation with its variant Gluk the Thunder Warrior
 * Street Fighter II: The World Warrior from Yoko Soft and its many variants named Super Fighter II´, Master Fighter II, Master Fighter III, Mario Fighter III'
 */

#include "mapinc.h"
#include "mmc3.h"

static struct {
	uint8_t reg;
} m189;

static void SetPRG(uint16_t A, uint16_t V) {
	setprg32(0x8000, m189.reg | (m189.reg >> 4));
}

static DECLFW(WriteReg45) {
	if (A & 0x100) {
		m189.reg = V;
		MMC3_SyncPRG();
	}
}

static DECLFW(WriteReg67) {
	if (MMC3_WramIsWritable()) {
		m189.reg = V;
		MMC3_SyncPRG();
	}
}

static void Power(void) {
	m189.reg = 3;
	MMC3_Power();
	SetWriteHandler(0x4120, 0x5FFF, WriteReg45);
	SetWriteHandler(0x6000, 0x7FFF, WriteReg67);
}

void Mapper189_Init(CartInfo *info) {
	MMC3_Init(info, MMC3B, 0, 0);
	MMC3_pwrap = SetPRG;
	info->Power = Power;
	AddExState(&m189.reg, 1, 0, "EXPR");
}
