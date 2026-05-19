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
 * A98402 board, A9711, A9746 similar
 * King of Fighters 96, The (Unl), Street Fighter Zero 2 (Unl)
 *
 */

#include "mapinc.h"
#include "mmc3.h"

static struct {
	uint8_t reg;
} m187;

static SFORMAT StateRegs[] = {
	{ &m187, 1, "EXPR" },
	( 0 )
};

static void SetPRG(uint16_t A, uint16_t V) {
	if (m187.reg & 0x80) {
		uint8_t bank = m187.reg >> 1;

		if (m187.reg & 0x20) {
			setprg32(0x8000, bank >> 1);
		} else {
			setprg16(0x8000, bank);
			setprg16(0xC000, bank);
		}
	} else {
		setprg8(A, V & 0x3F);
	}
}

static void SetCHR(uint16_t A, uint16_t V) {
	setchr1(A, (A >> 4) & 0x100 | V);
}

static DECLFR(ReadProtection) {
	return cpu.openbus | 0x80;
}

static DECLFW(WriteReg) {
	if (!(A & 0x01)) {
		m187.reg = V;
		MMC3_SyncPRG();
	}
}

static void Power(void) {
	memset(&m187, 0, sizeof(m187));
	MMC3_Power();
	SetReadHandler(0x5000, 0x5FFF, ReadProtection);
	SetWriteHandler(0x5000, 0x5FFF, WriteReg);
}

void Mapper187_Init(CartInfo *info) {
	MMC3_Init(info, MMC3B, 0, 0);
	MMC3_pwrap = SetPRG;
	MMC3_cwrap = SetCHR;
	info->Power = Power;
	AddExState(StateRegs, ~0, 0, NULL);
}
