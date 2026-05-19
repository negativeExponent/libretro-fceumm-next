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
} m488;

static SFORMAT StateRegs[] = {
    { &m488.dipsw, 1, "DPSW" },
    { 0 }
};

static void Sync(void) {
    uint16_t bank = ((latch.addr << 1) | ((latch.addr >> 4) & 0x01));

	if (latch.addr & 0x04) {
		setprg32(0x8000, bank >> 1);
    } else {
		setprg16(0x8000, bank);
		setprg16(0xC000, bank);
	}
    setchr8(latch.addr);
}

static DECLFR(Read) {
	if (latch.addr & 0x100)
		A = A & ~0x0F | m488.dipsw & 0xF;
	return CartBR(A);
}

static void Reset(void) {
	m488.dipsw++;
	Latch_RegReset();
}

static void Power(void) {
    m488.dipsw = 0;
    Latch_Power();
}

void Mapper488_Init(CartInfo *info) {
    Latch_Init(info, Sync, Read, 0, 0);
    info->Power = Power;
	info->Reset = Reset;
	AddExState(StateRegs, ~0, 0, 0);
}
