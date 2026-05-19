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

/* NES 2.0 Mapper 362 - PCB 830506C
 * 1995 Super HiK 4-in-1 (JY-005)
 */

#include "mapinc.h"
#include "vrc24.h"

static struct {
	uint8_t game;
	uint8_t ppuchrbus;
} m362;

static SFORMAT StateRegs[] = {
	{ &m362.game, 1, "GAME" },
	{ &m362.ppuchrbus, 1, "PPUC" },
	{ 0 }
};

static void SetPRG(uint16_t A, uint16_t V) {
	uint16_t base = (m362.game == 0) ? (vrc24.chr[m362.ppuchrbus] >> 3) : 0x40;
	uint16_t mask = 0x0F;

	setprg8(A, (base & ~mask) | (V & mask));
}

static void SetCHR(uint16_t A, uint16_t V) {
	uint16_t base = (m362.game == 0) ? vrc24.chr[m362.ppuchrbus] : 0x200;
	uint16_t mask = (m362.game == 0) ? 0x7F : 0x1FF;

	setchr1(A, (base & ~mask) | (V & mask));
}

static DECLFW(WriteCHR) {
	VRC24_Write(A, V);
	if ((m362.game == 0) && (A & 0x01)) {
		/* NOTE: Because the lst higher 2 CHR-ROM bits are repurposed as PRG/CHR outer bank,
		an extra PRG sync after a CHR write. */
		VRC24_SyncPRG();
	}
}

static void HBIRQHook(uint32_t A) {
	uint8_t bank = (A & 0x1FFF) >> 10;
	if ((m362.game == 0) && (m362.ppuchrbus != bank) && ((A & 0x3000) != 0x2000)) {
		m362.ppuchrbus = bank;
		VRC24_SyncCHR();
		VRC24_SyncPRG();
	}
}

static void Reset(void) {
	if (ROM.prg.size <= (512 * 1024)) {
		m362.game = 0;
	} else {
		m362.game = (m362.game + 1) & 0x01;
	}
	VRC24_SyncCHR();
	VRC24_SyncPRG();
}

static void Power(void) {
	memset(&m362, 0, sizeof(m362));
	VRC24_Power();
	SetWriteHandler(0xB000, 0xEFFF, WriteCHR);
}

void Mapper362_Init(CartInfo *info) {
	VRC24_Init(info, VRC24_VRC4, 0x01, 0x02, 0, 0);
	VRC24_pwrap = SetPRG;
	VRC24_cwrap = SetCHR;

	info->Reset = Reset;
	info->Power = Power;
	PPU_hook = HBIRQHook;

	AddExState(StateRegs, ~0, 0, NULL);
}
