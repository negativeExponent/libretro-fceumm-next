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
#include "mmc1.h"

static struct {
	uint32_t count;
} m105;

static SFORMAT StateRegs[] = {
	{ &m105.count, 4 | FCEUSTATE_RLSB, "IRQC" },
	{ 0 }
};

static uint32_t count_target = 0x28000000;

static void CPUIRQHook(int a) {
	while (a--) {
		if (mmc1.reg[1] & 0x10) {
			m105.count = 0;
			X6502_IRQEnd(FCEU_IQEXT);
		} else {
			if (++m105.count == count_target) {
				X6502_IRQBegin(FCEU_IQEXT);
			}
			if ((m105.count % 1789773) == 0) {
				uint32_t seconds = (count_target - m105.count) / 1789773;
				FCEU_DispMessage(RETRO_LOG_INFO, 1000, "Time left: %02i:%02i\n", seconds / 60, seconds % 60);
			}
		}
	}
}

static void SetCHRBank_mmc1(uint16_t A, uint16_t V) {
	setchr8r(0, 0);
}

static void SetPRGBank_mmc1(uint16_t A, uint16_t V) {
	if (mmc1.reg[1] & 0x08) {
		setprg16(A, 8 | (V & 0x7));
	} else {
		setprg32(0x8000, (mmc1.reg[1] >> 1) & 0x03);
	}
}

static void Power(void) {
	count_target = 0x20000000 | ((uint32_t)GameInfo->cspecial << 25);
	MMC1_Power();
}

static void Reset(void) {
	count_target = 0x20000000 | ((uint32_t)GameInfo->cspecial << 25);
	MMC1_Reset();
}

void Mapper105_Init(CartInfo *info) {
	MMC1_Init(info, MMC1B, 8, 0);
	MMC1_cwrap = SetCHRBank_mmc1;
	MMC1_pwrap = SetPRGBank_mmc1;
	MapIRQHook = CPUIRQHook;
	info->Power = Power;
	info->Reset = Reset;
	AddExState(StateRegs, ~0, 0, NULL);
}
