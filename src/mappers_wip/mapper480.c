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

#include "mapinc.h"
#include "mmc3.h"

static struct {
	uint8_t reg[2];
} m480;

static SFORMAT StateRegs[] = {
	{ &m480.reg, 2, "EXPR" },
	{ 0 }
};

static void SetPRG(uint16_t A, uint16_t V) {
	uint16_t mask;

	switch (iNESCart.submapper) {
	case 1:
		mask = ((m480.reg[0] & 0x1F) == 0x1F) ? 0x1F : 0x0F;
		setprg8(A, ((m480.reg[0] << 4) & ~mask) | (V & mask));
		break;

	case 2:
		mask = ((m480.reg[0] & 0x0E) == 0x0E) ? 0x1F : 0x0F;
		if (m480.reg[0] & 0x20) {
			setprg32(0x8000, ((m480.reg[0] << 2) & ~0x03) | (m480.reg[1] & 0x03));
		} else {
			setprg8(A, ((m480.reg[0] << 4) & ~mask) | (V & mask));
		}
		break;

	default:
		mask = (m480.reg[0] & 0x20) ? 0x1F : 0x0F;
		setprg8(A, ((m480.reg[0] << 4) & ~mask) | (V & mask));
		break;
	}
}

static void SetCHR(uint16_t A, uint16_t V) {
	uint16_t mask;

	switch (iNESCart.submapper) {
	case 1:
		mask = ((m480.reg[0] & 0x1F) == 0x19) ? 0xFF : 0x7F;
		if (m480.reg[0] & 0x20) {
			setchr8r(0x10, 0);
		} else {
			setchr1(A, ((m480.reg[0] << 7) & ~mask) | (V & mask));
		}
		break;

	case 2:
		mask = 0x7F;
		if (((m480.reg[0] & 0x0F) == 0x03) || ((m480.reg[0] & 0x0F) == 0x0F)) {
			mask = 0xFF;
		}
		if (m480.reg[0] & 0x10) {
			setchr2(0x0000, 0x400 | mmc3.reg[0] & 0xFE);
			setchr2(0x0800, 0x400 | mmc3.reg[1] | 0x01);
			setchr2(0x1000, 0x400 | mmc3.reg[2]);
			setchr2(0x1800, 0x400 | mmc3.reg[5]);
		} else {
			setchr1(A, ((m480.reg[0] << 7) & ~mask) | (V & mask));
		}
		break;

	default:
		mask = ((m480.reg[0] & 0x1F) == 0x1F) ? 0x1F : 0x0F;
		setprg8(A, ((m480.reg[0] << 4) & ~mask) | (V & mask));
		break;
	}
}

static DECLFW(WriteReg) {
	if ((iNESCart.submapper == 2) && (m480.reg[0] & 0x30)) {
		m480.reg[1] = (A & 0x100) ? (A >> 4) : (((V << 1) & 0x02) | ((V >> 4) & 0x01));
		MMC3_SyncPRG();
		MMC3_SyncCHR();
	} else {
		m480.reg[0] = A & 0xFF;
		MMC3_SyncPRG();
		MMC3_SyncCHR();
	}
}

static void Power(void) {
	m480.reg[0] = 0;
	m480.reg[1] = 3;
	MMC3_Power();
	SetWriteHandler(0x6000, 0x7FFF, WriteReg);
}

static void Reset(void) {
	m480.reg[0] = 0;
	m480.reg[1] = 3;
	MMC3_Reset();
}

void Mapper480_Init(CartInfo *info) {
	uint16_t ws = 8;
	if (info->iNES2) {
		ws = (info->PRGRamSize + info->PRGRamSaveSize);
		ws /= 1024;
	}
	MMC3_Init(info, MMC3B, ws, info->battery);
	MMC3_pwrap = SetPRG;
	MMC3_cwrap = SetCHR;
	info->Power = Power;
	info->Reset = Reset;
	AddExState(StateRegs, ~0, 0, NULL);
	if (info->submapper == 1) {
		CHRRAMSIZE = 8192;
		CHRRAM = (uint8_t *)FCEU_gmalloc(CHRRAMSIZE);
		SetupCartCHRMapping(0x10, CHRRAM, CHRRAMSIZE, TRUE);
		AddExState(CHRRAM, CHRRAMSIZE, 0, "CHRR");
	}
}
