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

/* added 2020-1-28 - */
/* NES 2.0 Mapper 372 is used for a revision of the Rockman I-VI multicart (PCB ID SFC-12).
 * It is INES Mapper 045 but with one bit of outer bank register #2 working as a CHR-ROM/RAM switch.
 */

#include "mapinc.h"
#include "mmc3.h"

static struct {
	uint8_t reg[4];
	uint8_t cmd;
} m372;

static SFORMAT StateRegs[] = {
	{ m372.reg, 4, "REGS" },
	{ &m372.cmd, 1, "CMD0" },
	{ 0 }
};

static void SetPRG(uint16_t A, uint16_t V) {
	uint16_t mask = ~m372.reg[3] & 0x3F;
	uint16_t base = ((m372.reg[2] << 2) & 0x300) | m372.reg[1];

	setprg8(A, (base & ~mask) | (V & mask));
}

static void SetCHR(uint16_t A, uint16_t V) {
	uint16_t mask = 0xFF >> (~m372.reg[2] & 0x0F);
	uint16_t base = ((m372.reg[2] << 4) & 0xF00) | m372.reg[0];

	if (m372.reg[2] & 0x20) {
		setchr8r(0x10, 0);
	} else {
		setchr1(A, (base & ~mask) | (V & mask));
	}
}

static DECLFW(WriteReg) {
	if (!(m372.reg[3] & 0x40)) {
		m372.reg[m372.cmd] = V;
		m372.cmd = (m372.cmd + 1) & 0x03;
		MMC3_SyncPRG();
		MMC3_SyncCHR();
	}
}

static void Reset(void) {
	memset(&m372, 0, sizeof(m372));
	m372.reg[2] = 0x0F;
	MMC3_Reset();
}

static void Power(void) {
	memset(&m372, 0, sizeof(m372));
	m372.reg[2] = 0x0F;
	MMC3_Power();
	SetWriteHandler(0x6000, 0x7FFF, WriteReg);
}

void Mapper372_Init(CartInfo *info) {
	MMC3_Init(info, MMC3B, 0, 0);
	MMC3_cwrap = SetCHR;
	MMC3_pwrap = SetPRG;
	info->Reset = Reset;
	info->Power = Power;
	AddExState(StateRegs, ~0, 0, NULL);

	CHRRAMSIZE = 8192;
	CHRRAM = (uint8_t *)FCEU_gmalloc(CHRRAMSIZE);
	SetupCartCHRMapping(0x10, CHRRAM, CHRRAMSIZE, 1);
	AddExState(CHRRAM, CHRRAMSIZE, 0, "CHRR");
}
