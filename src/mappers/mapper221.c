/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2006 CaH4e3
 *  Copyright (C) 2023-2024-2026 negativeExponent
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR m221.reg[0] PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Mapper 221 - UNL-N625092
 * 700in1 and 400in1 carts
 * 1000-in-1
 */

#include "mapinc.h"

static struct {
	uint16_t reg[2];
} m221;

static SFORMAT StateRegs[] = {
	{ &m221.reg[0], 2 | FCEUSTATE_RLSB, "REG0" },
	{ &m221.reg[1], 2 | FCEUSTATE_RLSB, "REG1" },
	{ 0 }
};

static uint16_t GetPRGBase(void) {
	uint32_t rshift = (iNESCart.submapper == 1) ? 2 : 3;
	return (((m221.reg[0] >> rshift) & 0x40) | ((m221.reg[0] >> 2) & 0x38));
}

static void Sync(void) {
	uint16_t prg = GetPRGBase() | (m221.reg[1] & 0x07);
	uint16_t unrom_mask = (iNESCart.submapper == 1) ? 0x200 : 0x100;
	uint8_t prot = (iNESCart.submapper == 1) ? (m221.reg[0] & 0x0400) : (m221.reg[1] & 0x0008);

	if (m221.reg[0] & unrom_mask) { /* UNROM */
		setprg16(0x8000, prg);
		setprg16(0xC000, prg | 0x07);
	} else {
		if (m221.reg[0] & 0x02) { /* NROM-256 */
			setprg32(0x8000, prg >> 1);
		} else { /* NROM-128 */
			setprg16(0x8000, prg);
			setprg16(0xC000, prg);
		}
	}

	SetupCartCHRMapping(0, CHRptr[0], CHRsize[0], !prot);

	setchr8(0);
	setmirror((m221.reg[0] & 0x01) ^ 0x01);
}

static DECLFR(ReadProtect) {
	if (GetPRGBase() >= PRG_BANK_COUNT(16)) {
		/* Selecting unpopulated banks results in open bus */
		return cpu.openbus;
	}
	return CartBR(A);
}

static DECLFW(WriteReg) {
	m221.reg[(A >> 14) & 0x01] = A;
	Sync();
}

static void Reset(void) {
	m221.reg[0] = m221.reg[1] = 0;
	Sync();
}

static void Power(void) {
	m221.reg[0] = m221.reg[1] = 0;
	Sync();
	SetReadHandler(0x8000, 0xFFFF, ReadProtect);
	SetWriteHandler(0x8000, 0xFFFF, WriteReg);
}

static void StateRestore(int version) {
	Sync();
}

void Mapper221_Init(CartInfo *info) {
	info->Power = Power;
	info->Reset = Reset;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);
}
