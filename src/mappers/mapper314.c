/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2005 CaH4e3
 *  Copyright (C) 2023-2024-2026 negativeExponent
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
 * BMC 42-in-1 "reset switch" type
 */

#include "mapinc.h"
#include "latch.h"

static struct {
	uint8_t reg[4];
} m314;

static SFORMAT StateRegs[] = {
	{ m314.reg, 4, "REGS" },
	{ 0 }
};

static void Sync(void) {
	uint16_t bank = ((m314.reg[0] << 7) & 0x80) | ((m314.reg[1] << 1) & 0x7E) | ((m314.reg[1] >> 6) & 0x01);

	if (m314.reg[0] & 0x80) { /* NROM mode */
		if (m314.reg[1] & 0x80) {
			setprg32(0x8000, bank >> 1);
		} else {
			setprg16(0x8000, bank);
			setprg16(0xC000, bank);
		}
	} else { /* UNROM mode */
		setprg16(0x8000, (bank & ~0x07) | (latch.data & 0x07));
		setprg16(0xC000, bank | 0x07);
	}
	setchr8((m314.reg[2] << 2) | ((m314.reg[0] >> 1) & 0x03));
	setmirror(((m314.reg[0] >> 5) & 0x01) ^ 0x01);
}

static DECLFW(WriteReg) {
	uint8_t mask = ROM.chr.size ? 0x03 : 0x01;

	m314.reg[A & mask] = V;
	Sync();
}

static void Reset(void) {
	/* Reset returns to menu */
	memset(&m314, 0, sizeof(m314));
	m314.reg[0] = 0x80;
	m314.reg[1] = 0x43;
	Sync();
}

static void Power(void) {
	memset(&m314, 0, sizeof(m314));
	m314.reg[0] = 0x80;
	m314.reg[1] = 0x43;
	Latch_Power();
	SetWriteHandler(0x5000, 0x5FFF, WriteReg);
}

void Mapper314_Init(CartInfo *info) {
	Latch_Init(info, Sync, NULL, FALSE, TRUE);
	info->Power = Power;
	info->Reset = Reset;
	AddExState(StateRegs, ~0, 0, NULL);
}
