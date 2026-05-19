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

#include "mapinc.h"
#include "eeprom_24C0x.h"
#include "fcg.h"

static X24C0X eeprom = { 0 };

static void SetPRG(uint16_t A, uint16_t V) {
	setprg16(A, V & 0x0F);
}

static void SetCHR(uint16_t A, uint16_t V) {
	setchr1(A, V);
}

void Mapper016_Init(CartInfo *info) {
	switch (info->submapper) {
	case 4:
		FCG_Init(info, FCG_TYPE_FCG);
		break;
	case 5:
		FCG_Init(info, FCG_TYPE_LZ93D50);
		break;
	default:
		FCG_Init(info, FCG_TYPE_Unknown);
		break;
	}
	FCG_pwrap = SetPRG;
	FCG_cwrap = SetCHR;

	if (!info->iNES2 || info->PRGRamSaveSize) {
		WRAMSIZE = info->PRGRamSaveSize ? info->PRGRamSaveSize : 256;
		WRAM = (uint8_t *)FCEU_malloc(WRAMSIZE);
		AddExState(WRAM, WRAMSIZE, 0, "WRAM");

		eeprom_24C02_init(&eeprom, WRAM);
		eeprom_AddStateInfo(&eeprom);

		FCG_SetEeprom(&eeprom);

		info->SaveGame[0] = WRAM;
		info->SaveGameLen[0] = WRAMSIZE;
	}
}
