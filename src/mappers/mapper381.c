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
 *
 */

/* Map 381 - 2-in-1 High Standard Game (BC-019), m381.reg-based */

#include "mapinc.h"
#include "latch.h"

static struct {
	uint8_t reg;
} m381;

static SFORMAT StateRegs[] = {
	{ &m381.reg, 1, "REGS" },
	{ 0 }
};

static void Sync(void) {
	uint8_t bank = (m381.reg << 4) | ((latch.data << 1) & 0x0E) | ((latch.data >> 4) & 0x01);

	setprg16(0x8000, bank);
	setprg16(0xC000, bank | 0x0F);
	setchr8(0);
}

static void Reset(void) {
	m381.reg++;
	Sync();
}

static void Power(void) {
	m381.reg = 0;
	Latch_Power();
}

void Mapper381_Init(CartInfo *info) {
	Latch_Init(info, Sync, NULL, FALSE, FALSE);
	info->Power = Power;
	info->Reset = Reset;
	AddExState(StateRegs, ~0, 0, NULL);
}
