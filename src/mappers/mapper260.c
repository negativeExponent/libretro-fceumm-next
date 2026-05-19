/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2017 CaH4e3
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

/* added on 2019-5-23 - NES 2.0 Mapper 260
 * HP10xx/HP20xx - a simplified version of FK23C mapper with pretty strict and better
 * organized banking behaviour. It seems that some 176 mapper assigned game may be
 * actually this kind of board instead but in common they aren't compatible at all,
 * the games on the regular FK23C boards couldn't run on this mapper and vice versa...
 */

static struct {
	uint8_t reg[4];
} m260;

static uint32_t dipsw;

static SFORMAT StateRegs[] = {
	{ m260.reg, 4, "REGS" },
	{ 0 }
};

static void SetPRGBank_mmc3(uint16_t A, uint16_t V) {
	uint8_t base = m260.reg[1] & 0x3F;

	switch (m260.reg[0] & 0x07) {
	case 0:
	case 1:
	case 2:
	case 3: {
		uint8_t mask = 0x1F >> ((m260.reg[0] >> 1) & 0x01);

		base <<= 1;
		setprg8(A, (base & ~mask) | (V & mask));
		break;
	}
	case 4:
		setprg16(0x8000, base);
		setprg16(0xC000, base);
		break;
	case 5:
		setprg32(0x8000, base >> 1);
		break;
	case 6:
		setprg32(0x8000, base >> 1);
		break;
	case 7:
		setprg32(0x8000, base >> 1);
		break;
	}
}

static void SetCHRBank_mmc3(uint16_t A, uint16_t V) {
	uint16_t base = m260.reg[2] & 0x7F;
	uint8_t mode = m260.reg[0] & 0x07;

	switch (m260.reg[0] & 0x07) {
	case 0:
	case 1:
	case 2:
	case 3: {
		uint16_t mask = 0xFF >> (m260.reg[0] & 0x01);

		base <<= 3;
		setchr1(A, (base & ~mask) | (V & mask));
		break;
	}
	case 4:
	case 5:
		setchr8(base);
		break;
	case 6:
	case 7: {
		uint16_t mask = (m260.reg[0] & 0x01) ? 0x03 : 0x01;

		setchr8((base & ~mask) | (m260.reg[3] & mask));
		break;
	}
	}
}

static void SyncMirror(void) {
	if (m260.reg[0] & 0x04) {
		setmirror(((m260.reg[3] >> 2) & 0x01) ^ 0x01);
	} else {
		setmirror((mmc3.mirr & 0x01) ^ 0x01);
	}
}

static DECLFR(ReadDIP) {
	return ((cpu.openbus & ~0x03) | (dipsw & 0x03));
}

static DECLFW(WriteReg) {
	if (!(m260.reg[0] & 0x80)) {
		m260.reg[A & 0x03] = V;
		MMC3_SyncPRG();
		MMC3_SyncCHR();
		MMC3_SyncMirror();
	}
}

static DECLFW(WriteLatch) {
	if (m260.reg[0] & 0x04) {
		m260.reg[3] = V;
		MMC3_SyncCHR();
		MMC3_SyncMirror();
	} else {
		MMC3_Write(A, V);
	}
}

static void Reset(void) {
	memset(&m260, 0, sizeof(m260));
	dipsw++;
	MMC3_Reset();
	MMC3_SyncPRG();
	MMC3_SyncCHR();
}

static void Power(void) {
	memset(&m260, 0, sizeof(m260));
	dipsw = 0;
	MMC3_Power();
	SetReadHandler(0x5000, 0x5FFF, ReadDIP);
	SetWriteHandler(0x5000, 0x5FFF, WriteReg);
	SetWriteHandler(0x8000, 0xFFFF, WriteLatch);
}

void Mapper260_Init(CartInfo *info) {
	MMC3_Init(info, MMC3B, 0, 0);
	MMC3_cwrap = SetCHRBank_mmc3;
	MMC3_pwrap = SetPRGBank_mmc3;
	MMC3_SyncMirror = SyncMirror;
	info->Power = Power;
	info->Reset = Reset;
	AddExState(StateRegs, ~0, 0, NULL);
}
