/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2019 Libretro Team
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

/* NES 2.0 Mapper 315
 * BMC-830134C
 * Used for multicarts using 820732C- and 830134C-numbered PCBs such as 4-in-1 Street Blaster 5
 * http://wiki.nesdev.com/w/index.php/NES_2.0_Mapper_315
 */

#include "mapinc.h"
#include "mmc3.h"

static struct {
	uint8_t reg;
} m315;

static SFORMAT StateRegs[] = {
	{ &m315.reg, 1, "EXPR"},
	{ 0 }
};

static void SetPRGBank(uint16_t A, uint16_t V) {
	uint16_t mask = 0x0F;
	uint16_t base = m315.reg << 3;

	if (m315.reg & 0x08) { /* GNROM-like */
		if (!(A & 0x4000)) {
			setprg8(A, (base & ~mask) | ((V & mask) & 0xFD));
			A += 0x4000;
			setprg8(A, (base & ~mask) | ((V & mask) | 0x02));
		}
	} else {
		setprg8(A, (base & ~mask) | (V & mask));
	}
}

static void SetCHRBank(uint16_t A, uint16_t V) {
	uint16_t mask = 0xFF;
	uint16_t base = m315.reg << 8;

	V = ((m315.reg << 6) & 0x80) | ((m315.reg << 3) & 0x40) | (V & 0xFF);
	setchr1(A, (base & ~mask) | (V & mask));
}

static DECLFW(WriteReg) {
	if (MMC3_WramIsWritable()) {
		m315.reg = A & 0xFF;
		MMC3_SyncPRG();
		MMC3_SyncCHR();
	}
}

static void Reset(void) {
	memset(&m315, 0, sizeof(m315));
	MMC3_Reset();
}

static void Power(void) {
	memset(&m315, 0, sizeof(m315));
	MMC3_Power();
	SetWriteHandler(0x6000, 0x7FFF, WriteReg);
}

void Mapper315_Init(CartInfo *info) {
	MMC3_Init(info, MMC3B, 0, 0);
	MMC3_pwrap = SetPRGBank;
	MMC3_cwrap = SetCHRBank;
	info->Power = Power;
	info->Reset = Reset;
	AddExState(StateRegs, ~0, 0, NULL);
}
