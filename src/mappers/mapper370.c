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

/* Mapper 370 - F600
 * Golden Mario Party II - Around the World (6-in-1 multicart)
 */

#include "mapinc.h"
#include "mmc3.h"

static struct {
	uint8_t reg;
} m370;

static uint8_t dipsw;

static SFORMAT StateRegs[] = {
	{ &m370.reg, 1, "EXPR" },
	{ 0 }
};

static void SetPRG(uint16_t A, uint16_t V) {
	uint16_t mask = m370.reg & 0x20 ? 0x0F : 0x1F;
	uint16_t base = m370.reg << 1;

	setprg8(A, (base & ~mask) | (V & mask));
}

static void SetCHR(uint16_t A, uint16_t V) {
	uint16_t mask = (m370.reg & 0x04) ? 0xFF : 0x7F;
	uint16_t base = m370.reg << 7;

	setchr1(A, (base & ~mask) | (V & mask));
}

static void SyncMirror(void) {
	if ((m370.reg & 0x07) == 1) {
		setmirrorw(
			MMC3_GetCHRBank(0) >> 7,
			MMC3_GetCHRBank(1) >> 7,
			MMC3_GetCHRBank(2) >> 7,
			MMC3_GetCHRBank(3) >> 7);
	} else {
		setmirror((mmc3.mirr & 0x01) ^ 0x01);
	}
}

static DECLFR(ReadDIP) {
	return (((dipsw << 7) & 0x80) | (cpu.openbus & 0x7F));
}

static DECLFW(WriteReg) {
	m370.reg = (A & 0xFF);
	MMC3_SyncPRG();
	MMC3_SyncCHR();
	MMC3_SyncMirror();
}

static DECLFW(WriteMMC3) {
	uint8_t oldcmd = mmc3.cmd;

	switch (A & 0xE001) {
	case 0x8000:
		mmc3.cmd = V;
		if ((oldcmd & 0x40) != (mmc3.cmd & 0x40)) {
			MMC3_SyncPRG();
		}
		if ((oldcmd & 0x80) != (mmc3.cmd & 0x80)) {
			MMC3_SyncCHR();
			MMC3_SyncMirror();
		}
		break;
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
			MMC3_SyncMirror();
			break;
		default:
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
	memset(&m370, 0, sizeof(m370));
	dipsw ^= 1;
	FCEU_printf("solderpad=%02x\n", dipsw);
	MMC3_Reset();
}

static void Power(void) {
	memset(&m370, 0, sizeof(m370));
	dipsw = 1; /* start off with the 6-in-1 menu */
	MMC3_Power();
	SetReadHandler(0x5000, 0x5FFF, ReadDIP);
	SetWriteHandler(0x5000, 0x5FFF, WriteReg);
	SetWriteHandler(0x8000, 0x9FFF, WriteMMC3);
}

void Mapper370_Init(CartInfo *info) {
	MMC3_Init(info, MMC3B, 8, 0);
	MMC3_SyncMirror = SyncMirror;
	MMC3_cwrap = SetCHR;
	MMC3_pwrap = SetPRG;
	info->Power = Power;
	info->Reset = Reset;
	AddExState(StateRegs, ~0, 0, NULL);
}
