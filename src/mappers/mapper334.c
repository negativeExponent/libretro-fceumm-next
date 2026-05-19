/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
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
 *
 */

#include "mapinc.h"
#include "mmc3.h"

static struct {
	uint8_t reg[2];
} m334;

static uint8_t dipsw;

static SFORMAT StateRegs[] = {
	{ m334.reg, 2, "REGS" },
	{ 0 }
};

static void SetPRG(uint16_t A, uint16_t V) {
	setprg32(0x8000, m334.reg[0] >> 1);
}

static DECLFW(WriteReg) {
	if (MMC3_WramIsWritable()) {
		m334.reg[A & 0x01] = V;
		MMC3_SyncPRG();
	}
}

static DECLFR(ReadDIP) {
	if (A & 0x02) {
		return ((cpu.openbus & 0xFE) | (dipsw & 0x01));
	}
	return cpu.openbus;
}

static void Reset(void) {
	dipsw++;
	m334.reg[0] = 0;
	MMC3_Reset();
}

static void Power(void) {
	dipsw = 0;
	m334.reg[0] = 0;
	MMC3_Power();
	SetReadHandler(0x6000, 0x7FFF, ReadDIP);
	SetWriteHandler(0x6000, 0x7FFF, WriteReg);
}

void Mapper334_Init(CartInfo *info) {
	MMC3_Init(info, MMC3B, 0, 0);
	MMC3_pwrap = SetPRG;
	info->Power = Power;
	info->Reset = Reset;
	AddExState(StateRegs, ~0, 0, NULL);
}
