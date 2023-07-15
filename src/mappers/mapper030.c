/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2014 CaitSith2, 2022 Cluster
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

/*
 * Roms still using NES 1.0 format should be loaded as 8K CHR RAM.
 * Roms defined under NES 2.0 should use the VRAM size field, defining 7, 8 or 9, based on how much VRAM should be
 * present. UNIF doesn't have this problem, because unique board names can define this information. The UNIF names are
 * UNROM-512-8K, UNROM-512-16K and UNROM-512-32K
 *
 * The battery flag in the NES header enables flash,  Mirrror mode 2 Enables MI_0 and MI_1 mode.
 * Known games to use this board are:
 *    Battle Kid 2: Mountain of Torment (512K PRG, 8K CHR RAM, Horizontal Mirroring, Flash disabled)
 *    Study Hall (128K PRG (in 512K flash chip), 8K CHR RAM, Horizontal Mirroring, Flash enabled)
 *    Nix: The Paradox Relic (512 PRG, 8K CHR RAM, Vertical Mirroring, Flash enabled)
 * Although Xmas 2013 uses a different board, where LEDs can be controlled (with writes to the $8000-BFFF space),
 * it otherwise functions identically.
  */

#include "mapinc.h"
#include "latch.h"

#define ROM_CHIP          0x00
#define CFI_CHIP          0x10
#define FLASH_CHIP        0x11
#define FLASH_SECTOR_SIZE (4 * 1024)

static uint8 flash_save, flash_state, flash_id_mode;
static uint8 *flash_data;
static uint16 flash_buffer_a[10];
static uint8 flash_buffer_v[10];
static uint8 flash_id[2];
static uint8 submapper;

static void M030_Sync() {
	int chip;
	if (flash_save) {
		chip = !flash_id_mode ? FLASH_CHIP : CFI_CHIP;
	} else {
		chip = ROM_CHIP;
	}
	setprg16r(chip, 0x8000, latch.data & 0x1F);
	setprg16r(chip, 0xc000, ~0);
	setchr8((latch.data >> 5) & 3);
	switch (submapper) {
	case 1:
		/* Mega Man II (30th Anniversary Edition) */
		setmirror((latch.data >> 7) & 1);	
		break;
	default:
		setmirror(MI_0 + ((latch.data >> 7) & 1));
		break;
	}
}

static void StateRestore(int version) {
	M030_Sync();
}

static DECLFW(M030FlashWrite) {
	int i, offset, sector;
	if (flash_state < sizeof(flash_buffer_a) / sizeof(flash_buffer_a[0])) {
		flash_buffer_a[flash_state] = (A & 0x3FFF) | ((latch.data & 1) << 14);
		flash_buffer_v[flash_state] = V;
		flash_state++;

		/* enter flash ID mode */
		if ((flash_state == 2) && (flash_buffer_a[0] == 0x5555) && (flash_buffer_v[0] == 0xAA) &&
		    (flash_buffer_a[1] == 0x2AAA) && (flash_buffer_v[1] == 0x55) && (flash_buffer_a[1] == 0x5555) &&
		    (flash_buffer_v[1] == 0x90)) {
			flash_id_mode = 0;
			flash_state   = 0;
		}

		/* erase sector */
		if ((flash_state == 6) && (flash_buffer_a[0] == 0x5555) && (flash_buffer_v[0] == 0xAA) &&
		    (flash_buffer_a[1] == 0x2AAA) && (flash_buffer_v[1] == 0x55) && (flash_buffer_a[2] == 0x5555) &&
		    (flash_buffer_v[2] == 0x80) && (flash_buffer_a[3] == 0x5555) && (flash_buffer_v[3] == 0xAA) &&
		    (flash_buffer_a[4] == 0x2AAA) && (flash_buffer_v[4] == 0x55) && (flash_buffer_v[5] == 0x30)) {
			offset = &Page[A >> 11][A] - flash_data;
			sector = offset / FLASH_SECTOR_SIZE;
			for (i = sector * FLASH_SECTOR_SIZE; i < (sector + 1) * FLASH_SECTOR_SIZE; i++) {
				flash_data[i % PRGsize[ROM_CHIP]] = 0xFF;
			}
			FCEU_printf("Flash sector #%d is erased (0x%08x - 0x%08x).\n", sector, offset, offset + FLASH_SECTOR_SIZE);
		}

		/* erase chip */
		if ((flash_state == 6) && (flash_buffer_a[0] == 0x5555) && (flash_buffer_v[0] == 0xAA) &&
		    (flash_buffer_a[1] == 0x2AAA) && (flash_buffer_v[1] == 0x55) && (flash_buffer_a[2] == 0x5555) &&
		    (flash_buffer_v[2] == 0x80) && (flash_buffer_a[3] == 0x5555) && (flash_buffer_v[3] == 0xAA) &&
		    (flash_buffer_a[4] == 0x2AAA) && (flash_buffer_v[4] == 0x55) && (flash_buffer_a[4] == 0x5555) &&
		    (flash_buffer_v[4] == 0x10)) {
			memset(flash_data, 0xFF, PRGsize[ROM_CHIP]);
			FCEU_printf("Flash chip erased.\n");
			flash_state = 0;
		}

		/* write byte */
		if ((flash_state == 4) && (flash_buffer_a[0] == 0x5555) && (flash_buffer_v[0] == 0xAA) &&
		    (flash_buffer_a[1] == 0x2AAA) && (flash_buffer_v[1] == 0x55) && (flash_buffer_a[2] == 0x5555) &&
		    (flash_buffer_v[2] == 0xA0)) {
			offset = &Page[A >> 11][A] - flash_data;
			if (CartBR(A) != 0xFF) {
				FCEU_PrintError("Error: can't write to 0x%08x, flash sector is not erased.\n", offset);
			} else {
				CartBW(A, V);
			}
			flash_state = 0;
		}
	}

	/* not a command */
	if (((A & 0xFFF) != 0x0AAA) && ((A & 0xFFF) != 0x0555)) {
		flash_state = 0;
	}

	/* reset */
	if (V == 0xF0) {
		flash_state   = 0;
		flash_id_mode = 0;
	}

	M030_Sync();
}

