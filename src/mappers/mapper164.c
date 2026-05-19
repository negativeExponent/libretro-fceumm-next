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

/* Dongda PEC-9588 educational computer. Provides the same 1bpp all-points-addressable graphics mode.
   Its chipset was later used for the Yancheng cy2000-3 PCB, used on most of the games that display "Union Bond" at the start.
   Some of them also use the 1bpp mode for a few screens!
*/

#include "mapinc.h"
#include "eeprom_93Cx6.h"

static struct {
	uint8_t reg[4];
} m164;

static uint8_t eeprom_data[512];

static SFORMAT StateRegs[] = {
	{ m164.reg, 4, "REGS" },
	{ 0 }
};

static void Sync(void) {
	/* D~7654 3210
	 * ---------
	 * CSQM PPPp
	 * ||+|-++++- PRG A18..A14 if M=0
	 * || | ++++- PRG A18..A15 if M=1
	 * || +------ PRG banking mode
	 * ||          0: PRG A14..A18=QPPPp when CPU A14=0 (UxROM, 16 KiB switchable bank)
	 * ||             PRG A14..A18=11111 when CPU A14=1 and S=0 (fixed bank=1F)
	 * ||             PRG A14..A18=111p0 when CPU A14=1 and S=1 (fixed bank=1C or 1E)
	 * ||          1: PRG A14=CPU A14, PRG A15..A18=PPPp (BxROM, 32 KiB switchable bank)
	 * ||         Also selects nametable mirroring:
	 * ||          0: Forced vertical mirroring
	 * ||          1: Mirroring selected by $5300
	 * |+-------- See 'M' bit description
	 * +--------- 1 bpp video mode: when PPU A13=0 (pattern table) ...
	 *             0: CHR A3=PPU A3, CHR A12=PPU A12 (disable 1 bpp mode)
	 *             1: CHR A3=PPU A0, CHR A12=PPU A9, both latched on
	 *               last rise of PPU A13 (enable 1 bpp mode)
	 */
	uint8_t prgHigh = m164.reg[1] << 5;
 	uint8_t prgLow = ((m164.reg[0] >> 1) & 0x10) | (m164.reg[0] & 0x0F);
	uint8_t mirrorH = ((m164.reg[0] & 0x10) && !(m164.reg[3] & 0x80)) ? MI_H : MI_V;

	if (m164.reg[0] & 0x10) {
		if (m164.reg[0] & 0x20) {
			setprg16(0x8000, prgHigh | ((prgLow << 1) & 0x10) | (prgLow & 0x0F));
			setprg16(0xC000, prgHigh | ((prgLow << 1) & 0x10) | 0x0F);
		} else {
			setprg32(0x8000, (prgHigh >> 1) | prgLow);
		}
	} else {
		setprg16(0x8000, prgHigh | prgLow);
		setprg16(0xC000, prgHigh | (!(m164.reg[0] & 0x40)) ? 0x1F : ((prgLow >= 0x1C) ? 0x1C : 0x1E));
	}

	setmirror(mirrorH);
}

static DECLFR(ReadReg) {
	return eeprom_93Cx6_read() ? 0x00 : 0x04;
}

static DECLFW(WriteReg) {
	switch (A & 0xFF00) {
	case 0x5000:
		m164.reg[0] = V;
		PEC586Hack = (m164.reg[0] & 0x80) ? TRUE : FALSE;
		Sync();
		break;
	case 0x5100:
		m164.reg[1] = V;
		Sync();
		break;
	case 0x5200:
		m164.reg[2] = V;
		eeprom_93Cx6_write((m164.reg[2] & 0x10), (m164.reg[2] & 0x04), (m164.reg[2] & 0x01));
		break;
	case 0x5300:
		m164.reg[3] = V;
		Sync();
		break;
	}
}

static void Power(void) {
	memset(&m164, 0, sizeof(m164));
	Sync();
	SetReadHandler(0x5400, 0x57FF, ReadReg);
	SetWriteHandler(0x5000, 0x57FF, WriteReg);
	SetReadHandler(0x6000, 0xFFFF, CartBR);
	SetWriteHandler(0x6000, 0x7FFF, CartBW);
	FCEU_CheatAddRAM(WRAMSIZE >> 10, 0x6000, WRAM);

	setprg8r(0x10, 0x6000, 0);
	setchr8(0);
}

static void Reset(void) {
	memset(&m164, 0, sizeof(m164));
	Sync();
}

static void Close(void) {
}

static void StateRestore(int version) {
	Sync();
}

void Mapper164_Init(CartInfo *info) {
	info->Power = Power;
	info->Reset = Reset;
	info->Close = Close;

	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);

	eeprom_93Cx6_init(eeprom_data, 512, 8);
	info->battery = 1;
	info->SaveGame[0] = eeprom_data;
	info->SaveGameLen[0] = 512;
	AddExState(eeprom_data, sizeof(eeprom_data), 0, "EPRM");

	WRAMSIZE = 8192;
	if (info->iNES2) {
		WRAMSIZE = info->iNES2 ? (info->PRGRamSize + (info->PRGRamSaveSize & ~0x7FF)) : 8192;
	}
	if (WRAMSIZE) {
		WRAM = (uint8_t *)FCEU_gmalloc(WRAMSIZE);
		SetupCartPRGMapping(0x10, WRAM, WRAMSIZE, 1);
		AddExState(WRAM, WRAMSIZE, 0, "WRAM");
	}
}
