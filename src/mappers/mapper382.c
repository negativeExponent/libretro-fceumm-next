/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2023-2025-2026 negativeExponent
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

/* NES 2.0 Mapper 382 - denotes the 830928C circuit board,
 * used on a 512 KiB 5-in-1 and a 1 MiB 9-in-1 multicart containing
 * the BNROM game Journey to the West and Capcom/Konami UNROM games.
 */

#include "mapinc.h"
#include "latch.h"

static struct {
	uint8_t reg;
} m382;

static SFORMAT StateRegs[] = {
	{ &m382.reg, 1, "REGS" },
	{ 0 }
};

static void Sync(void) {
	if (!(m382.reg & 0x20)) {
		m382.reg = latch.addr & 0x3F;
	}
	if (m382.reg & 0x08) { /* bnrom */
		setprg32(0x8000, (m382.reg << 2) | (latch.data & 0x03));
	} else { /* unrom */
		setprg16(0x8000, (m382.reg << 3) | (latch.data & 0x07));
		setprg16(0xC000, (m382.reg << 3) | 0x07);
	}
	setchr8(0);
	setmirror(((m382.reg >> 4) & 0x01) ^ 0x01);
}

static void Reset(void) {
	m382.reg = 0;
	Latch_RegReset();
}

static void Power(void) {
	m382.reg = 0;
	Latch_Power();
}

void Mapper382_Init(CartInfo *info) {
	Latch_Init(info, Sync, NULL, FALSE, TRUE);
	info->Power = Power;
	info->Reset = Reset;
	AddExState(StateRegs, ~0, 0, NULL);
}
