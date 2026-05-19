/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
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
 * NES 2.0 Mapper 396 - BMC-830752C
 * 1995 Super 8-in-1 (JY-050 rev0)
 * Super 8-in-1 Gold Card Series (JY-085)
 * Super 8-in-1 Gold Card Series (JY-086)
 * 2-in-1 (Realtec PG-07, GN-51 PCB)
 *
 * Submappers:
 * Submapper 0: Outer Bank Register at $A000-$BFFF, Nametable Arrangement via D5 or D6 (combined submapper 1 and 2)
 * Submapper 1: Outer Bank Register at $A000-$BFFF, Nametable Arrangement via D6 (J.Y. YY850437C PCB variant)
 * Submapper 2: Outer Bank Register at $A000-$BFFF, Nametable Arrangement via D5 (Realtec GN-51 PCB variant)
 * Submapper 3: Outer Bank Register at $8000-$BFFF, Nametable Arrangement via D5 (Realtec 8030 PCB variant)
 */

#include "mapinc.h"

static struct {
	uint8_t reg[2];
} m396;

static SFORMAT StateRegs[] = {
	{ &m396.reg, 2, "EXPR" },
	{ 0 }
};

static void Sync(void) {
	uint16_t bank = (m396.reg[0] << 3) | (m396.reg[1] & 0x07);
	setprg16(0x8000, bank);
	setprg16(0xC000, bank | 0x07);
	setchr8(0);
	switch (iNESCart.submapper) {
	case 1: setmirror(((m396.reg[0] >> 6) & 0x01) ^ 0x01); break;
	case 2:
	case 3: setmirror(((m396.reg[0] >> 5) & 0x01) ^ 0x01); break;
	default: setmirror(((m396.reg[0] & 0x60) != 0) ^ 0x01); break;
	}
}

static DECLFW(WriteReg) {
	if (iNESCart.submapper == 3) {
		m396.reg[(A >> 14) & 0x01] = V;
		Sync();
	} else {
		if ((A & 0xE000) == 0xA000) {
			m396.reg[0] = V;
		} else {
			m396.reg[1] = V;
		}
		Sync();
	}
}

static void Reset(void) {
	memset(&m396, 0, sizeof(m396));
	Sync();
}

static void Power(void) {
	Reset();
	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0x8000, 0xFFFF, WriteReg);
}

void Mapper396_Init(CartInfo *info) {
	info->Power = Power;
	info->Reset = Reset;
	AddExState(StateRegs, ~0, 0, NULL);
}
