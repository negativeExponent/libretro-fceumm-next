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

/* NES 2.0 Mapper 374
 * 1995 Super HiK 4-in-1 - 新系列機器戰警组合卡 (JY-022)
 * 1996 Super HiK 4-in-1 - 新系列超級飛狼組合卡 (JY-051)
 */

#include "mapinc.h"
#include "mmc1.h"

static struct {
	uint8_t reg;
} m374;

static SFORMAT StateRegs[] = {
	{ &m374.reg, 1, "GAME" },
	{ 0 }
};

static void SetPRG(uint16_t A, uint16_t V) {
	uint16_t mask = 0x07;
	uint16_t base = m374.reg << 3; 

	setprg16(A, (base & ~mask) |  (V & mask));
}

static void SetCHR(uint16_t A, uint16_t V) {
	uint16_t mask = 0x1F;
	uint16_t base = m374.reg << 5; 

	setchr4(A, (base & ~mask) |  (V & mask));
}

static void Reset(void) {
	m374.reg = (m374.reg + 1) & 0x03;
	MMC1_Reset();
}

static void Power(void) {
	m374.reg = 0;
	MMC1_Power();
}

void Mapper374_Init(CartInfo *info) {
	MMC1_Init(info, MMC1B, 0, 0);
	MMC1_cwrap = SetCHR;
	MMC1_pwrap = SetPRG;
	info->Power = Power;
	info->Reset = Reset;
	AddExState(StateRegs, ~0, 0, NULL);
}
