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

/* NES 2.0 Mapper 520 - VRC4 clone
 * Datach Dragon Ball Z/Datach Yu Yu Hakusho
 */

#include "mapinc.h"
#include "vrc24.h"

static struct {
	uint8_t ppuchrbus;
} m520;

static SFORMAT StateRegs[] = {
	{ &m520.ppuchrbus, 1, "PPUC" },
	{ 0 }
};

static void SetPRG(uint16_t A, uint16_t V) {
	setprg8(A, ((VRC24_GetCHRBank(m520.ppuchrbus) << 2) & 0x20) | (V & 0x1F));
}

static void SetCHR(uint16_t A, uint16_t V) {
	setchr1(A, V & 0x07);
}

static void PPUIRQHook(uint32_t A) {
	uint8_t bank = (A & 0x1FFF) >> 10;
	if ((m520.ppuchrbus != bank) && ((A & 0x3000) != 0x2000)) {
		m520.ppuchrbus = bank;
		VRC24_SyncPRG();
	}
}

static DECLFW(WriteVRC4CHR) {
	VRC24_Write(A, V);
	VRC24_SyncPRG();
}

static void Power(void) {
	VRC24_Power();
	SetWriteHandler(0xB000, 0xEFFF, WriteVRC4CHR);
}

void Mapper520_Init(CartInfo *info) {
	VRC24_Init(info, VRC24_VRC4, 0x04, 0x08, 0, 1);
	info->Power = Power;
	PPU_hook = PPUIRQHook;
	VRC24_pwrap = SetPRG;
	VRC24_cwrap = SetCHR;
	AddExState(StateRegs, ~0, 0, NULL);
}
