/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2005 CaH4e3
 *  Copyright (C) 2020
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

#include "mapinc.h"
#include "latch.h"

static struct {
	uint8_t reg;
} m235;

static SFORMAT StateRegs[] = {
	{ &m235.reg, 1, "REGS" },
	{ 0 }
};

static void Sync(void) {
	if (m235.reg) { /* Contra */
		setprg16(0x8000, (PRG_BANK_COUNT(16) & 0xC0) | (latch.data & 0x07));
		setprg16(0xC000, (PRG_BANK_COUNT(16) & 0xC0) | 0x07);
		setchr8(0);
		setmirror(MI_V);
	} else {
		uint16_t bank = (((latch.addr >> 3) & 0x60) | (latch.addr & 0x1F));
		uint8_t rd = bank < PRG_BANK_COUNT(16);

		if (bank >= PRG_BANK_COUNT(16)) {
			unsetcpu32(0x8000);
		} else {
			if (latch.addr & 0x800) {
				setprg16(0x8000, (bank << 1) | ((latch.addr >> 12) & 0x01));
				setprg16(0xC000, (bank << 1) | ((latch.addr >> 12) & 0x01));
			} else {
				setprg32(0x8000, bank);
			}
		}
		setchr8(0);
		if (latch.addr & 0x400) {
			setmirror(MI_0);
		} else {
			setmirror(((latch.addr >> 13) & 0x01) ^ 0x01);
		}
	}
}

static void Reset(void) {
	if (ROM.prg.size & 0x20000) {
		m235.reg = (m235.reg + 1) & 1;
	}

	Latch_RegReset();
	FCEU_printf("mode : %s\n", m235.reg ? "contra" : "multicart");
}

void Mapper235_Init(CartInfo *info) {
	memset(&m235, 0, sizeof(m235));
	Latch_Init(info, Sync, NULL, FALSE, FALSE);
	info->Reset = Reset;
	AddExState(StateRegs, ~0, 0, NULL);
}
