/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
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

#include "mapinc.h"
#include "mmc3.h"

static struct {
	uint8_t reg;
} m291;

static SFORMAT StateRegs[] = {
	{ &m291.reg, 1, "REGS" },
	{ 0 }
};

static void SetCHRBank(uint16_t A, uint16_t V) {
	setchr1(A, ((m291.reg << 2) & 0x100) | (V & 0xFF));
}

static void SetPRGBank(uint16_t A, uint16_t V) {
	if (m291.reg & 0x20) {
		uint16_t bank = ((m291.reg >> 4) & 0x04) | ((m291.reg >> 1) & 0x03);

		setprg32(0x8000, bank);
	} else {
		uint16_t mask = 0x0F;
		uint16_t base = m291.reg >> 2;

		setprg8(A, (base & ~mask) | (V & mask));
	}
}

static DECLFW(WriteReg) {
	/* The Outer Bank Register responds even when the MMC3 clone's WRAM bit is clear. */
	m291.reg = V;
	MMC3_SyncPRG();
	MMC3_SyncCHR();
}

static void Reset(void) {
	m291.reg = 0;
	MMC3_Reset();
}

static void Power(void) {
	m291.reg = 0;
	MMC3_Power();
	SetWriteHandler(0x6000, 0x7FFF, WriteReg);
}

void Mapper291_Init(CartInfo *info) {
	MMC3_Init(info, MMC3B, 0, 0);
	MMC3_cwrap = SetCHRBank;
	MMC3_pwrap = SetPRGBank;
	info->Power = Power;
	info->Reset = Reset;
	AddExState(StateRegs, ~0, 0, NULL);
}
