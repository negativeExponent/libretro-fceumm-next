/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2012 CaH4e3
 *  Copyright (C) 2002 Xodnizel
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

/* NTDEC TH2348 circuit board. UNROM plus m437.outer bank register at $5FFx. */

#include "mapinc.h"
#include "latch.h"

static struct {
	uint8_t outer;
} m437;

static SFORMAT StateRegs[] = {
	{ &m437.outer, 1, "OUTB" },
	{ 0 }
};

static void Sync(void) {
	setprg16(0x8000, (m437.outer << 3) | (latch.data & 0x07));
	setprg16(0xC000, (m437.outer << 3) | 0x07);
	setchr8(0);
	setmirror(((m437.outer >> 3) & 0x01) ^ 0x01);
}

static DECLFW(WriteOuter) {
	m437.outer = A & 0x0F;
	Sync();
}

static void Reset(void) {
	m437.outer = 0;
	Sync();
}

static void Power(void) {
	m437.outer = 0;
	Latch_Power();
	SetWriteHandler(0x5000, 0x5FFF, WriteOuter);
}

void Mapper437_Init(CartInfo *info) {
	Latch_Init(info, Sync, NULL, FALSE, TRUE);
	info->Reset = Reset;
	info->Power = Power;
	AddExState(StateRegs, ~0, 0, NULL);
}
