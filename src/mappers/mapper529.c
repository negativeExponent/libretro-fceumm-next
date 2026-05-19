/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2007 CaH4e3
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

/* NES 2.0 Mapper 529 - YY0807/J-2148/T-230
 * UNIF UNL-T-230
 */

#include "mapinc.h"
#include "vrc24.h"
#include "eeprom_93Cx6.h"

static uint8_t eeprom_data[256];

static void SyncPRG(void) {
	setprg16(0x8000, vrc24.prg[1]);
	setprg16(0xC000, ~0);
}

static DECLFR(ReadEEPROM) {
	return eeprom_93Cx6_read() ? 0x01 : 0x00;
}

static DECLFW(WriteEEPROM) {
	eeprom_93Cx6_write(!!(A & 0x04), !!(A & 0x02), !!(A & 0x01));
}

static void Power(void) {
	VRC24_Power();
	SetReadHandler(0x5000, 0x5FFF, ReadEEPROM);
	SetWriteHandler(0xF800, 0xFFFF, WriteEEPROM);
}

void Mapper529_Init(CartInfo *info) {
	VRC24_Init(info, VRC24_VRC4, 0x04, 0x08, 0, 1);
	VRC24_SyncPRG = SyncPRG;
	if (info->PRGRamSaveSize) {
		info->Power = Power;
		eeprom_93Cx6_init(eeprom_data, 256, 16);
		info->battery = 1;
		info->SaveGame[0] = eeprom_data;
		info->SaveGameLen[0] = 256;
	}
}
