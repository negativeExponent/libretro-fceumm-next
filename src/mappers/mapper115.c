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

static uint8 reg[2];
static uint8 dipsw;

static SFORMAT StateRegs[] = {
	{ reg, 2, "REGS" },
	{ &dipsw, 1, "DPSW" },
	{ 0 }
};

static void M115PW(uint16 A, uint16 V) {
	uint16 base = ((reg[0] >> 2) & 0x10) | (reg[0] & 0x0f);
	uint16 mask = 0x1F;
	if (reg[0] & 0x80) {
		if (reg[0] & 0x20) {
			setprg32(0x8000, base >> 1);
		} else {
			setprg16(0x8000, base);
			setprg16(0xC000, base);
		}
	} else {
		setprg8(A, ((base << 1) & ~mask) | (V & mask));
	}
}

static void M115CW(uint16 A, uint16 V) {
	setchr1(A, (reg[1] << 8) | V);
}

static DECLFR(M115Read) {
	return ((A & 0x03) == 0x02) ? dipsw : cpu.openbus;
}

static DECLFW(M115Write) {
	switch (A & 3) {
	case 0:
	case 1:
		reg[A & 0x01] = V;
		MMC3_FixPRG();
		MMC3_FixCHR();
		break;
	}
}

static void M115Reset(void) {
	dipsw++;
	MMC3_FixPRG();
	MMC3_FixCHR();
}

static void M115Power(void) {
	dipsw = 0;
	MMC3_Power();
	SetReadHandler(0x6000, 0x7FFF, M115Read);
	SetWriteHandler(0x6000, 0x7FFF, M115Write);
}

void Mapper115_Init(CartInfo *info) {
	MMC3_Init(info, MMC3B, 0, 0);
	MMC3_cwrap = M115CW;
	MMC3_pwrap = M115PW;
	info->Power = M115Power;
	info->Reset = M115Reset;
	AddExState(StateRegs, ~0, 0, NULL);
}
