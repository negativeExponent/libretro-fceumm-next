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
#include "mmc3.h"
#include "vrc24.h"

#define MAPPER_MMC3 0
#define MAPPER_MMC1 2
#define MAPPER_VRC4 3

static struct {
	uint8_t reg[4];
	uint8_t mapper;
} m351;

static uint8_t dipsw;

static SFORMAT StateRegs[] = {
	{ m351.reg, 4, "EXPR" },
	{ &m351.mapper, 1, "MPPR"},

	{ 0 }
};

static uint16_t GetPRGMask(void) {
	return ((m351.reg[2] & 0x04) ? 0x0F : 0x1F);
}

static uint16_t GetPRGBase(void) {
	return (m351.reg[1] >> 1);
}

static uint16_t GetCHRMask(void) {
	if (m351.reg[2] & 0x20) {
		return 0x7F;
	}
	if (m351.reg[2] & 0x10) {
		return 0x1F;
	}
	return 0xFF;
}

static uint16_t GetCHRBase(void) {
	return (m351.reg[0] << 1);
}

static void SetPRG(uint16_t A, uint16_t V) {
	if (m351.reg[2] & 0x10) { /* NROM mode */
		uint16_t bank = GetPRGBase();
		if (m351.reg[2] & 0x08) { /* NROM-64 */
			setprg8(0x8000, bank);
			setprg8(0xA000, bank);
			setprg8(0xC000, bank);
			setprg8(0xE000, bank);
		} else {
			if (m351.reg[2] & 0x04) { /* NROM-128 */
				setprg16(0x8000, bank >> 1);
				setprg16(0xC000, bank >> 1);
			} else { /* NROM-256 */
				setprg32(0x8000, bank >> 2);
			}
		}
	} else {
		uint16_t mask = GetPRGMask();
		uint16_t base = GetPRGBase();
		switch (m351.mapper) {
		case MAPPER_MMC1:
			setprg16(A, ((base & ~mask) >> 1) | (V & (mask >> 1)));
			break;
		default:
		case MAPPER_MMC3:
		case MAPPER_VRC4:
			setprg8(A, (base & ~mask) | (V & mask));
			break;
		}
	}
}

static void SetCHR(uint16_t A, uint16_t V) {
	if (m351.reg[2] & 0x01) { /* CHR RAM mode */
		setchr8r(0x10, 0);
	} else if (m351.reg[2] & 0x40) { /* CNROM mode */
		setchr8(GetCHRBase() >> 3);
	} else {
		uint16_t mask = GetCHRMask();
		uint16_t bank = GetCHRBase();
		switch (m351.mapper) {
		case MAPPER_MMC1:
			setchr4(A, ((bank & ~mask) >> 2) | (V & (mask >> 2)));
			break;
		default:
		case MAPPER_MMC3:
		case MAPPER_VRC4:
			setchr1(A, (bank & ~mask) | (V & mask));
			break;
		}
	}
}

static void SyncMirror(void) {
	switch (m351.mapper) {
	default:
	case MAPPER_MMC3:
		MMC3_SyncMirror();
		break;
	case MAPPER_MMC1:
		MMC1_SyncMirror();
		break;
	case MAPPER_VRC4:
		VRC24_SyncMirror();
		break;
	}
}

static void Sync(void) {
	switch (m351.mapper) {
	default:
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
	case MAPPER_VRC4:
		VRC24_SyncPRG();
		VRC24_SyncCHR();
		VRC24_SyncMirror();
		break;
	}
}

static DECLFW(WriteVRC4) {
	if (A & 0x800) {
		A = (A & 0xFFF3) | ((A << 1) & 0x08) | ((A >> 1) & 0x04);
	}
	VRC24_Write(A, V);
}

