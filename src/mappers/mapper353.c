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

/* NES 2.0 Mapper 353 is used for the 92 Super Mario Family multicart,
 * consisting of an MMC3 clone ASIC together with a PAL.
 * The PCB code is 81-03-05-C.
 */

#include "mapinc.h"
#include "mmc3.h"
#include "fdssound.h"

static struct {
	uint8_t reg;
} m353;

static SFORMAT StateRegs[] = {
	{ &m353.reg, 1, "REGS" },
	{ 0 }
};

static void SetPRG(uint16_t A, uint16_t V) {
	uint16_t base = m353.reg << 5;
	uint16_t mask = 0x1F;

	if (m353.reg == 2) {
		base |= ((mmc3.reg[0] >> 3) & 0x10);
		mask = 0x0F;
	} else if ((m353.reg == 3) && !(mmc3.reg[0] & 0x80) && (A & 0x4000)) {
		base = 0x70;
		mask = 0x0F;
		V = mmc3.reg[A >> 13];
	}

	setprg8(A, (base & ~mask) | (V & mask));
}

static void SetCHR(uint16_t A, uint16_t V) {
	uint16_t mask = 0x7F;
	uint16_t base = m353.reg << 7;

	if ((m353.reg == 2) && (mmc3.reg[0] & 0x80)) {
		setchr8r(0x10, 0);
	} else {
		setchr1(A, (base & ~mask) | (V & mask));
	}
}

static void SyncMirror(void) {
	if (m353.reg == 0) {
		setmirrorw(
			MMC3_GetCHRBank(0) >> 7,
			MMC3_GetCHRBank(1) >> 7,
			MMC3_GetCHRBank(2) >> 7,
			MMC3_GetCHRBank(3) >> 7);
	} else {
		setmirror((mmc3.mirr & 0x01) ^ 0x01);
	}
}

static DECLFW(WriteReg) {
	if (A & 0x80) {
		m353.reg = (A >> 13) & 0x03;
		MMC3_SyncPRG();
		MMC3_SyncCHR();
		MMC3_SyncMirror();
	} else {
		uint8_t oldcmd = mmc3.cmd;

		switch (A & 0xE001) {
		case 0x8000:
			mmc3.cmd = V;
			if ((oldcmd & 0x40) != (mmc3.cmd & 0x40)) {
				MMC3_SyncPRG();
			}
			if ((oldcmd & 0x80) != (mmc3.cmd & 80)) {
				MMC3_SyncCHR();
				MMC3_SyncMirror();
			}
			break;
		case 0x8001:
			mmc3.reg[mmc3.cmd & 0x07] = V;
			switch (mmc3.cmd & 0x07) {
			case 0:
				MMC3_SyncPRG();
				MMC3_SyncCHR();
				MMC3_SyncMirror();
				break;
			case 1:
			case 2:
			case 3:
			case 4:
			case 5:
				MMC3_SyncCHR();
				MMC3_SyncMirror();
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
}

static void Power(void) {
	memset(&m353, 0, sizeof(m353));
	FDSSound_Power();
	MMC3_Power();
	SetWriteHandler(0x8000, 0xFFFF, WriteReg);
}

static void Reset(void) {
	memset(&m353, 0, sizeof(m353));
	MMC3_Reset();
	FDSSoundRegReset();
	FDSSound_SC();
}

static void Close(void) {
	MMC3_Close();
}

void Mapper353_Init(CartInfo *info) {
	MMC3_Init(info, MMC3B, 8, info->battery);
	MMC3_cwrap = SetCHR;
	MMC3_pwrap = SetPRG;
	MMC3_SyncMirror = SyncMirror;
	info->Power = Power;
	info->Close = Close;
	info->Reset = Reset;
	AddExState(StateRegs, ~0, 0, NULL);

	CHRRAMSIZE = 8192;
	CHRRAM = (uint8_t *)FCEU_gmalloc(CHRRAMSIZE);
	SetupCartCHRMapping(0x10, CHRRAM, CHRRAMSIZE, 1);
	AddExState(CHRRAM, CHRRAMSIZE, 0, "CHRR");
}
