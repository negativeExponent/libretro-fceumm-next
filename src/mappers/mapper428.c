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
 */

#include "mapinc.h"
#include "latch.h"

static struct {
	uint8_t reg[4];
} m428;

static uint8_t dipsw;

static SFORMAT StateRegs[] = {
	{ m428.reg, 4, "REGS" },
	{ 0 }
};

static void Sync(void) {
	uint8_t prg = m428.reg[1] >> 5;
	uint8_t chrmask = m428.reg[2] >> 6;

	if (m428.reg[1] & 0x10) {
		setprg32(0x8000, prg >> 1);
	} else {
		setprg16(0x8000, prg);
		setprg16(0xC000, prg);
	}

	setchr8(((m428.reg[1] & 0x07) & ~chrmask) | (latch.data & chrmask));
	setmirror(((m428.reg[1] >> 3) & 0x01) ^ 0x01);
}

static DECLFR(ReadDIP) {
	return (cpu.openbus & ~0x03) | (dipsw & 0x03);
}

static DECLFW(WriteReg) {
	m428.reg[A & 0x03] = V;
	Sync();
}

static void Reset(void) {
	memset(&m428, 0, sizeof(m428));
	dipsw++;
	Sync();
}

static void Power(void) {
	memset(&m428, 0, sizeof(m428));
	dipsw = 0;
	Latch_Power();
	SetReadHandler(0x6000, 0x7FFF, ReadDIP);
	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0x6000, 0x7FFF, WriteReg);
}

void Mapper428_Init(CartInfo *info) {
	Latch_Init(info, Sync, NULL, FALSE, FALSE);
	info->Power = Power;
	info->Reset = Reset;
	AddExState(StateRegs, ~0, 0, NULL);
}
