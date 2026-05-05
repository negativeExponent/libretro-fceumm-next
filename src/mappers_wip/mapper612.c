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
} m612;

static SFORMAT StateRegs[] = {
	{ &m612.reg, 1, "EXPR" },
	{ 0 }
 };

static void Sync(void) {
	setprg16(0x8000, (m612.reg & 0x03) | (latch.data & 0x07));
	setprg16(0xC000, m612.reg & 0x03);
	setchr8(0);
}

static void Reset(void) {
	m612.reg++;
	Latch_RegReset();
}

static void Power(void) {
	m612.reg = 0;
	Latch_Power();
}

void Mapper612_Init(CartInfo *info) {
	Latch_Init(info, Sync, NULL, FALSE, FALSE);
	info->Power = Power;
	info->Reset = Reset;
	AddExState(StateRegs, ~0, 0, NULL);
}
