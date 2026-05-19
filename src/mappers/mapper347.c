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
 *
 * NES 2.0 Mapper 347 - Kaiser 7030
 * UNIF UNL-KS7030
 * FDS Conversion - Yume Koujou: Doki Doki Panic
 *
 * Logical bank layot 32 K BANK 0, 64K BANK 1, 32K ~0 hardwired, 8K is missing
 * need redump from MASKROM!
 * probably need refix mapper after hard dump
 *
 */

/* 2020-3-6 - update mirroring
 * PRG-ROM Bank Select #1/Mirroring Select ($8000-$8FFF, write)
 * A~FEDC BA98 7654 3210
 * -------------------
 *  1000 .... .... MBBB
 *                 |+++- Select 4 KiB PRG-ROM bank at CPU $7000-$7FFF
 *                 +---- Select nametable mirroring type
 *                        0: Vertical
 *                        1: Horizontal
 */

 /* Update: 2025-05-23
 * As the FCEUX source code comment indicates, the actual bank order in the
 * 128 KiB mask ROM was unknown until July 2020. Emulators previously expected
 * the ROM image to be laid out as follows:
 *
 * - The first 32 KiB contains the eight banks selected by register $8000,
 *   mapped to CPU $7000–$7FFF.
 *
 * - The next 64 KiB contains the sixteen banks selected by register $9000,
 *   with:
 *     - The first 1 KiB mapped to CPU $6C00–$6FFF
 *     - The second 3 KiB mapped to CPU $C000–$CBFF
 *
 * - The final 32 KiB is mapped to CPU $8000–$FFFF,
 *   except where replaced by RAM and the switchable PRG-ROM bank.
 *
 * However, the actual mask ROM layout is different:
 *
 * - The first 64 KiB contains the sixteen banks selected by register $9000,
 *   with:
 *     - The first 3 KiB mapped to CPU $C000–$CBFF
 *     - The second 1 KiB mapped to CPU $6C00–$6FFF
 *
 * - The next 32 KiB contains the eight banks selected by register $8000,
 *   mapped to CPU $7000–$7FFF.
 *
 * - The final 32 KiB is still mapped to CPU $8000–$FFFF,
 *   except where replaced by RAM and the switchable PRG-ROM bank.
 */

#include "mapinc.h"
#include "fdssound.h"

static struct {
	uint8_t reg[2];
	uint32_t pageOffset[3];
} m347;

static uint8_t old_rombank = FALSE;

static SFORMAT StateRegs[] = {
	{ m347.reg, 2, "REGS" },
	{ &m347.pageOffset[0], sizeof(m347.pageOffset[0]) | FCEUSTATE_RLSB, "OPG0" },
	{ &m347.pageOffset[1], sizeof(m347.pageOffset[1]) | FCEUSTATE_RLSB, "OPG1" },
	{ &m347.pageOffset[2], sizeof(m347.pageOffset[2]) | FCEUSTATE_RLSB, "OPG2" },
	{ 0 }
};

static void Sync(void) {
	setchr8(0);
	setprg32(0x8000, ~0);
	setmirror(((m347.reg[0] >> 3) & 0x01) ^ 0x01);

	m347.pageOffset[0] = (0x1000 * (m347.reg[1] & 0x0F)) + (old_rombank ? 0x08000 : 0x00C00);
	m347.pageOffset[1] = (0x1000 * (m347.reg[0] & 0x07)) + (old_rombank ? 0x00000 : 0x10000);
	m347.pageOffset[2] = (0x1000 * (m347.reg[1] & 0x0F)) + (old_rombank ? 0x08400 : 0x00000);
}

static DECLFR(ReadROMRAM) {
	if ((A >= 0x6000) && (A <= 0x6BFF)) {
		return WRAM[A - 0x6000];
	} else if ((A >= 0x6C00) && (A <= 0x6FFF)) {
		return ROM.prg.data[((A - 0x6C00) + m347.pageOffset[0]) & (ROM.prg.size - 1)];
	} else if ((A >= 0x7000) && (A <= 0x7FFF)) {
		return ROM.prg.data[((A - 0x7000) + m347.pageOffset[1]) & (ROM.prg.size - 1)];
	} else if ((A >= 0xB800) && (A <= 0xBFFF)) {
		return WRAM[0x0C00 + (A - 0xB800)];
	} else if ((A >= 0xC000) && (A <= 0xCBFF)) {
		return ROM.prg.data[((A - 0xC000) + m347.pageOffset[2]) & (ROM.prg.size - 1)];
	} else if ((A >= 0xCC00) && (A <= 0xD7FF)) {
		return WRAM[0x1400 + (A - 0xCC00)];
	}
	return CartBR(A);
}

static DECLFW(WriteRAMReg) {
	if ((A >= 0x6000) && (A <= 0x6BFF)) {
		WRAM[A - 0x6000] = V;
	} else if ((A >= 0xB800) && (A <= 0xBFFF)) {
		WRAM[0x0C00 + (A - 0xB800)] = V;
	} else if ((A >= 0xCC00) && (A <= 0xD7FF)) {
		WRAM[0x1400 + (A - 0xCC00)] = V;
	} else if ((A >= 0x8000) && (A <= 0x8FFF)) {
		if (m347.reg[0] != (A & 0x0F)) {
			m347.reg[0] = A & 0x0F;
			Sync();
		}
	} else if ((A >= 0x9000) && (A <= 0x9FFF)) {
		if (m347.reg[1] = A & 0x0F) {
			m347.reg[1] = A & 0x0F;
			Sync();
		}
	}
}

static void Power(void) {
	memset(&m347, 0xFF, sizeof(m347));
	FDSSound_Power();
	SetReadHandler(0x6000, 0xFFFF, ReadROMRAM);
	SetWriteHandler(0x6000, 0xFFFF, WriteRAMReg);

	if (iNESCart.CRC32 == 0xFA4DAC91) {
		old_rombank = TRUE;
	} else {
		old_rombank = FALSE;
	}

	Sync();
}

static void StateRestore(int version) {
	Sync();
}

void Mapper347_Init(CartInfo *info) {
	info->Power = Power;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);

	WRAMSIZE = 8192;
	WRAM = (uint8_t *)FCEU_gmalloc(WRAMSIZE);
	AddExState(WRAM, WRAMSIZE, 0, "WRAM");
}
