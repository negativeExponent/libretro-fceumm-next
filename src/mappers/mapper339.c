/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright (C) 2019 Libretro Team
 *  Copyright (C) 2023-2025 negativeExponent
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
 */

/* NES 2.0 mapper 339 is used for a 21-in-1 multicart.
 * Its UNIF board name is BMC-K-3006.
 * http://wiki.nesdev.com/w/index.php/NES_2.0_Mapper_339
 */

#include "mapinc.h"
#include "mmc3.h"

static struct {
	uint8 reg;
} m339;

static SFORMAT StateRegs[] = {
	{ &m339.reg, 1, "EXPR" },
	{ 0 }
};

static void SetPRG(uint16 A, uint16 V) {
	uint16 base = m339.reg << 1;
	uint16 mask = 0x0F;

	if (!(m339.reg & 0x20)) { /* NROM */
		if ((m339.reg & 0x06) == 0x06) { /* NROM-256 */
			mask = 0x03;
		} else { /* NROM-128 */
			mask = 0x01;
		}
		V = (A >> 13);
	}

	setprg8(A, (base & ~mask) | (V & mask));
}

static void SetCHR(uint16 A, uint16 V) {
	uint16 base = m339.reg << 4;
	uint16 mask = 0x7F; 

	setchr1(A, (base & ~mask) | (V & mask));
}

static DECLFW(WriteReg) {
	if (MMC3_WramIsWritable()) {
		m339.reg = A & 0xFF;
		MMC3_SyncPRG();
		MMC3_SyncCHR();
	}
}

static void Reset(void) {
	memset(&m339, 0, sizeof(m339));
	MMC3_Reset();
}

static void Power(void) {
	memset(&m339, 0, sizeof(m339));
	MMC3_Power();
	SetWriteHandler(0x6000, 0x7FFF, WriteReg);
}

void Mapper339_Init(CartInfo *info) {
	MMC3_Init(info, MMC3B, 0, 0);
	MMC3_pwrap = SetPRG;
	MMC3_cwrap = SetCHR;
	info->Power = Power;
	info->Reset = Reset;
	AddExState(StateRegs, ~0, 0, NULL);
}
