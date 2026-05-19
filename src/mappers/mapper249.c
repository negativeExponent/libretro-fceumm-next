/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
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

static struct {
	uint8_t reg;
} m249;

static SFORMAT StateRegs[] = {
	{ &m249.reg, 1, "EXPR" },
	{ 0 }
};

static uint32_t scrambleBankOrder(uint32_t V, const uint8_t *source, const uint8_t *target, uint32_t length) {
	uint32_t bank = 0;
	uint32_t bit = 0;

	for (bit = 0; bit < 8; bit++) {
		if (V & (0x01 << bit)) {
			uint32_t index = 0;

			for (index = 0; index < length; index++) {
				if (source[index] == bit) {
					break;
				}
			}
			bank |= (0x01 << (index == length ? bit : target[index]));
		}
	}
	return bank;
}

static void SetPRG(uint16_t A, uint16_t V) {
	static const uint8_t prg_pattern[4][4] = {
		{ 3, 4, 2, 1 },
		{ 4, 3, 1, 2 },
		{ 1, 2, 3, 4 },
		{ 2, 1, 4, 3 },
	};
	uint32_t bank = scrambleBankOrder(V, prg_pattern[m249.reg & 0x03], prg_pattern[(iNESCart.mapper == 249) ? 0 : 2], 4);
	setprg8(A, bank);
}

static void SetCHR(uint16_t A, uint16_t V) {
	static const uint8_t chr_pattern[8][6] = {
		{ 5, 2, 6, 7, 4, 3 },
		{ 4, 5, 3, 2, 7, 6 },
		{ 2, 3, 4, 5, 6, 7 },
		{ 6, 4, 2, 3, 7, 5 },
		{ 5, 3, 7, 6, 2, 4 },
		{ 4, 2, 5, 6, 7, 3 },
		{ 3, 6, 4, 5, 2, 7 },
		{ 2, 5, 6, 7, 3, 4 },
	};
	uint32_t bank = scrambleBankOrder(V, chr_pattern[m249.reg & 0x07], chr_pattern[(iNESCart.mapper == 249) ? 0 : 2], 6);
	setchr1(A, bank);
}

static DECLFW(WriteReg) {
	m249.reg = V;
	MMC3_SyncPRG();
	MMC3_SyncCHR();
}

static void Power(void) {
	memset(&m249, 0, sizeof(m249));
	MMC3_Power();
	SetWriteHandler(0x5000, 0x5FFF, WriteReg);
}

void Mapper249_Init(CartInfo *info) {
	MMC3_Init(info, MMC3B, 8, info->battery);
	MMC3_cwrap = SetCHR;
	MMC3_pwrap = SetPRG;
	info->Power = Power;
	AddExState(StateRegs, ~0, 0, NULL);
}
