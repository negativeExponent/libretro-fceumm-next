/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2023
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

/* NES 2.0 Mapper 406 */
/* Uses flashrom to save high scores. */

#include "mapinc.h"
#include "mmc3.h"
#include "flashrom.h"

static uint8 *FLASHROM = NULL;
static uint32 FLASHROM_size = 0;

static uint8 submapper;

static void M406PW(uint32 A, uint8 V) {
    setprg8r(0x10, A, V & 0x3F);
}

static DECLFR(M406FlashRead) {
	return FlashRead(A);
}

static DECLFW(M406FlashWrite) {
    FlashWrite(A, V);
    if (submapper == 0) {
        A = (A & 0xFFFC) | ((A << 1) & 2) | ((A >> 1) & 1);
	} else if ((A <= 0x9000) || (A >= 0xE000)) {
        A = A ^ 0x6000;
	}
    if (A >= 0xC000) {
        MMC3_IRQWrite(A, V);
    } else {
        MMC3_CMDWrite(A, V);
    }
}

static void M406Power(void) {
	GenMMC3Power();
	SetReadHandler(0x8000, 0xFFFF, M406FlashRead);
	SetWriteHandler(0x8000, 0xFFFF, M406FlashWrite);
}

static void M406Close() {
    GenMMC3Close();
	if (FLASHROM)
		FCEU_free(FLASHROM);
	FLASHROM = NULL;
}

void Mapper406_Init(CartInfo *info) {
	uint32 w, r;
	GenMMC3_Init(info, 512, 256, 0, 0);
	info->Power = M406Power;
	info->Close = M406Close;
    mmc3.pwrap = M406PW;
    submapper = info->submapper;
	MapIRQHook = FlashCPUHook;

	FLASHROM_size = PRGsize[0];
	FLASHROM = (uint8 *)FCEU_gmalloc(FLASHROM_size);
	info->SaveGame[0]    = FLASHROM;
	info->SaveGameLen[0] = FLASHROM_size;
	AddExState(FLASHROM, FLASHROM_size, 0, "FROM");
	/* copy PRG ROM into FLASHROM, use it instead of PRG ROM */
	for (w = 0, r = 0; w < FLASHROM_size; w++) {
		FLASHROM[w] = PRGptr[0][r];
		++r;
	}
	SetupCartPRGMapping(0x10, FLASHROM, FLASHROM_size, 0);
	if (submapper == 0) {
		Flash_Init(FLASHROM, FLASHROM_size, 0xC2, 0xA4, 65536, 0x5555, 0x02AAA);
	} else {
		Flash_Init(FLASHROM, FLASHROM_size, 0x01, 0xA4, 65536, 0x5555, 0x02AAA);
	}
}
