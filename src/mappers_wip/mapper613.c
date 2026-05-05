/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2026 negativeExponent
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
#include "mmc1.h"

#define MAPPER_MMC3 1
#define MAPPER_MMC1 0

static struct {
	uint8_t reg;
} m613;

static SFORMAT StateRegs[] = {
	{ &m613.reg, 1, "EXPR" },
	{ 0 }
};

static void SetPRG_mmc3(uint16_t A, uint16_t V) {
	uint16_t mask = 0x0F;
	uint16_t base = m613.reg << 4;

	setprg8(A, (base & ~mask) | (V & mask));
}

static void SetCHR_mmc3(uint16_t A, uint16_t V) {
	uint16_t mask = 0x7F;
	uint16_t base = m613.reg << 7;

	setchr1(A, (base & ~mask) | (V & mask));
}

static void SetMirror_mmc3(void) {
	setmirror((mmc3.mirr & 0x01) ^ 0x01);
}

static void SetPRG_mmc1(uint16_t A, uint16_t V) {
	uint16_t mask = 0x07;
	uint16_t base = m613.reg << 3;

	setprg16(A, (base & ~mask) | (V & mask));
}

static void SetCHR_mmc1(uint16_t A, uint16_t V) {
	uint16_t mask = 0x1F;
	uint16_t base = m613.reg << 5;

	setchr4(A, (base & ~mask) | (V & mask));
}

static void SetMirror_mmc1(void) {
	if (mmc1.reg[0] & 2) {
		setmirror((mmc1.reg[0] & 1) ? MI_H : MI_V);
	} else {
		setmirror((mmc1.reg[0] & 1) ? MI_1 : MI_0);
	}
}

static void Sync(void) {
	switch (m613.reg & 0x01) {
	case MAPPER_MMC3:
		MMC3_SyncPRG();
		MMC3_SyncCHR();
		MMC3_SyncMirror();
		break;
	case MAPPER_MMC1:
		MMC1_SyncPRG();
		MMC1_SyncCHR();
		MMC1_SyncMirror();
		break;
	}
}

static void SetMode(void) {
	PPU_hook = NULL;
	MapIRQHook = NULL;
	GameHBIRQHook = NULL;
	switch (m613.reg & 0x01) {
	case MAPPER_MMC3:
		GameHBIRQHook = MMC3_IRQHBHook;
		SetWriteHandler(0x8000, 0xFFFF, MMC3_Write);
		break;
	case MAPPER_MMC1:
		SetWriteHandler(0x8000, 0xFFFF, MMC1_Write);
		break;
	}
}

static void Reset(void) {
	m613.reg++;

	if (m613.reg & 0x01) {
		MMC3_Reset();
	} else {
		MMC1_Reset();	
	}

	SetMode();
	Sync();
}

static void Power(void) {
	MMC3_Power();
	MMC1_Power();	

	SetMode();
	Sync();
}

static void StateRestore(int version) {
	SetMode();
	Sync();
}

void Mapper613_Init(CartInfo *info) {
	MMC1_Init(info, MMC1B, FALSE, FALSE);
	MMC1_pwrap = SetPRG_mmc1;
	MMC1_cwrap = SetCHR_mmc1;

	MMC3_Init(info, MMC3B, FALSE, FALSE);
	MMC3_pwrap = SetPRG_mmc3;
	MMC3_cwrap = SetCHR_mmc3;

	info->Reset = Reset;
	info->Power = Power;

	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, 0);
}
