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

/*
 * NES 2.0 Mapper 553 denotes the 聖謙 (Sachen) 3013 circuit board, used on only
 * one game: 動動腦 1, the original Chinese-language version of The Penguin and
 * Seal. It is basically NROM-128 with an additional 74LS244 and TC40H139P,
 * which cause the CPU $8000-$BFFF range to return a constant value of $3A
 * instead of a mirror of the CPU $C000-$FFFF range.
 */

#include "mapinc.h"

static DECLFR(ReadProtection) {
	return 0x3A;
}

static void Power(void) {
	setprg16(0xC000, 0);
	setchr8(0);
	SetReadHandler(0x8000, 0xBFFF, ReadProtection);
	SetReadHandler(0xC000, 0xFFFF, CartBR);
}

void Mapper553_Init(CartInfo *info) {
	info->Power = Power;
}
