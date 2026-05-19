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
#include "cartram.h"

uint8_t *WRAM = NULL;
uint8_t *CHRRAM = NULL;

uint32_t WRAMSIZE = 0;
uint32_t CHRRAMSIZE = 0;

void CartRAM_Close(void) {
	if (WRAM) {
		FCEU_gfree(WRAM);
		WRAM = NULL;
	}
	if (CHRRAM) {
		FCEU_gfree(CHRRAM);
		CHRRAM = NULL;
	}
}

void CartRAM_Init(CartInfo *info,
                  uint8_t min_wram_kb,
                  uint8_t min_chrram_kb) {
	WRAMSIZE = GetWRAMSize(info, (uint32_t)min_wram_kb * 1024);
	if (WRAMSIZE) {
		WRAM = (uint8_t *)FCEU_gmalloc(WRAMSIZE);
		SetupCartPRGMapping(0x10, WRAM, WRAMSIZE, TRUE);
		AddExState(WRAM, WRAMSIZE, 0, "WRAM");
		if (info->battery && (info->PRGRamSaveSize || !info->iNES2)) {
			info->SaveGame[0] = WRAM;
			info->SaveGameLen[0] = info->iNES2? (uint32_t)info->PRGRamSaveSize: WRAMSIZE;
		}
	}
	CHRRAMSIZE = GetCHRRAMSize(info, (uint32_t)min_chrram_kb * 1024);
	if (ROM.chr.size == 0) {
		CHRRAMSIZE =
		    0; /* If there is no CHR-ROM, then any CHR-RAM will not be "extra"
		          and therefore will be handled by ines.c, not here. */
	}
	if (CHRRAMSIZE) {
		CHRRAM = (uint8_t *)FCEU_gmalloc(CHRRAMSIZE);
		SetupCartCHRMapping(0x10, CHRRAM, CHRRAMSIZE, TRUE);
		AddExState(CHRRAM, CHRRAMSIZE, 0, "CRAM");
		if (info->battery && (info->CHRRamSaveSize || !info->iNES2)) {
			info->SaveGame[info->SaveGameLen[0] ? 1 : 0] = CHRRAM;
			info->SaveGameLen[info->SaveGameLen[0] ? 1 : 0] =
			    info->iNES2 ? (uint32_t)info->CHRRamSaveSize : CHRRAMSIZE;
		}
	}
	FCEU_printf("cart ram, wram size : %d\n", WRAMSIZE);
}

void CHRRAM_Init(CartInfo *info, uint8_t min_chrram_kb) {
	CartRAM_Init(info, 0, min_chrram_kb);
}

void WRAM_Init(CartInfo *info, uint8_t min_wram_kb) {
	CartRAM_Init(info, min_wram_kb, 0);
}
