/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2023-2025 negativeExponent
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
	uint8 reg;
} m460;

static uint8 dipsw;

static SFORMAT StateRegs[] = {
	{ &m460.reg, 1, "REGS" },
	{ 0 }
};

static void SetPRG(uint16 A, uint16 V) {
	/* Menu selection by selectively connecting m460.reg's D7 to PRG /CE or not */
	if ((m460.reg & 0x80) && (dipsw & 0x01)) {
		unsetcpu8(A);
	} else {
		uint16 mask = 0x0F;
		uint16 base = m460.reg << 4;

		if (m460.reg & 0x38) {
			if (!(A & 0x4000)) { /* GNROM */
				uint8 A14 = (m460.reg >> 3) & 0x02;

				setprg8(A, (base & ~mask) | ((V & mask) & ~A14));
				A += 0x4000;
				setprg8(A, (base & ~mask) | ((V & mask) | A14));
			}
		} else {
			setprg8(A, (base & ~mask) | (V & mask));
		}
	}
}

static void SetCHR(uint16 A, uint16 V) {
	if (m460.reg & 0x04) {
		setchr2(0x0000, mmc3.reg[0] & 0xFE);
		setchr2(0x0800, mmc3.reg[1] | 0x01);
		setchr2(0x1000, mmc3.reg[2]);
		setchr2(0x1800, mmc3.reg[5]);
	} else {
		setchr8r(0x10, 0);
	}
}

static void SyncCHR(void) {
	if (m460.reg & 0x04) {
		setchr2(0x0000, mmc3.reg[0] & 0xFE);
		setchr2(0x0800, mmc3.reg[1] | 0x01);
		setchr2(0x1000, mmc3.reg[2]);
		setchr2(0x1800, mmc3.reg[5]);
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

static DECLFW(WriteMMC3) {
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
		case 6:
		case 7:
			MMC3_Write(A, V);
			break;
		}
		break;
	default:
		MMC3_Write(A, V);
		break;
	}
}

static void Reset(void) {
	memset(&m460, 0, sizeof(m460));
	dipsw++;
	MMC3_Reset();
}

static void Power(void) {
	memset(&m460, 0, sizeof(m460));
	dipsw = 0;
	MMC3_Power();
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

	CHRRAM = (uint8 *)FCEU_gmalloc(8192);
	SetupCartCHRMapping(0x10, CHRRAM, 8192, 1);
	AddExState(CHRRAM, 8192, 0, "CRAM");
}
