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

/* NES 2.0 Mapper 467 */
/* some 72-in-1 (UNL) (47-2)*/

#include "mapinc.h"
#include "mmc3.h"

static struct {
	uint8_t reg;
} m467;

static SFORMAT StateRegs[] = {
	{ &m467.reg, 1, "EXPR" },
	{ 0 }
};

static void SetPRG(uint16_t A, uint16_t V) {
	if (m467.reg & 0x20) {
		uint8_t mask = (m467.reg & 0x40) ? 0x0F : 0x03;
		uint8_t base = m467.reg << 1;

		setprg8(A, (base & ~mask) | (V & mask));
	} else {
		setprg16(0x8000, m467.reg & 0x1F);
		setprg16(0xC000, m467.reg & 0x1F);
	}
}

static void SetCHR(void) {
	uint16_t base = (m467.reg << 2) & 0x100;

	if (m467.reg & 0x40) {
		setchr2(0x0000, base | (mmc3.reg[0] & ~0x01));
		setchr2(0x0800, base | (mmc3.reg[0] | 0x01));
		setchr2(0x1000, base | mmc3.reg[2]);
		setchr2(0x1800, base | mmc3.reg[3]);
	} else {
		setchr2(0x0000, base | (mmc3.reg[0] & ~0x03) | 0);
		setchr2(0x0800, base | (mmc3.reg[0] & ~0x03) | 1);
		setchr2(0x1000, base | (mmc3.reg[2] & ~0x03) | 2);
		setchr2(0x1800, base | (mmc3.reg[3] & ~0x03) | 3);
	}
}

static void SyncMirror(void) {
	setmirror(((m467.reg >> 7) & 0x01) ^ 0x01);
}

static DECLFW(WriteReg) {
	if ((A & 0xF000) == 0x9000) {
		m467.reg = V;
		MMC3_SyncPRG();
		MMC3_SyncCHR();
		MMC3_SyncMirror();
	} else {
		switch (A & 0xE001) {
		case 0x8000:
			mmc3.cmd = V & 0x3F;
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
}

static void Reset(void) {
	memset(&m467, 0, sizeof(m467));
	MMC3_Reset();
}

static void Power(void) {
	memset(&m467, 0, sizeof(m467));
	MMC3_Power();
	SetWriteHandler(0x8000, 0x9FFF, WriteReg);
}

void Mapper467_Init(CartInfo *info) {
	MMC3_Init(info, MMC3B, 0, 0);
	MMC3_SyncCHR = SetCHR;
	MMC3_SyncMirror = SyncMirror;
	MMC3_pwrap = SetPRG;
	info->Power = Power;
	info->Reset = Reset;
	AddExState(StateRegs, ~0, 0, NULL);
}
