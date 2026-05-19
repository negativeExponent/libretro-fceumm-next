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

/* Famicom jump 2:
 * 0-7: Lower bit of data selects which 256KB PRG block is in use.
 * This seems to be a hack on the developers' part, so I'll make emulation
 * of it a hack(I think the current PRG block would depend on whatever the
 * lowest bit of the CHR bank switching register that corresponds to the
 * last CHR address read).
 */

 #include "mapinc.h"
#include "eeprom_24C0x.h"
#include "fcg.h"

 static struct {
	uint8_t reg;
} m153;

static SFORMAT StateRegs[] = {
	{ &m153.reg, 1, "EXPR" },
	{ 0 }
};

static void SetPRG(uint16_t A, uint16_t V) {
	setprg16(A, (m153.reg << 4) | (V & 0x0F));
}

static void SetCHR(uint16_t A, uint16_t V) {
	setchr8(0);
}

static DECLFW(WriteReg) {
	if ((A & 0x0F) <= 0x03) {
		m153.reg = V;
		FCG_SyncPRG();
	}
	FCG_Write(A, V);
}

static void Power(void) {
	FCG_Power();
	SetWriteHandler(0x8000, 0xFFFF, WriteReg);
	if (WRAMSIZE) {
		SetReadHandler(0x6000, 0x7FFF, CartBR);
		SetWriteHandler(0x6000, 0x7FFF, CartBW);
		FCEU_CheatAddRAM(WRAMSIZE >> 10, 0x6000, WRAM);
	}
}

void Mapper153_Init(CartInfo *info) {
	FCG_Init(info, FCG_TYPE_LZ93D50);
	info->Power = Power;
	FCG_pwrap = SetPRG;
	FCG_cwrap = SetCHR;
	AddExState(StateRegs, ~0, 0, NULL);

	WRAMSIZE = 8192;
	if (info->iNES2) {
		WRAMSIZE = info->PRGRamSize + info->PRGRamSaveSize;
	}
	if (WRAMSIZE) {
		WRAM = (uint8_t *)FCEU_gmalloc(WRAMSIZE);
		SetupCartPRGMapping(0x10, WRAM, WRAMSIZE, 1);
		AddExState(WRAM, WRAMSIZE, 0, "WRAM");
		if (info->battery) {
			info->SaveGame[0] = WRAM;
			info->SaveGameLen[0] = WRAMSIZE;
		}
	}
}
