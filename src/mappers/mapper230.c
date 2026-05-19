/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2005 CaH4e3
 *  Copyright (C) 2009 qeed
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
 * 22 + Contra Reset based custom mapper...
 *
 */

#include "mapinc.h"
#include "latch.h"

static struct {
	uint8_t mode;
} m230;

static SFORMAT StateRegs[] = {
	{ &m230.mode, 1, "MODE" },
	{ 0 }
};

static void Sync(void) {
	if (m230.mode) { /* Contra m230.mode */
		setprg16(0x8000, latch.data & 0x07);
		setprg16(0xC000, 0x07);
		setmirror(MI_V);
	} else { /* multicart m230.mode */
		uint8_t bank = 0x08 + (latch.data & 0x1F);

		if (latch.data & 0x20) {
			setprg16(0x8000, bank);
			setprg16(0xC000, bank);
		} else {
			setprg32(0x8000, bank >> 1);
		}
		setmirror((latch.data >> 6) & 0x01);
	}
	setchr8(0);
}

static void Reset(void) {
	m230.mode ^= 1;
	Latch_RegReset();
}

static void Power(void) {
	m230.mode = 0;
	Latch_Power();
}

void Mapper230_Init(CartInfo *info) {
	Latch_Init(info, Sync, NULL, FALSE, FALSE);
	info->Power = Power;
	info->Reset = Reset;
	AddExState(StateRegs, ~0, 0, NULL);
}
