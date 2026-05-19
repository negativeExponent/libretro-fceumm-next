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
} m536;

static SFORMAT StateRegs[] = {
	{ m536.reg, 2, "EXPR" },
	{ 0 }
};

static void SetPRG(uint16_t A, uint16_t V) {
	uint16_t base = ((m536.reg[0] >> 1) & 0x20) | (m536.reg[0] & 0x1F);

	if (m536.reg[0] & 0x80) {
		setprg8(A, (base << 2) | (V & 0x0F));
	} else {
		if (m536.reg[0] & 0x20)
			setprg32(0x8000, base >> 1);
		else {
			setprg16(0x8000, base);
			setprg16(0xC000, base);
		}
	}
}

static void SetCHR(uint16_t A, uint16_t V) {
	setchr8(0);
}

static DECLFW(WriteReg) {
	m536.reg[A & 0x01] = V;
	MMC3_SyncPRG();
	MMC3_SyncCHR();
}

static void Reset(void) {
	m536.reg[0] = 0x60;
	m536.reg[1] = 0x00;
	MMC3_Reset();
}

static void Power(void) {
	m536.reg[0] = 0x60;
	m536.reg[1] = 0x00;
	MMC3_Power();
	SetWriteHandler(0x6000, 0x7FFF, WriteReg);
}

void Mapper536_Init(CartInfo *info) {
	MMC3_Init(info, MMC3B, 0, 0);
	MMC3_pwrap = SetPRG;
	MMC3_cwrap = SetCHR;
	info->Power = Power;
	info->Reset = Reset;
	AddExState(StateRegs, ~0, 0, NULL);
}
