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

#include "mapinc.h"
#include "mmc3.h"

static struct {
	uint8_t cmd;
	uint8_t prg[4];
	uint8_t chr[8];
} m100;

static SFORMAT StateRegs[] = {
	{ &m100.cmd, 1, "CMD0" },
	{ m100.prg, 4, "PREG" },
	{ m100.chr, 8, "CREG" },
	{ 0 }
};

static void SyncPRG(void) {
	setprg8(0x8000, m100.prg[0]);
	setprg8(0xA000, m100.prg[1]);
	setprg8(0xC000, m100.prg[2]);
	setprg8(0xE000, m100.prg[3]);
}

static void SyncCHR(void) {
	setchr1(0x0000, m100.chr[0]);
	setchr1(0x0400, m100.chr[1]);
	setchr1(0x0800, m100.chr[2]);
	setchr1(0x0C00, m100.chr[3]);
	setchr1(0x1000, m100.chr[4]);
	setchr1(0x1400, m100.chr[5]);
	setchr1(0x1800, m100.chr[6]);
	setchr1(0x1C00, m100.chr[7]);
}

static DECLFW(WriteReg) {
	switch (A & 0xE001) {
	case 0x8000:
		m100.cmd = V;
		break;
	case 0x8001:
		switch (m100.cmd) {
		case 0x00:
			m100.chr[0] = V & 0xFE;
			m100.chr[1] = V | 0x01;
			break;
		case 0x01:
			m100.chr[2] = V & 0xFE;
			m100.chr[3] = V | 0x01;
			break;
		case 0x02:
			m100.chr[4] = V;
			break;
		case 0x03:
			m100.chr[5] = V;
			break;
		case 0x04:
			m100.chr[6] = V;
			break;
		case 0x05:
			m100.chr[7] = V;
			break;
		case 0x06:
			m100.prg[0] = V;
			break;
		case 0x07:
			m100.prg[1] = V;
			break;
		case 0x46:
			m100.prg[2] = V;
			break;
		case 0x47:
			m100.prg[1] = V;
			break;
		case 0x80:
			m100.chr[4] = V & 0xFE;
			m100.chr[5] = V | 0x01;
			break;
		case 0x81:
			m100.chr[6] = V & 0xFE;
			m100.chr[7] = V | 0x01;
			break;
		case 0x82:
			m100.chr[0] = V;
			break;
		case 0x83:
			m100.chr[1] = V;
			break;
		case 0x84:
			m100.chr[2] = V;
			break;
		case 0x85:
			m100.chr[3] = V;
			break;
		}
		break;
	}
	MMC3_SyncPRG();
	MMC3_SyncCHR();
}

static void ResetRegs(void) {
	memset(&m100, 0, sizeof(m100));

	m100.prg[0] = 0x00;
	m100.prg[1] = 0x01;
	m100.prg[2] = 0xFE;
	m100.prg[3] = 0xFF;
	m100.chr[0] = 0x00;
	m100.chr[1] = 0x01;
	m100.chr[2] = 0x02;
	m100.chr[3] = 0x03;
	m100.chr[4] = 0x04;
	m100.chr[5] = 0x05;
	m100.chr[6] = 0x06;
	m100.chr[7] = 0x07;
}

static void Reset(void) {
	ResetRegs();
	MMC3_Reset();
}

static void Power(void) {
	ResetRegs();
	MMC3_Power();
	SetWriteHandler(0x8000, 0x9FFF, WriteReg);

	if (iNESCart.trainer && ROM.misc.data) {
		if (ROM.misc.data[0] == 0x4C) {
			X6502_SetNewPC(0x7000);
		}
	}
}

void Mapper100_Init(CartInfo *info) {
	MMC3_Init(info, MMC3B, 1, 0);
	MMC3_SyncPRG = SyncPRG;
	MMC3_SyncCHR = SyncCHR;
	info->Power = Power;
	info->Reset = Reset;
	AddExState(StateRegs, ~0, 0, NULL);
}
