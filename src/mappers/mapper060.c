/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2020
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

#include "mapinc.h"

static struct {
	uint8_t reg;
} m060;

static void Sync(void) {
	setchr8(m060.reg);
	setprg16(0x8000, m060.reg);
	setprg16(0xC000, m060.reg);
}

static void Reset(void) {
	m060.reg++;
	Sync();
}

static void Power(void) {
	m060.reg = 0;
	Sync();
	SetReadHandler(0x8000, 0xFFFF, CartBR);
}

static void StateRestore(int version) {
	Sync();
}

void Mapper060_Init(CartInfo *info) {
	info->Power = Power;
	info->Reset = Reset;
	GameStateRestore = StateRestore;
	AddExState(&m060.reg, 1, 0, "REG0");
}
