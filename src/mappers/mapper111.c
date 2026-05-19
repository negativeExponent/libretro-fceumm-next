/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2002 Xodnizel
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

/* mapper 111 - Cheapocabra board by Memblers
 * http://forums.nesdev.com/viewtopic.php?p=146039
 *
 * 512k PRG-ROM in 32k pages (flashable if battery backed is specified)
 * 32k CHR-RAM used as:
 *     2 x 8k pattern pages
 *     2 x 8k nametable pages
 *
 * Notes:
 * - CHR-RAM for nametables maps to $3000-3FFF as well, but FCEUX internally mirrors to 4k?
 */

/*
 * Ninja Ryukenden Chinese Fan Translation (Mapper 111-MMC1)
 *
 * Prior to the introduction of GTROM, Mapper 111 was assigned to a Chinese
 * fan translation of Ninja Ryukenden (Japanese Ninja Gaiden). This translation
 * uses a non-serialized version of MMC1 and supports 256KiB of CHR-ROM,
 * whereas the official MMC1 is limited to 128KiB.
 *
 * This assignment can coexist with GTROM provided the emulator recognizes that
 * the translation uses CHR-ROM and therefore must emulate the MMC1 variant
 * instead of GTROM hardware.
 */


#include "mapinc.h"
#include "flashrom.h"
#include "mmc1.h"

static struct {
	uint8_t reg;
} m111;

static uint8_t *FLASHROM = NULL;

static SFORMAT StateRegs[] = {
	{ &m111.reg, 1, "REGS" },
	{ 0 }
};

static void SetPRGBank_mmc1(uint16_t A, uint16_t V) {
	setprg16(A, V & 0x0F);
}

static void SetCHRBank_mmc1(uint16_t A, uint16_t V) {
	setchr4(A, V & 0x3F);
}

static DECLFW(WriteReg_mmc1) {
	mmc1.reg[(A >> 13) & 0x03] = V;
	MMC1_SyncPRG();
	MMC1_SyncCHR();
	MMC1_SyncMirror();
}

static void Power_mmc1(void) {
	MMC1_Power();
	SetWriteHandler(0x8000, 0xFFFF, WriteReg_mmc1);
}

#define VRAM_OFFSET(x) (0x4000 + (0x400 * (x)))

static void Sync(void) {
	/* 7  bit  0
	 * ---- ----
	 * GRNC PPPP
	 * |||| ||||
	 * |||| ++++- Select 32 KB PRG ROM bank for CPU $8000-$FFFF
	 * |||+------ Select 8 KB CHR RAM bank for PPU $0000-$1FFF
	 * ||+------- Select 8 KB nametable for PPU $2000-$3EFF
	 * |+-------- Red LED - 0=On; 1=Off
	 * +--------- Green LED - 0=On; 1=Off */
	int nt = (m111.reg & 0x20) >> 5;

	setprg32r(FLASHROM ? 0x10 : 0, 0x8000, m111.reg & 0x0F);
	setchr8((m111.reg & 0x10) >> 4);
	setntamem(CHRptr[0] + VRAM_OFFSET(0) + (nt << 13), 1, 0);
	setntamem(CHRptr[0] + VRAM_OFFSET(1) + (nt << 13), 1, 1);
	setntamem(CHRptr[0] + VRAM_OFFSET(2) + (nt << 13), 1, 2);
	setntamem(CHRptr[0] + VRAM_OFFSET(3) + (nt << 13), 1, 3);
}

static DECLFR(GetOpenBus) {
	m111.reg = cpu.openbus;
	Sync();
	return m111.reg;
}

static DECLFW(WriteReg) {
	m111.reg = V;
	Sync();
}

static DECLFR(ReadFlash) {
	return FlashROM_Read(A);
}

static DECLFW(WriteFlash) {
	FlashROM_Write(A, V);
}

static void CPUIRQHook(int a) {
	FlashROM_CPUCyle(a);
}

static void Power(void) {
	m111.reg = 0xFF;
	Sync();

	SetReadHandler(0x5000, 0x5FFF, GetOpenBus);
	SetReadHandler(0x7000, 0x7FFF, GetOpenBus);

	SetWriteHandler(0x5000, 0x5FFF, WriteReg);
	SetWriteHandler(0x7000, 0x7FFF, WriteReg);

	SetReadHandler(0x8000, 0xFFFF, ReadFlash);
	SetWriteHandler(0x8000, 0xFFFF, WriteFlash);
}

static void Close(void) {
	if (FLASHROM) {
		FCEU_free(FLASHROM);
	}
	FLASHROM = NULL;
}

static void StateRestore(int version) {
	Sync();
}

void Mapper111_Init(CartInfo *info) {
	if (ROM.chr.size) {
		MMC1_Init(info, MMC1B, FALSE, FALSE);
		info->Power = Power_mmc1;
		MMC1_pwrap = SetPRGBank_mmc1;
		MMC1_cwrap = SetCHRBank_mmc1;
		return;
	}

	info->Power = Power;
	info->Close = Close;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);

	if (info->battery) {
		int fsize = PRGsize[0];

		FLASHROM = (uint8_t *)FCEU_malloc(fsize);
		info->SaveGame[0] = FLASHROM;
		info->SaveGameLen[0] = fsize;
		AddExState(FLASHROM, fsize, 0, "FROM");
		memcpy(FLASHROM, PRGptr[0], fsize);
		SetupCartPRGMapping(0x10, FLASHROM, fsize, 0);

		FlashROM_Init(FLASHROM, fsize, 0xBF, 0xB7, 4096, 0x5555, 0x2AAA);
		MapIRQHook = CPUIRQHook;
	}
}
