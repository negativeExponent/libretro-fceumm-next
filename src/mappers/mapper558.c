/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2002 Xodnizel
 *  Copyright (C) 2005 CaH4e3
 *  Copyright (C) 2019 Libretro Team
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

/* Yancheng YC-03-09/Waixing FS??? PCB */

#include "mapinc.h"

#include "eeprom_93Cx6.h"

static struct {
	uint8_t reg[4];
} m558;

static uint8_t haveEEPROM;
static uint8_t eeprom_data[512];

static SFORMAT StateRegs[] = {
	{ m558.reg, 4, "REGS" },
	{ 0 }
};

static void Sync(void) {
	setprg32(0x8000, (m558.reg[1] << 4) | (m558.reg[0] & 0xF) | ((m558.reg[3] & 0x04) ? 0x00 : 0x03));
	setprg8r(0x10, 0x6000, 0);
	if (!(m558.reg[0] & 0x80)) {
		setchr8(0);
	}
}

static DECLFR(Read5) {
	if (haveEEPROM) {
		return eeprom_93Cx6_read() ? 0x04 : 0x00;
	}
	return m558.reg[2] & 0x04;
}

static DECLFW(Write5) {
	switch (A & 0xFF00) {
	case 0x5000:
		if (!(m558.reg[3] & 0x02)) {
			V = (V & ~0x03) | ((V >> 1) & 0x01) | ((V << 1) & 0x02);
		}
		m558.reg[0] = V;
		Sync();
		break;
	case 0x5100:
		if (!(m558.reg[3] & 0x02)) {
			V = (V & ~0x03) | ((V >> 1) & 0x01) | ((V << 1) & 0x02);
		}
		m558.reg[1] = V;
		Sync();
		break;
	case 0x5200:
		if ((ROM.prg.size != (1024 * 1024)) && !(m558.reg[3] & 0x02)) {
			V = (V & ~0x03) | ((V >> 1) & 0x01) | ((V << 1) & 0x02);
		}
		m558.reg[2] = V;
		if (haveEEPROM) {
			eeprom_93Cx6_write((m558.reg[2] & 0x04), (m558.reg[2] & 0x02), (m558.reg[2] & 0x01));
		}
		break;
	case 0x5300:
		m558.reg[3] = V;
		Sync();
		break;
	}
}

static void HBIRQHook(void) {
	if ((m558.reg[0] & 0x80) && (scanline < 239)) { /* Actual hardware cannot look at the current scanline number, but instead latches PA09 on
												PA13 rises. This does not seem possible with the current PPU emulation however. */
		setchr4(0x0000, (scanline >= 127) ? 1 : 0);
		setchr4(0x1000, (scanline >= 127) ? 1 : 0);
	} else {
		setchr8(0);
	}
}

static void Reset(void) {
	memset(m558.reg, 0, sizeof(m558.reg));
	Sync();
}

static void Power(void) {
	memset(&m558, 0, sizeof(m558));
	Sync();
	SetReadHandler(0x5000, 0x57FF, Read5);
	SetWriteHandler(0x5000, 0x57FF, Write5);
	SetReadHandler(0x6000, 0xFFFF, CartBR);
	SetWriteHandler(0x6000, 0x7FFF, CartBW);
}

static void StateRestore(int version) {
	Sync();
}

void Mapper558_Init(CartInfo *info) {
	info->Power = Power;
	info->Reset = Reset;
	GameHBIRQHook = HBIRQHook;

	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);

	WRAMSIZE = info->PRGRamSize + (info->PRGRamSaveSize & ~0x7FF);
	WRAM = (uint8_t *)FCEU_gmalloc(WRAMSIZE);
	SetupCartPRGMapping(0x10, WRAM, WRAMSIZE, 1);
	AddExState(WRAM, WRAMSIZE, 0, "WRAM");
	FCEU_CheatAddRAM(WRAMSIZE >> 10, 0x6000, WRAM);

	haveEEPROM = !!(info->PRGRamSaveSize & 0x200);
	if (haveEEPROM) {
		eeprom_93Cx6_init(eeprom_data, 512, 8);
		info->battery = 1;
		info->SaveGame[0] = eeprom_data;
		info->SaveGameLen[0] = 512;
	} else if (info->battery) {
		info->SaveGame[0] = WRAM;
		info->SaveGameLen[0] = (info->PRGRamSaveSize & ~0x7FF);
	}
}
