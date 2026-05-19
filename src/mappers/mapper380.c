/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 * Copyright (C) 2023-2025-2026 negativeExponent
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

/* NES 2.0 Mapper 380 denotes the 970630C circuit board,
 * used on a 512 KiB multicart having 42 to 80,000 listed NROM and UNROM games. */

#include "mapinc.h"
#include "latch.h"

static uint8_t dipsw;
static uint32_t temp;

static SFORMAT StateRegs[] = {
	{ &dipsw, 4, "DPSW" },
	{ &temp, 4 | FCEUSTATE_RLSB, "TEMP" },
	{ 0 }
};

static void Sync(void) {
	uint8_t bank = (latch.addr >> 2) & 0x1F;

	if (latch.addr & 0x200) { /* NROM */
		if (latch.addr & 0x01) {
			setprg16(0x8000, bank);
			setprg16(0xC000, bank);
		} else {
			setprg32(0x8000, bank >> 1);
		}
	} else {
		setprg16(0x8000, bank);
		setprg16(0xC000, bank | (((iNESCart.submapper == 1) && (latch.addr & 0x100)) ? 0x0F : 0x07));
	}

	SetupCartCHRMapping(0, CHRptr[0], 0x2000, !(latch.addr & 0x80));

	setchr8(0);
	if (iNESCart.submapper == 2) {
		setmirror(((latch.addr >> 6) & 0x01) ^ 0x01);
	} else {
		setmirror(((latch.addr >> 1) & 0x01) ^ 0x01);
	}
}

static DECLFR(ReadDIP) {
	if ((iNESCart.submapper == 0) && (latch.addr & 0x100)) {
		A |= dipsw;
	}
	return CartBR(A);
}

static void Reset(void) {
	dipsw = (dipsw + 1) & 0xF;
	Latch_RegReset();
}

static void Power(void) {
	dipsw = 0;
	Latch_Power();
}

void Mapper380_Init(CartInfo *info) {
	Latch_Init(info, Sync, ReadDIP, FALSE, FALSE);
	info->Power = Power;
	info->Reset = Reset;
	AddExState(StateRegs, ~0, 0, NULL);
}
