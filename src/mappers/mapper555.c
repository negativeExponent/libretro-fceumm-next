/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2020
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

/*
 * NES 2.0 Mapper 555 is used for the Nintendo Campus Challenge 1991 cartridge
 * (retroUSB version, which may or may not have been modified from the only
 * known copy in existence).
 */

#include "mapinc.h"
#include "mmc3.h"

#define TARGET_COUNT 0x20000000

static struct {
	uint8_t reg[2];
	uint8_t count_expired;
	uint32_t count;
	uint32_t count_target;
} m555;

static SFORMAT StateRegs[] = {
	{ m555.reg, 2, "EXPR" },
	{ &m555.count, 4 | FCEUSTATE_RLSB, "CNTR" },
	{ &m555.count_target, 4 | FCEUSTATE_RLSB, "CNTR" },
	{ &m555.count_expired, 1, "CNTE" },
	{ 0 }
};

static void SetPRG(uint16_t A, uint16_t V) {
	uint16_t mask = ((m555.reg[0] << 3) & 0x18) | 0x07;
	uint16_t base = ((m555.reg[0] << 3) & 0x20);

	setprg8(A, base | (V & mask));
}

static void SetCHR(uint16_t A, uint16_t V) {
	uint16_t base = (m555.reg[0] << 5) & 0x80;

	if ((m555.reg[0] & 0x06) == 0x02) {
		if (V & 0x40) {
			setchr1r(0x10, A, base | (V & 0x07));
		} else {
			setchr1(A, base | (V & 0xFF));
		}
	} else {
		setchr1(A, base | (V & 0x7F));
	}
}

static DECLFR(Read5000) {
	if (A & 0x800) {
		return (0x5C | (m555.count_expired ? 0x80 : 0));
	}
	return WRAM[0x2000 | (A & 0xFFF)];
}

static DECLFW(Write5000) {
	if (A & 0x800) {
		m555.reg[(A >> 10) & 0x01] = V;
		MMC3_SyncPRG();
		MMC3_SyncCHR();
	} else {
		WRAM[0x2000 | (A & 0xFFF)] = V;
	}
}

static void CPUIRQHook(int a) {
	while (a--) {
		if (!(m555.reg[0] & 0x08)) {
			m555.count = 0;
			m555.count_expired = false;
		} else {
			if (++m555.count == m555.count_target) {
				m555.count_expired = TRUE;
			}
			if ((m555.count % 1789773) == 0) {
				uint32_t seconds = (m555.count_target - m555.count) / 1789773;
				FCEU_DispMessage(RETRO_LOG_INFO, 1000, "Time left: %02i:%02i\n", seconds / 60, seconds % 60);
			}
		}
	}
}

static void Reset(void) {
	memset(&m555, 0, sizeof(m555));
	m555.count_target = TARGET_COUNT | ((uint32_t)GameInfo->cspecial << 25);
	MMC3_Reset();
}

static void Power(void) {
	memset(&m555, 0, sizeof(m555));
	m555.count_target = TARGET_COUNT | ((uint32_t)GameInfo->cspecial << 25);
	MMC3_Power();

	SetReadHandler(0x5000, 0x5FFF, Read5000);
	SetWriteHandler(0x5000, 0x5FFF, Write5000);

	setprg8r(0x10, 0x6000, 0);
	SetReadHandler(0x6000, 0x7FFF, CartBR);
	SetWriteHandler(0x6000, 0x7FFF, CartBW);
}

void Mapper555_Init(CartInfo *info) {
	MMC3_Init(info, MMC3B, 0, 0);
	MMC3_cwrap = SetCHR;
	MMC3_pwrap = SetPRG;
	info->Power = Power;
	info->Reset = Reset;
	MapIRQHook = CPUIRQHook;
	AddExState(StateRegs, ~0, 0, NULL);

	WRAMSIZE = 16 * 1024;
	WRAM = (uint8_t *)FCEU_gmalloc(WRAMSIZE);
	SetupCartPRGMapping(0x10, WRAM, WRAMSIZE, TRUE);
	AddExState(WRAM, WRAMSIZE, 0, "WRAM");

	CHRRAMSIZE = 8 * 1024;
	CHRRAM = (uint8_t *)FCEU_gmalloc(CHRRAMSIZE);
	SetupCartCHRMapping(0x10, CHRRAM, CHRRAMSIZE, TRUE);
	AddExState(CHRRAM, CHRRAMSIZE, 0, "CHRR");
}
