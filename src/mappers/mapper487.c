/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2025-2026 negativeExponent
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

/* NES 2.0 Mapper 487 denotes the AVE NINA-08 circuit board, used only for the
   unreleased original 30-in-1 version of the Maxivision multicart.
   Unlike the released version, which uses INES Mapper 234,
   the 30-in-1 contained both AVE and Color Dreams games completely unmodified,
   so the board supports both original mappers' bankswitching schemes.

   30-in-1 (Maxivision) (Proto)
 */

#include "mapinc.h"

static struct {
	uint8_t reg[2];
} m487;

static SFORMAT StateRegs[] = {
	{ m487.reg, 2, "EXPR" },
	{ 0 }
};

static void Sync(void) {
	uint8_t prg, chr;

	if (m487.reg[1] & 0x40) {
		prg = (m487.reg[1] & 0x3E) | ((m487.reg[0] >> 3) & 0x01);
		chr = ((m487.reg[1] << 2) & 0xF8) | (m487.reg[0] & 0x07);
	} else {
		prg = m487.reg[1] & 0x3F;
		chr = ((m487.reg[1] << 2) & 0xFC) | (m487.reg[0] & 0x03);
	}

	setprg32(0x8000, prg);
	setchr8(chr);
	setmirror(((m487.reg[1] >> 7) & 0x01) ^ 0x01);
}

static DECLFW(Write4) {
	/*	FCEU_printf("wr %04x %02x\n", A, V); */
	switch (A & 0x4180) {
	case 0x4100: /* NINA-03-compatible Inner Bank Register */
		if (!(m487.reg[1] & 0x20)) {
			m487.reg[0] = V;
			Sync();
		}
		break;
	case 0x4180: /* Outer Bank Register */
		m487.reg[1] = V;
		Sync();
		break;
	}
}

static DECLFW(Write8) {
	if (m487.reg[1] & 0x20) {
		/* Color-Dreams-compatible Inner Bank Register */
		/* Registers rearranged to be similar to NINA-03 registers */
		m487.reg[0] = ((V << 3) & 0x08) | ((V >> 4) & 0x07);
		Sync();
	}
}

static void Reset(void) {
	memset(&m487, 0, sizeof(m487));
	Sync();
}

static void Power(void) {
	memset(&m487, 0, sizeof(m487));
	Sync();
	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0x4100, 0x5FFF, Write4);
	SetWriteHandler(0x8000, 0xFFFF, Write8);
}

static void StateRestore(int version) {
	Sync();
}

void Mapper487_Init(CartInfo *info) {
	info->Power = Power;
	info->Reset = Reset;
	GameStateRestore = StateRestore;
	AddExState(&StateRegs, ~0, 0, 0);

	/* The register layout assumes a 2 MiB address space for PRG and CHR,
	 * with 1 MiB allocated for each. However, the first ROM chip only contains
	 * 512 KiB of PRG and 512 KiB of CHR, and the .NES file is not padded or
	 * repeated to fill the full 2 MiB space.
	 *
	 * To accommodate the banking logic without modifying it, the ROM is rebuilt
	 * by duplicating the first 512 KiB of PRG and CHR to fill the full 1 MiB
	 * expected by the mapper.
	 */

	if (ROM.prg.size < (2048 * 1024) && ROM.chr.size < (2048 * 1024)) {
		ROM.prg.data = realloc(ROM.prg.data, 2048 * 1024);
		memmove(ROM.prg.data + (1024 * 1024), ROM.prg.data + (512 * 1024), 1024 * 1024);
		memcpy(ROM.prg.data + (512 * 1024), ROM.prg.data, 512 * 1024);
		SetupCartPRGMapping(0, ROM.prg.data, 2048 * 1024, 0);

		ROM.chr.data = realloc(ROM.chr.data, 2048 * 1024);
		memmove(ROM.chr.data + (1024 * 1024), ROM.chr.data + (512 * 1024), 1024 * 1024);
		memcpy(ROM.chr.data + (512 * 1024), ROM.chr.data, 512 * 1024);
		SetupCartCHRMapping(0, ROM.chr.data, 2048 * 1024, 0);
	}
}
