/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2022 NewRisingSun
 *  Copyright (C) 2025-2026 negativeExponent
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

static struct {
	uint8_t reg[3];
} m310;

static SFORMAT StateRegs[] = {
	{ m310.reg, 4, "REGS" },
	{ 0 }
};

static void Sync(void) {
	uint16_t prg = (m310.reg[0] & 0x3F) | ((m310.reg[1] << 4) & ~0x3F);
	uint8_t chrProtect = FALSE;

	switch (m310.reg[1] & 3) {
	case 0:
		setprg32(0x8000, prg >> 1);
		chrProtect = TRUE;
		break;
	case 1:
		setprg16(0x8000, prg);
		setprg16(0xC000, prg | 7);
		break;
	case 2:
		prg = prg << 1 | m310.reg[0] >> 7;
		setprg8(0x8000, prg);
		setprg8(0xA000, prg);
		setprg8(0xC000, prg);
		setprg8(0xE000, prg);
		break;
	case 3:
		setprg16(0x8000, prg);
		setprg16(0xC000, prg);
		chrProtect = TRUE;
		break;
	}
	SetupCartCHRMapping(0, CHRptr[0], 0x8000, !chrProtect);
	setchr8(m310.reg[2]);
	setmirror((m310.reg[0] & 0x40) ? MI_H : MI_V);
}

static DECLFW(WriteReg0) {
	m310.reg[0] = V;
	Sync();
}

static DECLFW(WriteReg1) {
	m310.reg[1] = A & 0xFF;
	m310.reg[2] = V;
	Sync();
}

static void Reset(void) {
	memset(&m310, 0, sizeof(m310));
	Sync();
}

static void Power(void) {
	memset(&m310, 0, sizeof(m310));
	Sync();
	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0x8000, 0xBFFF, WriteReg0);
	SetWriteHandler(0xC000, 0xFFFF, WriteReg1);
}

static void StateRestore(int version) {
	Sync();
}

void Mapper310_Init(CartInfo *info) {
	info->Power = Power;
	info->Reset = Reset;
	GameStateRestore = StateRestore;
	AddExState(&StateRegs, ~0, 0, 0);
}
