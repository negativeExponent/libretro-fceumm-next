/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2015 CaH4e3
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

/* NES 2.0 Mapper 519
 * UNIF board name UNL-EH8813A
 */

#include "mapinc.h"
#include "latch.h"

static struct {
	uint8_t scratch[4];
} m519;

static SFORMAT StateRegs[] = {
	{ m519.scratch, 4, "SCRA" },
	{ 0 }
};

static uint8_t dipsw;

static void Sync(void) {
	if (latch.addr & 0x80) {
		setprg16(0x8000, latch.addr);
		setprg16(0xC000, latch.addr);
	} else {
		setprg32(0x8000, latch.addr >> 1);
	}
	setchr8(latch.data);
	setmirror(((latch.data >> 7) & 0x01) ^ 0x01);
}

static DECLFR(ReadRAM) {
	return m519.scratch[A & 0x03];
}

static DECLFW(WriteRAM) {
	m519.scratch[A & 0x03] = V & 0x0F;
}

static DECLFR(ReadDIP) {
	if (latch.addr & 0x40) {
		return CartBR((A & 0xFFF0) | (dipsw & 0x0F));
	}
	return CartBR(A);
}

static DECLFW(WriteLatch) {
	if (A & 0x100) {
		V = (latch.data & 0xFC) | (V & 0x03);
	}
	Latch_Write(A, V);
}

static void Power(void) {
	memset(&m519, 0, sizeof(m519));
	Latch_Power();
	SetReadHandler(0x5800, 0x5FFF, ReadRAM);
	SetWriteHandler(0x5800, 0x5FFF, WriteRAM);
	SetWriteHandler(0x8000, 0xFFFF, WriteLatch);
}

static void Reset(void) {
	dipsw++;
	Latch_RegReset();
}

void Mapper519_Init(CartInfo *info) {
	Latch_Init(info, Sync, ReadDIP, FALSE, FALSE);
	info->Reset = Reset;
	info->Power = Power;
	AddExState(StateRegs, ~0, 0, NULL);
}
