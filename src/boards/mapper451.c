/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2020
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

/* NES 2.0 Mapper 451 */
/* Uses flashrom to save high scores. */

#include "mapinc.h"
#include "mmc3.h"
#include "flashrom.h"

static uint8 *FLASHROM = NULL;
static uint32 FLASHROM_size = 0;

static void M451PW(uint32 A, uint8 V) {
    switch (A & 0xE000) {
    case 0x8000: V = 0; break;
    case 0xE000: V = 0x30; break;
    default: V &= 0x3F; break;
    }
    setprg8r(0x10, A, V);
}

static DECLFR(M451FlashRead) {
	return FlashRead(A);
}

static DECLFW(M451FlashWrite) {
	FlashWrite(A, V);
	switch (A & 0xE000) {
    case 0xA000:
        MMC3_CMDWrite(0xA000, A & 1);
        break;
    case 0xC000:
        A &= 0xFF;
	    MMC3_IRQWrite(0xC000, A - 1);
	    MMC3_IRQWrite(0xC001, 0);
	    MMC3_IRQWrite(0xE000 + (A == 0xFF ? 0 : 1), 0);
        break;
    case 0xE000:
		A = ((A << 2) & 8) | (A & 1);
		MMC3_CMDWrite(0x8000, 0x40);
		MMC3_CMDWrite(0x8001, (A << 3) | 0);
		MMC3_CMDWrite(0x8000, 0x41);
		MMC3_CMDWrite(0x8001, (A << 3) | 2);
		MMC3_CMDWrite(0x8000, 0x42);
		MMC3_CMDWrite(0x8001, (A << 3) | 4);
		MMC3_CMDWrite(0x8000, 0x43);
		MMC3_CMDWrite(0x8001, (A << 3) | 5);
		MMC3_CMDWrite(0x8000, 0x44);
		MMC3_CMDWrite(0x8001, (A << 3) | 6);
		MMC3_CMDWrite(0x8000, 0x45);
		MMC3_CMDWrite(0x8001, (A << 3) | 7);
		MMC3_CMDWrite(0x8000, 0x46);
		MMC3_CMDWrite(0x8001, 0x20 | A);
		MMC3_CMDWrite(0x8000, 0x47);
		MMC3_CMDWrite(0x8001, 0x10 | A);
        break;
	}
}

static void M451Power(void) {
	GenMMC3Power();
	SetReadHandler(0x8000, 0xFFFF, M451FlashRead);
	SetWriteHandler(0x8000, 0xFFFF, M451FlashWrite);
}

static void M451Close() {
    GenMMC3Close();
	if (FLASHROM)
		FCEU_free(FLASHROM);
	FLASHROM = NULL;
}

void Mapper451_Init(CartInfo *info) {
	uint32 w, r;
	GenMMC3_Init(info, 512, 32, 0, 0);
	info->Power = M451Power;
	info->Close = M451Close;
    pwrap = M451PW;

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
	Flash_Init(FLASHROM, FLASHROM_size, 0x37, 0x86, 65536, 0x0555, 0x02AA);
}
