/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2009 CaH4e3
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
 *
 * NES 2.0 mapper 331 is used for the 7-in-1 (NS03) multicart.
 * Its UNIF board name is BMC-12-IN-1.
 *
 * 7-in-1  Darkwing Duck, Snake, MagicBlock (PCB marked as "12 in 1")
 * 12-in-1 1991 New Star Co. Ltd.
 *
 */

#include "mapinc.h"

static struct {
	uint8_t reg[3];
	uint8_t ppuchrbus;
} m331;

static SFORMAT StateRegs[] = {
	{ m331.reg, 3, "REGS" },
	{ &m331.ppuchrbus, 1, "PPUC" },
	{ 0 }
};

static void Sync(void) {
	if (m331.reg[2] & 0x08) {
		setprg32(0x8000, ((m331.reg[2] << 3) | (m331.reg[m331.ppuchrbus] & 0x07)) >> 1); /* actually, both 0 and 1 registers used,
		      but they will switch each PA12 transition if bits are different
		      for both registers, so they must be programmed strongly the same! */
	} else {
		setprg16(0x8000, (m331.reg[2] << 3) | (m331.reg[m331.ppuchrbus] & 0x07));
		setprg16(0xc000, (m331.reg[2] << 3) | 0x07);
	}
	setchr4(0x0000, (m331.reg[2] << 5) | (m331.reg[0] >> 3));
	setchr4(0x1000, (m331.reg[2] << 5) | (m331.reg[1] >> 3));
	setmirror(((m331.reg[2] & 4) >> 2) ^ 1);
}

static DECLFW(WriteReg) {
	switch (A & 0xE000) {
	case 0xA000:
		m331.reg[0] = V;
		Sync();
		break;
	case 0xC000:
		m331.reg[1] = V;
		Sync();
		break;
	case 0xE000:
		m331.reg[2] = V;
		Sync();
		break;
	}
}

static void Reset(void) {
	memset(&m331, 0, sizeof(m331));
	Sync();
}

static void Power(void) {
	memset(&m331, 0, sizeof(m331));
	Sync();
	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0x8000, 0xFFFF, WriteReg);
}

static void StateRestore(int version) {
	Sync();
}

static void PPUHook(uint32_t A) {
	uint8_t bank = (A & 0x1000) >> 12;

	if ((m331.ppuchrbus != bank) && !(A & 0x2000)) {
		m331.ppuchrbus = bank;
		Sync();
	}
}

void Mapper331_Init(CartInfo *info) {
	info->Power = Power;
	info->Reset = Reset;
	PPU_hook = PPUHook;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);
}
