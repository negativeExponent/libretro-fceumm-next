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
 */

#include "mapinc.h"

static uint8_t dipsw;
static struct {
	uint8_t reg[2];
} m057;

static SFORMAT StateRegs[] = {
	{ &dipsw, 1, "DPSW" },
	{ m057.reg, 2, "REGS" },
	{ 0 }
};

static void Sync(void) {
	uint8_t prg = (m057.reg[1] >> 5) & 0x07;
	uint8_t chr = ((m057.reg[0] >> 3) & 0x08) | (m057.reg[1] & 0x07);

	if (m057.reg[1] & 0x10) {
		setprg32(0x8000, prg >> 1);
	} else {
		setprg16(0x8000, prg);
		setprg16(0xC000, prg);
	}
	if (m057.reg[0] & 0x80) {
		setchr8(chr);
	} else {
		setchr8((chr & ~0x03) | (m057.reg[0] & 0x03));
	}
	setmirror(((m057.reg[1] >> 3) & 0x01) ^ 0x01);
}

static DECLFR(ReadDIP) {
	return dipsw & 0x03;
}

static DECLFW(WriteReg) {
	if (A & 0x2000) {
		m057.reg[(A >> 11) & 0x01] = (m057.reg[(A >> 11) & 0x01] & ~0x40) | (V & 0x40);
	} else {
		m057.reg[(A >> 11) & 0x01] = V;
	}
	Sync();
}

static void Power(void) {
	m057.reg[1] = m057.reg[0] = 0; /* Always reset to menu */
	dipsw = 1; /* YH-xxx "Olympic" multicarts disable the menu after one selection */
	SetReadHandler(0x5000, 0x6FFF, ReadDIP);
	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0x8000, 0xFFFF, WriteReg);
	Sync();
}

static void Reset(void) {
	m057.reg[1] = m057.reg[0] = 0; /* Always reset to menu */
	dipsw++;
	FCEU_printf("Select Register = %02x\n", dipsw);
	Sync();
}

static void StateRestore(int version) {
	Sync();
}

void Mapper057_Init(CartInfo *info) {
	info->Power = Power;
	info->Reset = Reset;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);
}
