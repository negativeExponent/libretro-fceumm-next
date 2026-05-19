/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2025-2026 negativeExponent
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
#include "flashrom.h"

#define CHIP_ROM 0
#define CHIP_WRAM 0x10
#define CHIP_FLASH 0x11

static struct {
	uint8_t reg;
} m595;

static uint8_t flash_save;
static uint8_t *flash_data;
static uint32_t flashrom_len;


static SFORMAT StateRegs[] = {
	{ &m595.reg, 1, "EXPR" },
	{ 0 }
};

static void Sync(void) {
	int chip = flash_save ? CHIP_FLASH : CHIP_ROM;

	setprg8r(CHIP_WRAM, 0x6000, 0);
	setprg16r(chip, 0x8000, (m595.reg & 0x1F));
	setprg16r(chip, 0xC000, 0xFF);
	setchr8(0);
}

static void CPUCycle(int a) {
	FlashROM_CPUCyle(a);
}

static DECLFR(ReadFlash) {
	return FlashROM_Read(A);
}

static DECLFW(WriteFlash) {
	FlashROM_Write(A, V);
}

static DECLFW(WriteReg) {
	m595.reg = ((m595.reg >> 1) | ((V << 4) & 0x10));
	Sync();
}

static void Power(void) {
	m595.reg = 0;
	Sync();

	SetReadHandler(0x6000, 0xFFFF, CartBR);
	SetWriteHandler(0x6000, 0x7FFF, CartBW);
	SetWriteHandler(0xC000, 0xFFFF, WriteReg);

	if (flash_save) {
		SetReadHandler(0x8000, 0xFFFF, ReadFlash);
		SetWriteHandler(0x8000, 0xBFFF, WriteFlash);
	}
}

static void Close(void) {
	if (flash_data) {
		FCEU_gfree(flash_data);
	}
	flash_data = NULL;
}

static void StateRestore(int version) {
	Sync();
}

void Mapper595_Init(CartInfo *info) {
	flash_save = info->battery ? TRUE : FALSE;
	info->Power = Power;
	info->Close = Close;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);

	WRAMSIZE = 8192;
	WRAM = (uint8_t *)FCEU_malloc(WRAMSIZE);
	AddExState(WRAM, WRAMSIZE, 0, "WRAM");
	SetupCartPRGMapping(CHIP_WRAM, WRAM, WRAMSIZE, TRUE);

	if (flash_save) {
		uint32_t i, ssize;
		/* Allocate memory for flash */
		ssize = ROM.prg.size;
		flash_data = (uint8_t *)FCEU_gmalloc(ssize);
		/* Copy ROM to flash data */
		for (i = 0; i < ssize; i++) {
			flash_data[i] = ROM.prg.data[i % ssize];
		}
		SetupCartPRGMapping(CHIP_FLASH, flash_data, ssize, TRUE);
		AddExState(flash_data, ssize, 0, "FLSH");
		info->SaveGame[0] = flash_data;
		info->SaveGameLen[0] = ssize;

		FlashROM_Init(flash_data, ssize, 0xBF, 0xB7, 4096, 0x5555, 0x2AAA);
		MapIRQHook = CPUCycle;
	}
}
