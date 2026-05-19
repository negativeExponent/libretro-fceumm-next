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
} m475;

static SFORMAT StateRegs[] = {
	{ &m475.reg, 2, "EXPR" },
	{ 0 }
};

static void SetPRG(uint16_t A, uint16_t V) {
	if (m475.reg[1] & 1) {
		if ((m475.reg[0] & 0x03) == 0x03) {
			setprg32(0x8000, ((m475.reg[0] << 2) & 0x0C) | ((m475.reg[0] >> 2) & 0x03));
		} else {
			setprg8(A, ((m475.reg[0] << 4) & 0xF0) | (V & 0x0F));
		}
	} else {
		setprg32(0x8000, 0x10 | ((m475.reg[0] >> 4) & 0x03));
	}
}

static void SetCHR(uint16_t A, uint16_t V) {
	if (m475.reg[1] & 0x01) {
		setchr1(A, ((m475.reg[0] << 7) & 0x180) | (V & 0x7F));
	} else {
		setchr1(A, 0x200 | (V & 0x7F));
	}
}

static DECLFW(WriteExtra) {
	if (m475.reg[1] & 0x01) {
		m475.reg[0] = A & 0xFF;
	} else {
		m475.reg[0] = V;
	}
	MMC3_SyncPRG();
	MMC3_SyncCHR();
}

static void Reset(void) {
	m475.reg[0] = 0;
	m475.reg[1] ^= 1;
	MMC3_Reset();
}

static void Power(void) {
	memset(&m475, 0, sizeof(m475));
	MMC3_Power();
	SetWriteHandler(0x6000, 0x7FFF, WriteExtra);
}

void Mapper475_Init(CartInfo *info) {
	MMC3_Init(info, MMC3B, 0, 0);
	MMC3_pwrap = SetPRG;
	MMC3_cwrap = SetCHR;
	info->Power = Power;
	info->Reset = Reset;
	AddExState(StateRegs, ~0, 0, NULL);
}
