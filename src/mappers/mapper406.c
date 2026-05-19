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
 */

/* NES 2.0 Mapper 406 */
/* Uses flashrom to save high scores. */

#include "mapinc.h"
#include "mmc3.h"
#include "flashrom.h"

static uint8_t *FLASHROM_data = NULL;
static uint32_t FLASHROM_size = 0;

static void SetPRG(uint16_t A, uint16_t V) {
	setprg8r(0x10, A, V & 0x3F);
}

static DECLFR(ReadFlash) {
	return FlashROM_Read(A);
}

static DECLFW(WriteFlash) {
	FlashROM_Write(A, V);
	if (iNESCart.submapper == 0) {
		A = (A & 0xFFFC) | ((A << 1) & 2) | ((A >> 1) & 1);
	} else if ((A <= 0x9000) || (A >= 0xE000)) {
		A = A ^ 0x6000;
	}
	MMC3_Write(A, V);
}

static void Power(void) {
	MMC3_Power();
	SetReadHandler(0x8000, 0xFFFF, ReadFlash);
	SetWriteHandler(0x8000, 0xFFFF, WriteFlash);
}

static void Close(void) {
	MMC3_Close();
	if (FLASHROM_data) {
		FCEU_free(FLASHROM_data);
	}
	FLASHROM_data = NULL;
}

void Mapper406_Init(CartInfo *info) {
	uint32_t w, r, id;

	MMC3_Init(info, MMC3B, 0, 0);
	info->Power = Power;
	info->Close = Close;
	MMC3_pwrap = SetPRG;
	MapIRQHook = FlashROM_CPUCyle;

	info->battery = 1;
	FLASHROM_size = PRGsize[0];
	FLASHROM_data = (uint8_t *)FCEU_gmalloc(FLASHROM_size);
	info->SaveGame[0] = FLASHROM_data;
	info->SaveGameLen[0] = FLASHROM_size;
	AddExState(FLASHROM_data, FLASHROM_size, 0, "FROM");
	/* copy PRG ROM into FLASHROM_data, use it instead of PRG ROM */
	for (w = 0, r = 0; w < FLASHROM_size; w++) {
		FLASHROM_data[w] = PRGptr[0][r];
		++r;
	}
	SetupCartPRGMapping(0x10, FLASHROM_data, FLASHROM_size, 0);

	id = (info->submapper == 0) ? 0xC2 : 0x01;
	FlashROM_Init(FLASHROM_data, FLASHROM_size, id, 0xA4, 65536, 0x5555, 0x02AAA);
}
