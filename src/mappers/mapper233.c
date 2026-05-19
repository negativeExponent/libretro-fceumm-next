/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2005 CaH4e3
 *  Copyright (C) 2009 qeed
 *  Copyright (C) 2019 Libretro Team
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
 */

/* Updated 2019-07-12
 * Mapper 233 - UNIF 42in1ResetSwitch - m233.reg-based switching
 */

#include "mapinc.h"
#include "latch.h"

static struct {
	uint8_t reg;
} m233;

static SFORMAT StateRegs[] = {
	{ &m233.reg, 1, "REGS" },
	{ 0 }
};

static void Sync(void) {
	uint8_t bank = (m233.reg << 5) | (latch.data & 0x1F);

	if (latch.data & 0x20) {
		setprg16(0x8000, bank);
		setprg16(0xC000, bank);
	} else {
		setprg32(0x8000, bank >> 1);
	}
	setchr8(0);
	switch ((latch.data >> 6) & 0x03) {
	case 0:
		setmirror(MI_0);
		break;
	case 1:
		setmirror(MI_V);
		break;
	case 2:
		setmirror(MI_H);
		break;
	case 3:
		setmirror(MI_1);
		break;
	}
}

static void Power(void) {
	m233.reg = 0;
	Latch_Power();
}

static void Reset(void) {
	m233.reg ^= 1;
	Sync();
}

void Mapper233_Init(CartInfo *info) {
	Latch_Init(info, Sync, NULL, FALSE, FALSE);
	info->Power = Power;
	info->Reset = Reset;
	AddExState(StateRegs, ~0, 0, NULL);
}
