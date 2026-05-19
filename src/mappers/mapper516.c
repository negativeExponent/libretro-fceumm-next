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

/* NES 2.0 Mapper 516 - Brilliant Com Cocoma Pack */

#include "mapinc.h"
#include "mmc3.h"

static struct {
	uint8_t reg;
} m516;

static SFORMAT StateRegs[] = {
	{ &m516.reg, 1, "EXPR" },
	{ 0 }
};

static void SetPRG(uint16_t A, uint16_t V) {
	/* FCEU_printf("PRG: A:%04x V:%02x R0:%02x\n", A, V, m516.reg); */
	setprg8(A, ((m516.reg << 4) & 0x30) | (V & 0x0F));
}

static void SetCHR(uint16_t A, uint16_t V) {
	/* FCEU_printf("CHR: A:%04x V:%02x R0:%02x\n", A, V, m516.reg); */
	setchr1(A, ((m516.reg << 5) & 0x180) | (V & 0x7F));
}

static DECLFW(WriteMMC3) {
	/* FCEU_printf("Wr: A:%04x V:%02x R0:%02x\n", A, V, m516.reg); */
	if (A & 0x10) {
		m516.reg = A & 0x0F;
	}
	MMC3_Write(A, V);
}

static void Power(void) {
	m516.reg = 0;
	MMC3_Power();
	SetWriteHandler(0x8000, 0xFFFF, WriteMMC3);
}

void Mapper516_Init(CartInfo *info) {
	MMC3_Init(info, MMC3B, 0, 0);
	MMC3_cwrap = SetCHR;
	MMC3_pwrap = SetPRG;
	info->Power = Power;
	AddExState(StateRegs, ~0, 0, NULL);
}
