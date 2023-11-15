/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2024 negativeExponent
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

/* NES 2.0 Mapper 373 - SFC-13 */

#include "mapinc.h"
#include "mmc3.h"

static uint8 reg[4];
static uint8 cmd;

static SFORMAT StateRegs[] = {
	{ reg, 4, "REGS" },
	{ &cmd, 1, "CMD0" },
	{ 0 }
};

static void M373CW(uint16 A, uint16 V) {
	uint32 mask = 0xFF >> (~reg[2] & 0xF);
	uint32 base = reg[0] | reg[2] << 4 & 0xF00;

	setchr1(A, base | (V & mask));
}

static void M373PW(uint16 A, uint16 V) {
	uint16 mask = ~reg[3] & 0x3F;
	uint16 base = reg[1] | reg[2] << 2 & 0x300;

	if (reg[2] & 0x20) { /* GNROM-like */
		setprg8(0x8000, base | ((mmc3.reg[6] & ~0x02) & mask));
		setprg8(0xA000, base | ((mmc3.reg[7] & ~0x02) & mask));
		setprg8(0xC000, base | ((mmc3.reg[6] | 0x02) & mask));
		setprg8(0xE000, base | ((mmc3.reg[7] | 0x02) & mask));
	} else {
		setprg8(A, base | (V & mask));
	}
}

static DECLFW(M373WriteReg) {
	CartBW(A, V);
	if (!(reg[3] & 0x40)) {
		reg[cmd] = V;
		cmd = (cmd + 1) & 0x03;
		MMC3_FixPRG();
		MMC3_FixCHR();
	}
}

static void M373Reset(void) {
	reg[0] = reg[1] = reg[3] = cmd = 0;
	reg[2] = 0x0F;
	MMC3_Reset();
}

static void M373Power(void) {
	reg[0] = reg[1] = reg[3] = cmd = 0;
	reg[2] = 0x0F;
	MMC3_Power();
	SetWriteHandler(0x6000, 0x7FFF, M373WriteReg);
}

void Mapper373_Init(CartInfo *info) {
	MMC3_Init(info, MMC3B, 8, info->battery);
	MMC3_cwrap = M373CW;
	MMC3_pwrap = M373PW;
	info->Reset = M373Reset;
	info->Power = M373Power;
	AddExState(StateRegs, ~0, 0, NULL);
}
