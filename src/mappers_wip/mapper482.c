/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include "mapinc.h"
#include "latch.h"

static struct {
	uint8_t dipsw;
} m482;

static SFORMAT StateRegs[] = {
    { &m482.dipsw, 1, "DIPS" },
    { 0 }
};

static void Sync(void) {
	uint16_t bank = latch.data & 0x3F;

	if (latch.data & 0x80) {
		setprg16(0x8000, bank);
		setprg16(0xC000, bank);
	} else {
		setprg32(0x8000, bank >> 1);
	}
	setchr8(0);
	setmirror(((latch.data >> 6) & 0x01) ^ 0x01);
}

static DECLFR(ReadLatch) {
	if ((latch.addr & 0x03) == 0x03) {
		return (m482.dipsw & 0x07);
	}
	return CartBR(A);
}

static void Reset(void) {
	m482.dipsw++;
	Latch_RegReset();
}

static void Power(void) {
	m482.dipsw = 0;
	Latch_Power();
}

void Mapper482_Init(CartInfo *info) {
	Latch_Init(info, Sync, ReadLatch, 0, 0);
	info->Power = Power;
	info->Reset = Reset;
	AddExState(StateRegs, ~0, 0, NULL);
}
