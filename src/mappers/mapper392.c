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

/* NES 2.0 Mapper 392 denotes the 00202650 PCB, used on an 8-in-1 multicart. It
 * uses an MMC3 clone with both CHR-ROM and CHR-RAM and an outer bank register that
 * is implemented using a GAL16v8.

 * The circuit board mounts two PRG-ROM chips each holding 512 KiB of game data
 * (U2/U3), a third PRG-ROM chip with 32 KiB of menu data (U1), as well as two 512
 * KiB CHR-ROM chips holding game graphics (U10/U4) and 8 KiB of CHR-RAM (U6). The
 * NES 2.0 file holds these chips' data in the order U2-U3-U1-U10-U4.
*/

#include "mapinc.h"
#include "mmc3.h"

static struct {
	uint8_t reg;
} m392;

static SFORMAT StateRegs[] = {
	{ &m392.reg, 1, "EXPR" },
	{ 0 }
};

static void SetPRG(uint16_t A, uint16_t V) {
	uint8_t mask = 0x0F;
	uint8_t base = m392.reg << 4;

	if (m392.reg & 0x10) {
		setprg8(A, (base & ~mask) | (V & mask));
	} else {
		setprg8(A, 0x20);
	}
}

static void SetCHR(uint16_t A, uint16_t V) {
	uint16_t mask = 0x7F;
	uint16_t base = m392.reg << 7;

	if (m392.reg & 0x10) {
		setchr1(A, (base & ~mask) | (V & mask));
	} else {
		setchr8r(0x10, 0);
	}
}

static DECLFW(WriteReg) {
	if (MMC3_WramIsWritable()) {
		CartBW(A, V);
		if (!(m392.reg & 0x10)) {
			m392.reg = V;
			MMC3_SyncPRG();
			MMC3_SyncCHR();
		}
	}
}

static void Reset(void) {
	memset(&m392, 0, sizeof(m392));
	MMC3_Reset();
}

static void Power(void) {
	memset(&m392, 0, sizeof(m392));
	MMC3_Power();
	SetWriteHandler(0x6000, 0x7FFF, WriteReg);
}

void Mapper392_Init(CartInfo *info) {
	int ws = (info->PRGRamSize + info->PRGRamSaveSize) / 1024;

	if (!ws) {
		if (info->battery) {
			ws = 8;
		}
	}

	MMC3_Init(info, MMC3B, ws ? ws : 8, info->battery);
	MMC3_cwrap = SetCHR;
	MMC3_pwrap = SetPRG;
	info->Power = Power;
	info->Reset = Reset;
	AddExState(StateRegs, ~0, 0, NULL);

	CHRRAMSIZE = 8 * 1024;
	CHRRAM = (uint8_t *)FCEU_gmalloc(CHRRAMSIZE);
	SetupCartCHRMapping(0x10, CHRRAM, CHRRAMSIZE, 1);
	AddExState(CHRRAM, CHRRAMSIZE, 0, "CHRM");
}
