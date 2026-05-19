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

/* NES 2.0 Mapper 543 - 1996 無敵智カ卡 5-in-1 (CH-501) */
/* NOTE: needs RAM to be initialized to all 0x00 */

#include "mapinc.h"
#include "mmc1.h"

static struct {
	uint8_t reg;
	uint8_t bits;
	uint8_t shift;
} m543;

static SFORMAT StateRegs[] = {
	{ &m543.bits, 1, "BITS" },
	{ &m543.shift, 1, "SHFT" },
	{ &m543.reg, 1, "REG0" },
	{ 0 }
};

static void SetPRG(uint16_t A, uint16_t V) {
	setprg16(A, (m543.reg << 4) | (V & 0x0F));
}

static void SetCHR(uint16_t A, uint16_t V) {
	setchr4(A, (V & 0x07));
}

static void SyncWRAM(void) {
	uint32_t wramBank;

	if (m543.reg & 0x02) {
		wramBank = 0x04 | ((m543.reg >> 1) & 0x02) | (m543.reg & 0x01);
	} else {
		wramBank = ((m543.reg << 1) & 0x02) | ((MMC1_GetCHRBank(0) >> 3) & 0x01);
	}
	setprg8r(0x10, 0x6000, wramBank);
}

static DECLFW(WriteReg) {
	m543.bits |= ((V >> 3) & 0x01) << m543.shift++;
	if (m543.shift == 4) {
		m543.reg = m543.bits;
		m543.bits = m543.shift = 0;
		MMC1_SyncPRG();
		MMC1_SyncCHR();
		MMC1_SyncWRAM();
	}
}

static void Reset(void) {
	memset(&m543, 0, sizeof(m543));
	MMC1_Reset();
}

static void Power(void) {
	memset(&m543, 0, sizeof(m543));
	MMC1_Power();
	SetWriteHandler(0x5000, 0x5FFF, WriteReg);
}

void Mapper543_Init(CartInfo *info) {
	MMC1_Init(info, MMC1B, 64, info->battery ? 64 : 0);
	info->Power = Power;
	info->Reset = Reset;
	MMC1_cwrap = SetCHR;
	MMC1_pwrap = SetPRG;
	MMC1_SyncWRAM = SyncWRAM;
	AddExState(StateRegs, ~0, 0, NULL);
}
