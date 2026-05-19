/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2025-2026 negativeExponent
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

/* NES 2.0 Mapper 461 denotes the CM-9309 PCB, used for the 6-in-1 Wuzi Gun
 * MMC1-based multicart, consisting of the two MMC1 games Operation Wolf and
 * Mechanized Attack plus four NROM-128 games. There is one 256 KiB CHR ROM chip
 * and one 32 KiB CHR ROM chip, with the 256 KiB one being stored first in the
 * .NES file. */

#include "mapinc.h"
#include "mmc1.h"

static struct {
	uint8_t reg;
} m461;

static SFORMAT StateRegs[] = {
	{ &m461.reg, 1, "EXPR" },
	{ 0 }
};

static void SetPRG(uint16_t A, uint16_t V) {
	uint16_t mask = ((m461.reg & 0x08) ? 0x07 : 0x03);
	uint16_t base = (m461.reg & 0x0C);

	if (m461.reg & 0x04) {
		setprg16(A, (base & ~mask) | (V & mask));
	} else {
		uint16_t bank = m461.reg & 0x03;

		setprg16(0x8000, bank);
		setprg16(0xC000, bank);
	}
}

static void SetCHR(uint16_t A, uint16_t V) {
	uint16_t mask = (m461.reg &0x04) ? 0x1F : 0x07;
	uint16_t base = (m461.reg &0x04) ? ((m461.reg << 2) & 0x20) : 0x40;

	setchr4(A, (base & ~mask) | (V & mask));
}

static DECLFW(WriteReg) {
	m461.reg = A & 0xFF;
	MMC1_SyncPRG();
	MMC1_SyncCHR();
}

static void Reset(void) {
	memset(&m461, 0, sizeof(m461));
	MMC1_Reset();
}

static void Power(void) {
	memset(&m461, 0, sizeof(m461));
	MMC1_Power();
	SetWriteHandler(0x8000, 0xFFFF, WriteReg);
}

void Mapper461_Init(CartInfo *info) {
	MMC1_Init(info, MMC1A, 0, 0);
	info->Power = Power;
	info->Reset = Reset;
	AddExState(StateRegs, ~0, 0, NULL);
}
