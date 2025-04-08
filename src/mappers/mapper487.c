/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2025 negativeExponent
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
#include "mmc3.h"

static uint8 reg[2];

static SFORMAT StateRegs[] = {
	{ reg, 2, "REGS" },
	{ 0 }
};

static void Sync(void) {
	if (reg[1] & 0x40) {
		setprg32(0x8000, (reg[1] & 0x3E) | ((reg[0] >> 3) & 0x01));
		setchr8(((reg[1] << 2) & 0xF8) | (reg[0] & 0x07));
	} else {
		setprg32(0x8000, reg[1] & 0x3F);
		setchr8(((reg[1] << 2) & 0xFC) | (reg[0] & 0x03));
	}
	setmirror(((reg[1] >> 7) & 0x01) ^ 0x01);
}

static DECLFW(M487WriteNINA) {
/*	FCEU_printf("wr %04x %02x\n", A, V); */
	switch (A & 0x4180) {
	case 0x4100:
		if (!(reg[1] & 0x20)) {
			reg[0] = V;
			Sync();
		}
		break;
	case 0x4180:
		reg[1] = V;
		Sync();
		break;
	}
}

static DECLFW(M487WriteColorDreams) {
	if (reg[1] & 0x20) {
		reg[0] = ((V << 3) & 0x08) | ((V >> 4) & 0x07);
		Sync();
	}
}

static void M487Reset(void) {
	reg[0] = reg[1] = 0;
	Sync();
}

static void M487Power(void) {
	reg[0] = reg[1] = 0;
	Sync();
	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0x4100, 0x5FFF, M487WriteNINA);
	SetWriteHandler(0x8000, 0xFFFF, M487WriteColorDreams);
}

static void StateRestore(int version) {
	Sync();
}

void Mapper487_Init(CartInfo *info) {
	info->Power = M487Power;
	info->Reset = M487Reset;
	GameStateRestore = StateRestore;
	AddExState(&StateRegs, ~0, 0, 0);

    ROM.prg.data = realloc(ROM.prg.data, 2048 * 1024);
	memmove(ROM.prg.data + (1024 * 1024), ROM.prg.data + (512 * 1024), 1024 * 1024);
	memcpy(ROM.prg.data + (512 * 1024), ROM.prg.data, 512 * 1024);
	SetupCartPRGMapping(0, ROM.prg.data, 2048 * 1024, 0);

	ROM.chr.data = realloc(ROM.chr.data, 2048 * 1024);
	memmove(ROM.chr.data + (1024 * 1024), ROM.chr.data + (512 * 1024), 1024 * 1024);
	memcpy(ROM.chr.data + (512 * 1024), ROM.chr.data, 512 * 1024);
	SetupCartCHRMapping(0, ROM.chr.data, 2048 * 1024, 0);
}
