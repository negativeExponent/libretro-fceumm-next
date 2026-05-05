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
	uint8_t pad;
} m599;

static SFORMAT StateRegs[] = {
	{ &m599.pad, 1, "PADS" },
	{ 0 }
};

static void Sync(void) {
	uint16_t prgMask, prgBase;

	if (PRG_BANK_COUNT(16) >= 32) {
		prgMask = (latch.addr & 0x400) ? 0x07 : 0x1F;
		prgBase = (latch.addr & 0x400) ? 0x20 : 0x00;
	} else {
		prgMask = (latch.addr & 0x400) ? 0x07 : 0x0F;
		prgBase = (latch.addr & 0x400) ? 0x10 : 0x00;
	}
	if (latch.addr & 0x80) {
		if (latch.addr & 0x01) {
			setprg32(0x8000, (prgBase | ((latch.addr >> 0x02) & prgMask)) >> 1);
		} else {
			setprg16(0x8000, (prgBase | ((latch.addr >> 0x02) & prgMask)));
			setprg16(0xC000, (prgBase | ((latch.addr >> 0x02) & prgMask)));
		}
	} else {
		setprg16(0x8000, (prgBase | ((latch.addr >> 0x02) & prgMask)));
		setprg16(0xC000, prgBase);
	}
	if (latch.addr & 0x400) {
		setchr8(((latch.addr >> 6) & ~0x03) | (latch.data & 0x03));
	} else {
		setchr8r(0x10, 0);
	}
	setmirror(((latch.addr << 1) & 0x01) ^ 0x01);
}

static DECLFR(ReadCart) {
	if (PRG_BANK_COUNT(16) >= 32 && latch.addr & 0x200 && m599.pad & 1) {
		return cpu.openbus;
	}
	return CartBR(A);
}

static DECLFW(WriteCart) {
	if (latch.addr & 0x4000) {
		latch.addr = A;
	}
	latch.data = V;
	Sync();
}

static void Power(void) {
	m599.pad = 0;
	Latch_Power();
}

static void Reset(void) {
	m599.pad++;
	Latch_RegReset();
}

void Mapper599_Init(CartInfo *info) {
	Latch_Init(info, Sync, ReadCart, 0, FALSE);
	info->Reset = Latch_RegReset;

	CHRRAMSIZE = 8192;
	CHRRAM = (uint8_t *)FCEU_gmalloc(CHRRAMSIZE);
	SetupCartCHRMapping(0x10, CHRRAM, CHRRAMSIZE, 1);
	AddExState(CHRRAM, CHRRAMSIZE, 0, "CHRR");
}