static void M030LatchPower(void) {
	Latch_Power();
	if (flash_save) {
		SetWriteHandler(0x8000, 0xBFFF, M030FlashWrite);
	}
}

static void M030LatchClose(void) {
	Latch_Close();
	if (flash_data) {
		FCEU_gfree(flash_data);
	}
	flash_data = NULL;
}

void Mapper030_Init(CartInfo *info) {
	Latch_Init(info, M030_Sync, NULL, 0, !info->battery);

	info->Power      = M030LatchPower;
	info->Close      = M030LatchClose;
	GameStateRestore = StateRestore;

	submapper = (info->iNES2 && info->submapper) ? info->submapper : ((info->PRGCRC32 == 0x891C14BC) ? 0x01 : 0x00);

	if (!(submapper & 1)) {
		switch (info->mirror2bits) {
		case 0: /* hard horizontal, internal */
			SetupCartMirroring(MI_H, 1, NULL);
			break;
		case 1: /* hard vertical, internal */
			SetupCartMirroring(MI_V, 1, NULL);
			break;
		case 2: /* switchable 1-screen, internal (flags: 4-screen + horizontal) */
			SetupCartMirroring(MI_0, 0, NULL);
			break;
		case 3: /* hard four screen, last 8k of 32k RAM (flags: 4-screen + vertical) */
			SetupCartMirroring(4, 1, ROM.chr.data + (info->CHRRamSize - 8192));
			break;
		}
	}

	flash_state   = 0;
	flash_id_mode = 0;
	flash_save    = info->battery;

	if (flash_save) {
		uint32 i;
		/* Allocate memory for flash */
		flash_data = (uint8 *)FCEU_gmalloc(PRGsize[ROM_CHIP]);
		/* Copy ROM to flash data */
		for (i = 0; i < PRGsize[ROM_CHIP]; i++) {
			flash_data[i] = PRGptr[ROM_CHIP][i % PRGsize[ROM_CHIP]];
		}
		SetupCartPRGMapping(FLASH_CHIP, flash_data, PRGsize[ROM_CHIP], 1);
		info->SaveGame[0]    = flash_data;
		info->SaveGameLen[0] = PRGsize[ROM_CHIP];

		flash_id[0] = 0xBF;
		flash_id[1] = 0xB5 + (ROM.prg.size >> 4);
		SetupCartPRGMapping(CFI_CHIP, flash_id, sizeof(flash_id), 0);

		AddExState(flash_data, PRGsize[ROM_CHIP], 0, "FLSH");
		AddExState(&flash_state, 1, 0, "FLST");
		AddExState(&flash_id_mode, 1, 0, "FLMD");
		AddExState(flash_buffer_a, sizeof(flash_buffer_a), 0, "FLBA");
		AddExState(flash_buffer_v, sizeof(flash_buffer_v), 0, "FLBV");
	}
}
