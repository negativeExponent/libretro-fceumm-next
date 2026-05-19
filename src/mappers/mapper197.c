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

#include "mapinc.h"
#include "mmc3.h"

static struct {
	uint8_t reg;
} m197;

static SFORMAT StateRegs[] = {
	{ &m197.reg, 1, "REGS" },
	{ 0 }
};

static void SetPRG(uint16_t A, uint16_t V) {
	uint8_t mask = (m197.reg & 0x08) ? 0x0F : 0x1F;
	uint8_t base = 0;

	switch (iNESCart.submapper) {
	case 0:
	case 1:
	case 2:
		mask = 0x3F;
		break;
	case 3:
		base = m197.reg << 4;
		mask = (m197.reg & 0x08) ? 0x0F : 0x1F;
		break;
	}
	setprg8(A, base | (V & mask));
}

static void SyncCHR(void) {
	switch (iNESCart.submapper) {
	case 0:
		setchr2(0x0000, MMC3_GetCHRBank(0));
		setchr2(0x0800, MMC3_GetCHRBank(1));
		setchr2(0x1000, MMC3_GetCHRBank(4));
		setchr2(0x1800, MMC3_GetCHRBank(5));
		break;
	case 1:
		setchr2(0x0000, MMC3_GetCHRBank(2));
		setchr2(0x0800, MMC3_GetCHRBank(3));
		setchr2(0x1000, MMC3_GetCHRBank(6));
		setchr2(0x1800, MMC3_GetCHRBank(7));
		break;
	case 2:
		setchr2(0x0000, MMC3_GetCHRBank(0));
		setchr2(0x0800, MMC3_GetCHRBank(3));
		setchr2(0x1000, MMC3_GetCHRBank(4));
		setchr2(0x1800, MMC3_GetCHRBank(7));
		break;
	case 3:
		setchr2(0x0000, (m197.reg << 7) | MMC3_GetCHRBank(0));
		setchr2(0x0800, (m197.reg << 7) | MMC3_GetCHRBank(1));
		setchr2(0x1000, (m197.reg << 7) | MMC3_GetCHRBank(4));
		setchr2(0x1800, (m197.reg << 7) | MMC3_GetCHRBank(5));
		break;
	}
}

static DECLFW(WriteReg) {
	if (MMC3_WramIsWritable()) {
		m197.reg = V;
		MMC3_SyncPRG();
		MMC3_SyncCHR();
	}
}

static DECLFW(WriteASIC) {
	switch (A & 0xE001) {
	case 0x8001:
		switch (mmc3.cmd & 0x07) {
		case 0:
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
			mmc3.reg[mmc3.cmd & 0x07] = V;
			MMC3_SyncCHR();
			break;
		default:
			MMC3_CMDWrite(A, V);
			break;
		}
		break;
	default:
		MMC3_CMDWrite(A, V);
		break;
	}
}

static void Reset(void) {
	memset(&m197, 0, sizeof(m197));
	MMC3_SyncCHR();
	MMC3_SyncPRG();
	MMC3_SyncMirror();
}

static void Power(void) {
	memset(&m197, 0, sizeof(m197));
	MMC3_Power();
	SetWriteHandler(0x6000, 0x7FFF, WriteReg);
	SetWriteHandler(0x8000, 0x9FFF, WriteASIC);
}

void Mapper197_Init(CartInfo *info) {
	MMC3_Init(info, MMC3B, 0, 0);
	info->Power = Power;
	info->Reset = Reset;
	MMC3_SyncCHR = SyncCHR;
	MMC3_pwrap = SetPRG;
	AddExState(StateRegs, ~0, 0, NULL);
}
