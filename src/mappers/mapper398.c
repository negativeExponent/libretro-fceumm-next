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

/* NES 2.0 Mapper 398 - PCB YY840820C
 * 1995 Super HiK 5-in-1 - 新系列米奇老鼠組合卡 (JY-048)
 */

#include "mapinc.h"
#include "vrc24.h"

static struct {
	uint8_t reg;
	uint8_t ppuchrbus;
} m398;

static SFORMAT StateRegs[] = {
	{ &m398.reg, 1, "REGS" },
	{ &m398.ppuchrbus, 1, "PPUC" },
	{ 0 }
};

static void SetPRG(uint16_t A, uint16_t V) {
	if (m398.reg & 0x80) {
		/* GNROM-like */
		setprg32(0x8000, ((m398.reg >> 5) & 0x06) | ((vrc24.chr[m398.ppuchrbus] >> 2) & 0x01));
	} else {
		setprg8(A, V & 0x0F);
	}
}

static void SetCHR(uint16_t A, uint16_t V) {
	if (m398.reg & 0x80) {
		/* GNROM-like */
		setchr8(0x40 | ((m398.reg >> 3) & 0x08) | (vrc24.chr[m398.ppuchrbus] & 0x07));
	} else {
		setchr1(A, V & 0x1FF);
	}
}

static DECLFW(WriteLatch) {
	uint8_t reg = A & 0xFF;
	if (reg != m398.reg) {
		m398.reg = A & 0xFF;
		VRC24_SyncPRG();
		VRC24_SyncCHR();
	}
	VRC24_Write(A, V);
}

static void PPUHook(uint32_t A) {
	uint8_t bank = (A & 0x1FFF) >> 10;
	if ((m398.ppuchrbus != bank) && ((A & 0x3000) != 0x2000)) {
		m398.ppuchrbus = bank;
		VRC24_SyncPRG();
		VRC24_SyncCHR();
	}
}

static void Reset(void) {
	m398.reg = 0xC0;
	VRC24_SyncPRG();
	VRC24_SyncCHR();
}

static void Power(void) {
	m398.ppuchrbus = 0;
	m398.reg = 0xC0;
	VRC24_Power();
	SetWriteHandler(0x8000, 0xFFFF, WriteLatch);
}

void Mapper398_Init(CartInfo *info) {
	VRC24_Init(info, VRC24_VRC4, 0x01, 0x02, 0, 1);
	info->Reset = Reset;
	info->Power = Power;
	PPU_hook = PPUHook;
	VRC24_pwrap = SetPRG;
	VRC24_cwrap = SetCHR;
	AddExState(StateRegs, ~0, 0, NULL);
}
