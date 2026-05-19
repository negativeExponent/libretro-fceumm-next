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
 */

/* NES 2.0 Mapper 345
 * UNIF BMC-L6IN1
 * New Star 6-in-1 Game Cartridge
 * https://wiki.nesdev.com/w/index.php/NES_2.0_Mapper_345
 */

#include "mapinc.h"
#include "mmc3.h"

static struct {
	uint8_t reg;
} m345;

static SFORMAT StateRegs[] = {
	{ &m345.reg, 1, "EXPR" },
	{ 0 }
};

static void SetPRG(uint16_t A, uint16_t V) {
	uint16_t base = m345.reg >> 2;
	uint16_t mask = 0x0F;

	if (!(m345.reg & 0x0C)) { /* NROM-256 */
		V = ((m345.reg << 2) & ~0x03) | ((A >> 13) & 0x03);
	}

	setprg8(A, (base & ~mask) | (V & mask));
}

static void SyncMirror(void) {
	if (m345.reg & 0x20) {
		setmirror(MI_0 + ((m345.reg & 0x10) >> 1));
	} else {
		setmirror((mmc3.mirr & 0x01) ^ 0x01);
	}
}

static DECLFW(WriteReg) {
	if (MMC3_WramIsWritable()) {
		m345.reg = V;
		MMC3_SyncPRG();
		MMC3_SyncCHR();
		MMC3_SyncMirror();
	}
}

static void Reset(void) {
	m345.reg = 0;
	MMC3_Reset();
}

static void Power(void) {
	MMC3_Power();
	SetWriteHandler(0x6000, 0x7FFF, WriteReg);
}

void Mapper345_Init(CartInfo *info) {
	MMC3_Init(info, MMC3B, 0, 0);
	MMC3_pwrap = SetPRG;
	MMC3_SyncMirror = SyncMirror;
	info->Power = Power;
	info->Reset = Reset;
	AddExState(StateRegs, ~0, 0, NULL);
}
