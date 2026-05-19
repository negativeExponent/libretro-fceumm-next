/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2016 CaH4e3
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
 * 8-in-1  Rockin' Kats, Snake, (PCB marked as "8 in 1"), similar to 12IN1,
 * but with MMC3 on board, all games are hacked the same, Snake is buggy too!
 *
 * no reset-citcuit, so selected game can be reset, but to change it you must use power
 *
 * UNIF: BMC-NEWSTAR-GRM070-8IN1
 *
 */

#include "mapinc.h"
#include "mmc3.h"

static struct {
	uint8_t reg;
} m333;

static SFORMAT StateRegs[] = {
	{ &m333.reg, 1, "EXPR" },
	{ 0 }
};

static void SetPRGBank(uint16_t A, uint16_t V) {
	uint16_t base = m333.reg << 2;
	uint16_t mask = 0x0F;

	if (m333.reg & 0x10) { /* MMC3 mode */
		setprg8(A, (base & ~mask) | (V & mask));
	} else {
		setprg32(0x8000, m333.reg);
	}
}

static void SetCHRBank(uint16_t A, uint16_t V) {
	uint16_t base = m333.reg << 5;
	uint16_t mask = 0x7F;

	setchr1(A, (base & ~mask) | (V & mask));
}

static DECLFW(WriteReg) {
	m333.reg = V;
	MMC3_SyncPRG();
	MMC3_SyncCHR();
}

static void Reset(void) {
	memset(&m333, 0, sizeof(m333));
	MMC3_Reset();
}

static void Power(void) {
	memset(&m333, 0, sizeof(m333));
	MMC3_Power();
	SetWriteHandler(0x9000, 0x9FFF, WriteReg);
	SetWriteHandler(0xB000, 0xBFFF, WriteReg);
	SetWriteHandler(0xD000, 0xDFFF, WriteReg);
	SetWriteHandler(0xF000, 0xFFFF, WriteReg);
}

void Mapper333_Init(CartInfo *info) {
	MMC3_Init(info, MMC3B, 0, 0);
	MMC3_cwrap = SetCHRBank;
	MMC3_pwrap = SetPRGBank;
	info->Power = Power;
	info->Reset = Reset;
	AddExState(StateRegs, ~0, 0, NULL);
}
