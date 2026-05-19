/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2012 CaH4e3
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 */

/* NES 2.0 Mapper denotes the prototype board used for the Maxivision 30 Super
 * Games prototype multicart. */

#include "mapinc.h"
#include "latch.h"

static struct {
	uint8_t reg;
} m493;

static SFORMAT StateRegs[] = {
	{ &m493.reg, 1, "EXPR" },
	{ 0 }
};

static void Sync(void) {
	setprg32(0x8000, ((m493.reg & 0x0F) << 1) | (latch.data & 0x01));
	setchr8(((m493.reg & 0x0F) << 3) | ((latch.data >> 4) & 0x07));
}

static DECLFW(WriteReg) {
	m493.reg = V;
	Sync();
}

static void Reset(void) {
	memset(&m493, 0, sizeof(m493));
	Latch_RegReset();
}

static void Power(void) {
	memset(&m493, 0, sizeof(m493));
	Latch_Power();
	SetWriteHandler(0x6000, 0x7FFF, WriteReg);
}

void Mapper493_Init(CartInfo *info) {
	Latch_Init(info, Sync, NULL, FALSE, TRUE);
	info->Power = Power;
	info->Reset = Reset;
	AddExState(StateRegs, ~0, 0, NULL);
}
