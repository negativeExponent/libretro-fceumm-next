/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2007 CaH4e3
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

/* NES 2.0 Mapper 529 - YY0807/J-2148/T-230
 * UNIF UNL-T-230
 */

#include "mapinc.h"
#include "vrc24.h"
#include "eeprom_93C66.h"

static uint8 haveEEPROM;
static uint8 eeprom_data[256];

static void UNLT230PRGSync(uint32 A, uint8 V) {
	switch (A) {
	case 0x8000:
		setprg16(0x8000, vrc24.prgreg[1]);
		break;
	case 0xC000:
		setprg16(0xC000, ~0);
		break;
	}
}

static DECLFR(UNLT230EEPROMRead) {
	if (haveEEPROM) {
		return eeprom_93C66_read() ? 0x01 : 0x00;
	}
	return 0x01;
}

static DECLFW(UNLT230Write) {
	if (A & 0x800) {
		if (haveEEPROM) {
			eeprom_93C66_write(!!(A & 0x04), !!(A & 0x02), !!(A & 0x01));
		}
	} else {
		VRC24Write(A, V);
	}
}

static void UNLT230Power(void) {
	GenVRC24Power();
	SetReadHandler(0x5000, 0x5FFF, UNLT230EEPROMRead);
	SetWriteHandler(0x8000, 0xFFFF, UNLT230Write);
}

void UNLT230_Init(CartInfo *info) {
	haveEEPROM = (info->PRGRamSaveSize & 0x100) != 0;
	GenVRC24_Init(info, VRC4e, !haveEEPROM);
	info->Power = UNLT230Power;
	vrc24.pwrap = UNLT230PRGSync;
	if (haveEEPROM) {
		eeprom_93C66_init(eeprom_data, 256, 16);
		info->battery = 1;
		info->SaveGame[0] = eeprom_data;
		info->SaveGameLen[0] = 256;
	}
}
