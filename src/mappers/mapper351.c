/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2023-2025 negativeExponent
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
	uint8 reg[4];
	uint8 mapper;
} m351;

static uint8 dipsw;

static SFORMAT StateRegs[] = {
	{ m351.reg, 4, "EXPR" },
	{ &m351.mapper, 1, "MPPR"},

	{ 0 }
};

static uint32 GetPRGMask(void) {
	return ((m351.reg[2] & 0x04) ? 0x0F : 0x1F);
}

static uint32 GetPRGBase(void) {
	return (m351.reg[1] >> 1);
}

static uint32 GetCHRMask(void) {
	if ((m351.reg[2] & 0x10) && !(m351.reg[2] & 0x20)) {
		return 0x1F;
	}
	return ((m351.reg[2] & 0x20) ? 0x7F : 0xFF);
}

static uint32 GetCHRBase(void) {
	return (m351.reg[0] << 1);
}

static void SetPRG_mmc1(uint16 A, uint16 V) {
	uint8 mask = GetPRGMask() >> 1;
	uint8 bank = GetPRGBase() >> 1;

	setprg16(A, (bank & ~mask) | (V & mask));
}

static void SetCHR_mmc1(uint16 A, uint16 V) {
	uint16 mask = GetCHRMask() >> 2;
	uint16 bank = GetCHRBase() >> 2;

	setchr4(A, (bank & ~mask) | (V & mask));
}

static void SetPRG_mmc3(uint16 A, uint16 V) {
	uint8 mask = GetPRGMask();
	uint8 bank = GetPRGBase();

	setprg8(A, (bank & ~mask) | (V & mask));
}

static void SetCHR_mmc3(uint16 A, uint16 V) {
	uint16 mask = GetCHRMask();
	uint16 bank = GetCHRBase();

	setchr1(A, (bank & ~mask) | (V & mask));
}

static void SetPRG_vrc4(uint16 A, uint16 V) {
	uint8 mask = GetPRGMask();
	uint8 bank = GetPRGBase();

	setprg8(A, (bank & ~mask) | (V & mask));
}

static void SetCHR_vrc4(uint16 A, uint16 V) {
	uint16 mask = GetCHRMask();
	uint16 bank = GetCHRBase();

	setchr1(A, (bank & ~mask) | (V & mask));
}

static void SyncPRG(void) {
	if (m351.reg[2] & 0x10) { /* NROM mode */
		uint32 bank = GetPRGBase();

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
		switch (m351.mapper) {
		default:
		case MAPPER_MMC3:
			MMC3_SyncPRG();
			break;
		case MAPPER_MMC1:
			MMC1_SyncPRG();
			break;
		case MAPPER_VRC4:
			VRC24_SyncPRG();
			break;
		}
	}
}

static void SyncCHR(void) {
	if (m351.reg[2] & 0x01) { /* CHR RAM mode */
		setchr8r(0x10, 0);
	} else if (m351.reg[2] & 0x40) { /* CNROM mode */
		setchr8(GetCHRBase() >> 3);
	} else {
		switch (m351.mapper) {
		default:
		case MAPPER_MMC3:
			MMC3_SyncCHR();
			break;
		case MAPPER_MMC1:
			MMC1_SyncCHR();
			break;
		case MAPPER_VRC4:
			VRC24_SyncCHR();
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
	SyncPRG();
	SyncCHR();
	SyncMirror();
}

static void SetMode(void) {
	switch (m351.reg[0] & 0x03) {
	default:
	case MAPPER_MMC3:
		m351.mapper = MAPPER_MMC3;
		break;
	case MAPPER_MMC1:
		m351.mapper = MAPPER_MMC1;
		break;
	case MAPPER_VRC4:
		m351.mapper = MAPPER_VRC4;
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

static DECLFW(WriteVRC4) {
	if (A & 0x800) {
		A = (A & 0xFFF3) | ((A << 1) & 0x08) | ((A >> 1) & 0x04);
	}
	VRC24_Write(A, V);
}

static DECLFW(WriteASIC) {
	switch (m351.mapper) {
	case MAPPER_MMC1:
		MMC1_Write(A, V);
		break;
	case MAPPER_MMC3:
		MMC3_Write(A, V);
		break;
	case MAPPER_VRC4:
		WriteVRC4(A, V);
		break;
	}
}

static void CPUIRQHook(int a) {
	if (m351.mapper == MAPPER_VRC4) {
		VRC24_IRQCPUHook(a);
	}
}

static void HBIRQHook(void) {
	if (m351.mapper == MAPPER_MMC3) { /* MMC3 mode */
		MMC3_IRQHBHook();
	}
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
	SetWriteHandler(0x8000, 0xFFFF, WriteASIC);

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
	VRC24_pwrap = SetPRG_vrc4;
	VRC24_cwrap = SetCHR_vrc4;

	MMC1_Init(info, MMC1B, FALSE, FALSE);
	MMC1_pwrap = SetPRG_mmc1;
	MMC1_cwrap = SetCHR_mmc1;

	MMC3_Init(info, MMC3B, FALSE, FALSE);
	MMC3_pwrap = SetPRG_mmc3;
	MMC3_cwrap = SetCHR_mmc3;

	info->Reset = Reset;
	info->Power = Power;
	info->Close = Close;

	MapIRQHook = CPUIRQHook;
	GameHBIRQHook = HBIRQHook;

	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, 0);

	if (ROM.chr.size) {
		uint32 newsize = ROM.prg.size + ROM.chr.size;
		uint8 *buffer;
		/* This crazy thing can map CHR-ROM into CPU address space. Allocate a
		 * combined PRG+CHR address space and treat it a second "chip". */
		buffer = (uint8 *)FCEU_malloc(newsize);
		memcpy(buffer, ROM.prg.data, ROM.prg.size);
		memcpy(&buffer[ROM.prg.size], ROM.chr.data, ROM.chr.size);

		FCEU_free(ROM.prg.data);
		ROM.prg.size = newsize;
		ROM.prg.data = (uint8 *)FCEU_malloc(ROM.prg.size);
		memcpy(ROM.prg.data, buffer, ROM.prg.size);
		SetupCartPRGMapping(0, ROM.prg.data, ROM.prg.size, 0);

		FCEU_free(buffer);
	}

	if (ROM.chr.size && CHRRAMSIZE) {
		CHRRAM = (uint8 *)FCEU_gmalloc(CHRRAMSIZE);
		SetupCartCHRMapping(0x10, CHRRAM, CHRRAMSIZE, 1);
		AddExState(CHRRAM, CHRRAMSIZE, 0, "CHRR");
	}
}
