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

#include "mapinc.h"
#include "mmc3.h"

static struct {
	uint8_t prg[2];
	uint8_t chr[2];
} m399;

static SFORMAT StateRegs[] = {
	{ m399.prg, 2, "PREG" },
	{ m399.chr, 2, "CREG" },
	{ 0 }
};

static void SyncPRG(void) {
	if (iNESCart.submapper == 1) {
		setprg8(0x6000, 0xFE);
		setprg8(0x8000, m399.prg[0] << 1 | 0);
		setprg8(0xA000, m399.prg[0] << 1 | 1);
		setprg8(0xC000, m399.prg[1]);
		setprg8(0xE000, 0xFF);
	} else {
		setprg8(0x8000, 0x00);
		setprg8(0xA000, m399.prg[0]);
		setprg8(0xC000, m399.prg[1]);
		setprg8(0xE000, 0xFF);
	}
}

static void SyncCHR(void) {
	setchr4(0x0000, m399.chr[0]);
	setchr4(0x1000, m399.chr[1]);
}

static DECLFW(WriteReg) {
	if (A & 0x01) {
		m399.prg[V >> 7] = V;
		MMC3_SyncPRG();
	} else {
		m399.chr[V >> 7] = V;
		MMC3_SyncCHR();
	}
}

static DECLFW(WriteReg_sub1) {
	MMC3_Write(0x2000 + A, V);
}

static void Power(void) {
	memset(&m399, 0, sizeof(m399));
	MMC3_Power();
	if (iNESCart.submapper == 1) {
		SetReadHandler(0x6000, 0x7FFF, CartBR);
		SetWriteHandler(0x8000, 0xDFFF, WriteReg_sub1);
		SetWriteHandler(0xE000, 0xFFFF, WriteReg);
	} else {
		SetWriteHandler(0x8000, 0x9FFF, WriteReg);
	}
}

void Mapper399_Init(CartInfo *info) {
	MMC3_Init(info, MMC3B, 0, 0);
	MMC3_SyncPRG = SyncPRG;
	MMC3_SyncCHR = SyncCHR;
	info->Power = Power;
	AddExState(StateRegs, ~0, 0, NULL);
}
