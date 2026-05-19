/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2012
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
 * Pegasus 5-in-1 (Golden Five) (Unl)
 */

#include "mapinc.h"

static struct {
	uint8_t reg[2];
	int32_t cycles;
} m104;

static SFORMAT StateRegs[] = {
	{ &m104.reg[0], 1, "INNB" },
	{ &m104.reg[1], 1, "OUTB" },
	{ &m104.cycles, 4 | FCEUSTATE_RLSB, "CYCL" },
	{ 0 }
};

static void Sync(void) {
	setprg8r(0x10, 0x6000, 0);
	setprg16(0x8000, (m104.reg[1] << 4) | (m104.reg[0] & 0x0F));
	setprg16(0xC000, (m104.reg[1] << 4) | 0x0F);
	setchr8(0);
}

static DECLFW(WriteOuter) {
	if (!(m104.reg[1] & 0x08) && (m104.cycles >= 110000)) {
		m104.reg[1] = V;
		Sync();
	}
}

static DECLFW(WriteInner) {
	m104.reg[0] = V;
	Sync();
}

static void CPUIRQHook(int a) {
	if (m104.cycles < 110000) {
		m104.cycles += a;
	}
}

static void Close(void) {
}

static void Power(void) {
	memset(&m104, 0, sizeof(m104));
	Sync();
	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0x8000, 0xBFFF, WriteOuter);
	SetWriteHandler(0xC000, 0xFFFF, WriteInner);
}

static void StateRestore(int version) {
	Sync();
}

void Mapper104_Init(CartInfo *info) {
	info->Power = Power;
	info->Close = Close;
	MapIRQHook = CPUIRQHook;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);
}
