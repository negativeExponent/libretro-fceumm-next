/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2026 negativeExponent
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include "mapinc.h"
#include "latch.h"

static struct {
	uint8_t reg;
} m607;

static SFORMAT StateRegs[] = {
	{ &m607.reg, 4, "EXPR" },
	{ 0 }
 };

static void Sync () {
	if (m607.reg & 0x10) { /* UNROM mode */
		setprg16(0x8000, ((m607.reg << 3)) | (latch.data & 0x07));
		setprg16(0xC000, ((m607.reg << 3) & ~0x07) | 0x07);
	} else { /* NROM-256 mode */
		setprg32(0x8000, m607.reg << 3 & 0x04 | latch.data >> 1 & 0x03);
	}
	setchr8(0);
	setmirror(m607.reg & 0x20 ? MI_H : MI_V);
}

static DECLFR(ReadCart) {
	if (m607.reg & 0x0C) {
		return cpu.openbus;
	}
	return CartBR(A);
}

static DECLFW(WriteReg) {
	m607.reg = V;
	Sync();
}

static void Reset(void) {
	m607.reg = 0;
	Latch_RegReset();
}

static void Power(void) {
	memset(&m607, 0, sizeof(m607));
	Latch_Power();
	SetReadHandler(0x8000, 0xFFFF, ReadCart);
	SetWriteHandler(0x6000, 0x7FFF, WriteReg);
}

static void StateRestore(int version) {
	Sync();
}

void Mapper607_Init(CartInfo *info) {
	Latch_Init(info, Sync, NULL, FALSE, FALSE);
	info->Power = Power;
	info->Reset = Reset;
	AddExState(StateRegs, ~0, 0, NULL);
}
