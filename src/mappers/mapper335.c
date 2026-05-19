/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2019 Libretro Team
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
 */

/* NES 2.0 mapper 335 is used for a 10-in-1 multicart.
 * Its UNIF board name is BMC-CTC-09.
 * http://wiki.nesdev.com/w/index.php/NES_2.0_Mapper_335 */

#include "mapinc.h"

static struct {
	uint8_t reg[2];
} m335;

static SFORMAT StateRegs[] = {
	{ m335.reg, 2, "REGS" },
	{ 0 }
};

static void Sync(void) {
	uint16_t prg = (m335.reg[1] << 1) | ((m335.reg[1] >> 3) & 0x01);

	if (m335.reg[1] & 0x10) {
		setprg16(0x8000, prg);
		setprg16(0xC000, prg);
	} else {
		setprg32(0x8000, prg >> 1);
	}
	setchr8(m335.reg[0]);
	setmirror(((m335.reg[1] >> 5) & 0x01) ^ 0x01);
}

static DECLFW(WriteReg) {
	switch (A & 0xE000) {
	case 0x8000:
	case 0xA000:
		m335.reg[0] = V;
		Sync();
		break;
	case 0xC000:
	case 0xE000:
		m335.reg[1] = V;
		Sync();
		break;
	}
}

static void Reset(void) {
	memset(&m335, 0, sizeof(m335));
	Sync();
}

static void Power(void) {
	memset(&m335, 0, sizeof(m335));
	Sync();
	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0x8000, 0xFFFF, WriteReg);
}

static void StateRestore(int version) {
	Sync();
}

void Mapper335_Init(CartInfo *info) {
	info->Power = Power;
	info->Reset = Reset;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);
}
