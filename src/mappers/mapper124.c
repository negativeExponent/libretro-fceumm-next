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
 *
 */

/* iNES mapper 124 is assigned to the Super Game Mega Type III pirate arcade board. */

#include "mapinc.h"
#include "mmc1.h"
#include "mmc3.h"
#include "latch.h"

#define MAPPER_UNROM 0
#define MAPPER_AMROM 1
#define MAPPER_MMC1	 2
#define MAPPER_MMC3	 3

static struct {
	uint8_t reg[2];
	uint8_t mapper;
} m124;

static uint8_t dipsw;

static uint8_t audioEnable;
static uint8_t *ExRAM;

static SFORMAT StateRegs[] = {
	{ m124.reg, 2, "EXPR" },
	{ &m124.mapper, 1, "MAPR" },
	{ 0 }
};

static uint32_t GetPRGBase(void) {
	return ((m124.reg[1] << 4) & 0x1F0);
}

static uint32_t GetCHRBase(void) {
	return ((m124.reg[0] << 7) & 0x780);
}

static void SetPRG_mmc1(uint16_t A, uint16_t V) {
	setprg16(A, (GetPRGBase() >> 1) | (V & 0x07));
}

static void SetCHR_mmc1(uint16_t A, uint16_t V) {
	setchr4(A, (GetCHRBase() >> 2) | (V & 0x1F));
}

static void SyncWRAM_mmc1(void) {
	if (!(m124.reg[1] & 0x20)) {
		MMC1_SyncWRAM_default();
	}
}

static void SetPRG_mmc3(uint16_t A, uint16_t V) {
	uint16_t mask = (m124.reg[1] & 0x20) ? 0x0F : 0x1F;
	setprg8(A, GetPRGBase() | (V & mask));
}

static void SetCHR_mmc3(uint16_t A, uint16_t V) {
	uint16_t mask = (m124.reg[0] & 0x40) ? 0x7F : 0xFF;
	setchr1(A, GetCHRBase() | (V & mask));
}

static void Sync(void) {
	switch (m124.mapper) {
	case MAPPER_UNROM:
		setprg16(0x8000, (GetPRGBase() >> 1) | (latch.data & 0x07));
		setprg16(0xC000, (GetPRGBase() >> 1) | 0x07);
		setchr8(0);
		setmirror(MI_V);
		break;
	case MAPPER_AMROM:
		setprg32(0x8000, (GetPRGBase() >> 2) | (latch.data & 0x07));
		setchr8(0);
		setmirror(MI_0 + ((latch.data >> 4) & 1));
		break;
	case MAPPER_MMC1:
		MMC1_SyncWRAM();
		MMC1_SyncPRG();
		MMC1_SyncCHR();
		MMC1_SyncMirror();
		break;
	case MAPPER_MMC3:
		setprg8r(0x10, 0x6000, 0);
		MMC3_SyncPRG();
		MMC3_SyncCHR();
		MMC3_SyncMirror();
		break;
	}
	setprg4(0x5000, 0x380 + 0x05);
	if (m124.reg[1] & 0x20) {
		setprg8(0x6000, (0x380 + 0x06) >> 1);
	}
	if (!(m124.reg[1] & 0x80)) {
		setprg32(0x8000, (0x380 + 0x08) >> 3);
	}
	if (!(m124.reg[1] & 0x40)) {
		setchr8r(0x10, 0);
	}
}

static void applyMode(void) {
	m124.mapper = (m124.reg[0] >> 4) & 0x03;
}

static DECLFR(ReadRAM) {
	return ExRAM[A];
}

static DECLFW(WriteRAM) {
	ExRAM[A] = V;
}

#include "vsuni.h"
static DECLFR(ReadDIP) {
	if ((A & 0x0F) == 0x0F) {
		/* TODO: Hack to use VS' coin insert map to use as mappers coin insert trigger */
		return ((vsuni_system.coinon[0] ? 0x80 : 0x00) | (dipsw & 0x7F));
	}
	return cpu.openbus;
}

static DECLFW(WriteReg) {
	if (A & 0x10) {
		audioEnable = V;
	} else {
		m124.reg[A & 0x01] = V;
		applyMode();
		Sync();
	}
}

static DECLFW(WriteASIC) {
	switch (m124.mapper) {
	case MAPPER_UNROM:
	case MAPPER_AMROM:
		Latch_Write(A, V);
		break;
	case MAPPER_MMC3:
		MMC3_Write(A, V);
		break;
	case MAPPER_MMC1:
		MMC1_Write(A, V);
		break;
	}
}

static void HBIRQHook(void) {
	if (m124.mapper == MAPPER_MMC3) {
		MMC3_IRQHBHook();
	}
}

static void Close(void) {
	if (ExRAM) {
		FCEU_gfree(ExRAM);
		ExRAM = NULL;
	}
}

static void Reset(void) {
	dipsw++;
	Sync();
}

static void Power(void) {
	memset(m124.reg, 0, sizeof(m124.reg));
	m124.mapper = 0;
	dipsw = 0;

	MMC1_Reset();
	MMC3_Reset();
	Latch_RegReset();

	SetReadHandler(0x0800, 0x0FFF, ReadRAM);
	SetWriteHandler(0x800, 0x0FFF, WriteRAM);

	SetReadHandler(0x4F00, 0x4FFF, ReadDIP);

	SetReadHandler(0x5000, 0xFFFF, CartBR);
	SetWriteHandler(0x5000, 0x5FFF, WriteReg);

	SetWriteHandler(0x6000, 0x7FFF, CartBW);

	SetWriteHandler(0x8000, 0xFFFF, WriteASIC);

	applyMode();
	Sync();
}

static void StateRestore(int version) {
	applyMode();
	Sync();
}

void Mapper124_Init(CartInfo *info) {
	MMC1_Init(info, MMC1A, FALSE, FALSE);
	MMC1_pwrap = SetPRG_mmc1;
	MMC1_cwrap = SetCHR_mmc1;
	MMC1_SyncWRAM = SyncWRAM_mmc1;

	MMC3_Init(info, MMC3B, FALSE, FALSE);
	MMC3_pwrap = SetPRG_mmc3;
	MMC3_cwrap = SetCHR_mmc3;

	Latch_Init(info, Sync, NULL, FALSE, TRUE);

	info->Power = Power;
	info->Reset = Reset;
	info->Close = Close;
	GameHBIRQHook = HBIRQHook;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);

	WRAMSIZE = 8192;
	WRAM = (uint8_t *)FCEU_gmalloc(WRAMSIZE);
	SetupCartPRGMapping(0x10, WRAM, WRAMSIZE, TRUE);
	AddExState(WRAM, WRAMSIZE, 0, "WRAM");

	ExRAM = (uint8_t *)FCEU_gmalloc(4096);
	AddExState(ExRAM, 4096, 0, "ExRAM");

	CHRRAMSIZE = 8192;
	CHRRAM = (uint8_t *)FCEU_gmalloc(CHRRAMSIZE);
	SetupCartCHRMapping(0x10, CHRRAM, CHRRAMSIZE, TRUE);
	AddExState(CHRRAM, CHRRAMSIZE, 0, "CRAM");
}
