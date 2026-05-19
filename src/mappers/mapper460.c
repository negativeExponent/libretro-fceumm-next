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

/*
 * NES 2.0 Mapper 460 denotes the FC-29-40 and K-3101 MMC3--based multicart
 * circuit boards, used by the 13-in-1 實實在在的好朋友 快打的超値感受 and
 * Street Fighter III 40-in-1 - Games Screen Selectable multicarts. Mounting
 * both 512 KiB of CHR ROM and 8 KiB of CHR RAM, they combine the
 * functionalities of INES Mapper 197 with NROM-based 32 KiB banking.
 */

#include "mapinc.h"
#include "mmc3.h"

static struct {
	uint8_t reg;
	uint8_t dipsw;
} m460;

static SFORMAT StateRegs[] = {
	{ &m460.reg, 1, "EXPR" },
	{ &m460.dipsw, 1, "DPSW" },
	{ 0 }
};

static void SetPRG(uint16_t A, uint16_t V) {
	uint16_t mask = 0x0F;
	uint16_t base = m460.reg << 4;

	if (m460.reg & 0x08) {
		if (!(A & 0x4000)) { /* GNROM */
			uint8_t A14 = (m460.reg >> 3) & 0x02;

			setprg8(A, (base & ~mask) | ((V & mask) & ~A14));
			A += 0x4000;
			setprg8(A, (base & ~mask) | ((V & mask) | A14));
		}
	} else {
		setprg8(A, (base & ~mask) | (V & mask));
	}
}

static void SyncCHR(void) {
	if (m460.reg & 0x04) {
		setchr2(0x0000, MMC3_GetCHRBank(0));
		setchr2(0x0800, MMC3_GetCHRBank(3));
		setchr2(0x1000, MMC3_GetCHRBank(4));
		setchr2(0x1800, MMC3_GetCHRBank(7));
	} else {
		setchr8r(0x10, 0);
	}
}

static DECLFW(WriteReg) {
	if (MMC3_WramIsWritable()) {
		m460.reg = A & 0xFF;
		MMC3_SyncPRG();
		MMC3_SyncCHR();
	}
}

static DECLFR(ReadDIP) {
	if (((iNESCart.submapper == 0) && (m460.reg & 0x80)) ||
	    ((iNESCart.submapper == 1) && (m460.reg & 0x20))) {
		A = (A & ~0x03) | (m460.dipsw & 0x03);
	}
	return CartBR(A);
}

static DECLFW(WriteMMC3) {
	switch (A & 0xE001) {
	case 0x8001:
		mmc3.reg[mmc3.cmd & 0x07] = V;
		switch (mmc3.cmd & 0x07) {
		case 0:
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
			MMC3_SyncCHR();
			break;
		case 6:
		case 7:
			MMC3_SyncPRG();
			break;
		}
		break;
	default:
		MMC3_Write(A, V);
		break;
	}
}

static void Reset(void) {
	memset(&m460.reg, 0, sizeof(m460.reg));
	m460.dipsw++;
	MMC3_Reset();
}

static void Power(void) {
	memset(&m460, 0, sizeof(m460));
	MMC3_Power();
	SetReadHandler(0x8000, 0xFFFF, ReadDIP);
	SetWriteHandler(0x6000, 0x7FFF, WriteReg);
	SetWriteHandler(0x8000, 0x9FFF, WriteMMC3);
}

void Mapper460_Init(CartInfo *info) {
	MMC3_Init(info, MMC3B, 0, 0);
	MMC3_pwrap = SetPRG;
	MMC3_SyncCHR = SyncCHR;
	info->Power = Power;
	info->Reset = Reset;
	AddExState(StateRegs, ~0, 0, NULL);

	CHRRAM = (uint8_t *)FCEU_gmalloc(8192);
	SetupCartCHRMapping(0x10, CHRRAM, 8192, 1);
	AddExState(CHRRAM, 8192, 0, "CRAM");
}