static void SetMode(void) {
	MapIRQHook = NULL;
	GameHBIRQHook = NULL;
	switch (m351.reg[0] & 0x03) {
	default:
	case MAPPER_MMC3:
		m351.mapper = MAPPER_MMC3;
		GameHBIRQHook = MMC3_IRQHBHook;
		SetWriteHandler(0x8000, 0xFFFF, MMC3_Write);
		break;
	case MAPPER_MMC1:
		m351.mapper = MAPPER_MMC1;
		SetWriteHandler(0x8000, 0xFFFF, MMC1_Write);
		break;
	case MAPPER_VRC4:
		m351.mapper = MAPPER_VRC4;
		MapIRQHook = VRC24_IRQCPUHook;
		SetWriteHandler(0x8000, 0xFFFF, VRC24_Write);
		break;
	}
}

static DECLFW(WriteReg) {
	m351.reg[A & 0x03] = V;
	if ((A & 0x03) == 0) {
		SetMode();
	}
	Sync();
}

static DECLFR(ReadDIP) {
	return (cpu.openbus & ~0x07) | (dipsw & 0x07);
}

static DECLFW(WriteMirror) {
	mmc3.mirr = (V >> 3) & 0x01;
	SyncMirror();
}

static void Power(void) {
	memset(&m351, 0, sizeof(m351));

	dipsw = 0;

	MMC1_Power();
	MMC3_Reset();
	VRC24_Power();

	SetReadHandler(0x6000, 0xFFFF, CartBR);
	SetReadHandler(0x5000, 0x5FFF, ReadDIP);
	SetWriteHandler(0x4025, 0x4025, WriteMirror);
	SetWriteHandler(0x5000, 0x5FFF, WriteReg);

	SetMode();
	Sync();
}

static void Reset(void) {
	memset(&m351, 0, sizeof(m351));
	dipsw = (dipsw + 1) & 0x07;

	MMC1_Reset();
	VRC24_Reset();
	MMC3_Reset();

	FCEU_printf(" Mapper Reset! dpsw:%d\n", dipsw);

	SetMode();
	Sync();
}

static void Close(void) {
}

static void StateRestore(int version) {
	SetMode();
	Sync();
}

void Mapper351_Init(CartInfo *info) {
	int CHRRAMSIZE = info->CHRRamSize + info->CHRRamSaveSize;

	VRC24_Init(info, VRC24_VRC4, 0x04, 0x08, FALSE, TRUE);
	VRC24_pwrap = SetPRG;
	VRC24_cwrap = SetCHR;

	MMC1_Init(info, MMC1B, FALSE, FALSE);
	MMC1_pwrap = SetPRG;
	MMC1_cwrap = SetCHR;

	MMC3_Init(info, MMC3B, FALSE, FALSE);
	MMC3_pwrap = SetPRG;
	MMC3_cwrap = SetCHR;

	info->Reset = Reset;
	info->Power = Power;
	info->Close = Close;

	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, 0);

	if (ROM.chr.size) {
		size_t newsize = ROM.prg.size + ROM.chr.size;
		uint8_t *buffer;
		/* This crazy thing can map CHR-ROM into CPU address space. Allocate a
		 * combined PRG+CHR address space and treat it a second "chip". */
		buffer = (uint8_t *)FCEU_malloc(newsize);
		memcpy(buffer, ROM.prg.data, ROM.prg.size);
		memcpy(&buffer[ROM.prg.size], ROM.chr.data, ROM.chr.size);

		FCEU_free(ROM.prg.data);
		ROM.prg.size = newsize;
		ROM.prg.data = (uint8_t *)FCEU_malloc(ROM.prg.size);
		memcpy(ROM.prg.data, buffer, ROM.prg.size);
		SetupCartPRGMapping(0, ROM.prg.data, ROM.prg.size, 0);

		FCEU_free(buffer);
	}

	if (ROM.chr.size && CHRRAMSIZE) {
		CHRRAM = (uint8_t *)FCEU_gmalloc(CHRRAMSIZE);
		SetupCartCHRMapping(0x10, CHRRAM, CHRRAMSIZE, 1);
		AddExState(CHRRAM, CHRRAMSIZE, 0, "CHRR");
	}
}
