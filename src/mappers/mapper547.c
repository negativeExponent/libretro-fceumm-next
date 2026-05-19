/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2005-2019 CaH4e3 (FCEUX)
 *  Copyright (C) 2024-2025-2026 negativeExponent
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
 * VRC-V (CAI Shogakko no Sansu)
 *
 */

/*
 * NES 2.0 Mapper 547 denotes games for the Konami QTa adapter, based on the
 * Konami VRC5 ASIC.
 * Its UNIF board name is KONAMI-QTAI (sic).
 */

#include "mapinc.h"

static struct {
	uint8_t reg[16];
	uint8_t IRQa;
	uint8_t IRQr;
	uint32_t IRQLatch;
	uint32_t IRQCount;
} m547;

static SFORMAT StateRegs[] = {
	{ m547.reg, 16, "REGS" },
	{ &m547.IRQCount, 4 | FCEUSTATE_RLSB, "IRQC" },
	{ &m547.IRQLatch, 4 | FCEUSTATE_RLSB, "IRQL" },
	{ &m547.IRQa, 1, "IRQA" },
	{ &m547.IRQr, 1, "IRQR" },

	{ &qtaintramreg, 1, "IREG" },
	{ QTAINTRAM, 0x800, "IRAM" },
	{ 0 }
};

static void SyncPRG(void) {
	/*
	D~7654 3210
	  ---------
	  .CBB BBBB
	   |++-++++- PRG A13-A18
	   +-------- Chip select
				 0: Internal PRG-ROM (128 KiB)
				 1: External PRG-ROM (512 KiB) */
	setprg8(0x8000, (m547.reg[2] & 0x40) ? (0x10 + (m547.reg[2] & 0x3F)) : (m547.reg[2] & 0x0F));
	setprg8(0xA000, (m547.reg[3] & 0x40) ? (0x10 + (m547.reg[3] & 0x3F)) : (m547.reg[3] & 0x0F));
	setprg8(0xC000, (m547.reg[4] & 0x40) ? (0x10 + (m547.reg[4] & 0x3F)) : (m547.reg[4] & 0x0F));
	setprg8(0xE000, 0x10 + 0x3F);
}

static void SyncCHR(void) {
	setchr4r(0x10, 0x0000, m547.reg[5] & 0x01);
	setchr4r(0x10, 0x1000, 0x01);
}

static void SyncMirror(void) {
	setmirror(((m547.reg[10] >> 1) & 1) ^ 1);
}

static void SyncWRAM(void) {
	/*
	D~7654 3210
	  ---------
	  .... C..B
		   |  +- PRG A12
		   +---- Chip select
				 0: External cartridge's 8 KiB (battery-backed)
				 1: Internal 8 KiB (not battery-backed) */
	setprg4r(0x10, 0x6000, ((m547.reg[0] & 0x08) >> 2) | (m547.reg[0] & 0x01));
	setprg4r(0x10, 0x7000, ((m547.reg[1] & 0x08) >> 2) | (m547.reg[1] & 0x01));
}

static void Sync(void) {
	SyncPRG();
	SyncWRAM();
	SyncCHR();
	SyncMirror();
}

static const uint8_t pageTable[0x24] = {
	/* JIS X 0208 rows $20-$4F. $20 is not a valid row number. */
	0x0, 0x0, 0x2, 0x2, 0x1, 0x1, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF,
	/* JIS X 0208 rows $50-$7F. $7F is not a valid row number. */
	0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF, 0xD, 0xD
};

static DECLFR(ReadReg) {
	uint8_t row = m547.reg[13] - 0x20;
	uint8_t col = m547.reg[12] - 0x20;

	if ((row < 0x60) && (col < 0x60)) {
		/* "row" and "col" are the first and second 7-bit JIS X 0208 code byte, respectively, each minus the $21 offset. */
		uint16_t code = (col % 32) +    /* First, go through 32 columns of a column-third. */
		    (row % 16) * 32 +         /* Then, through 16 rows of a row-third. */
		    (col / 32) * 32 * 16 +    /* Then, through three column-thirds. */
		    (row / 16) * 32 * 16 * 3; /* Finally, through three row-thirds. */
		uint16_t glyph = (code & 0xFF) | (pageTable[code >> 8] << 8);
		uint32_t tile = glyph * 4; /* four tiles per glyph */

		if (A == 0xDC00) {
			/* tile number */
			return ((tile & 0xFF) | (m547.reg[11] & 0x03));
		} else {
			/* bank byte */
			return (0x40 | ((m547.reg[11] & 0x04) << 5) | (tile >> 8));
		}
	}
	return 0;
}

static DECLFW(WriteReg) {
	int index = (A & 0x0F00) >> 8;

	m547.reg[index] = V;
	switch (A) {
	case 0xD000:
	case 0xD100:
		SyncWRAM();
		break;
	case 0xD200:
	case 0xD300:
	case 0xD400:
		SyncPRG();
		break;
	case 0xD500:
		SyncCHR();
		break;
	case 0xD600:
		m547.IRQLatch &= 0xFF00;
		m547.IRQLatch |= V;
		break;
	case 0xD700:
		m547.IRQLatch &= 0x00FF;
		m547.IRQLatch |= V << 8;
		break;
	case 0xD800:
		m547.IRQa = m547.IRQr;
		X6502_IRQEnd(FCEU_IQEXT);
		break;
	case 0xD900:
		m547.IRQr = V & 0x01;
		m547.IRQa = V & 0x02;
		if (m547.IRQa) {
			m547.IRQCount = m547.IRQLatch;
		}
		X6502_IRQEnd(FCEU_IQEXT);
		break;
	case 0xDA00:
		SyncMirror();
		/* register shadow to share it with ppu */
		qtaintramreg = m547.reg[10] & 0x03;
		break;
	}
}

static void CPUIRQHook(int a) {
	if (m547.IRQa) {
		m547.IRQCount += a;
		if (m547.IRQCount & 0x10000) {
			X6502_IRQBegin(FCEU_IQEXT);
			m547.IRQCount = m547.IRQLatch;
		}
	}
}

static void Power(void) {
	memset(&m547, 0, sizeof(m547));
	Sync();
	SetReadHandler(0x6000, 0xFFFF, CartBR);
	SetWriteHandler(0x6000, 0x7FFF, CartBW);
	SetWriteHandler(0xD000, 0xDFFF, WriteReg);
	SetReadHandler(0xDC00, 0xDC00, ReadReg);
	SetReadHandler(0xDD00, 0xDD00, ReadReg);
	FCEU_CheatAddRAM(WRAMSIZE >> 10, 0x6000, WRAM);
}

static void Close(void) {
	QTAIHack = FALSE;
}

static void StateRestore(int version) {
	Sync();
}

void Mapper547_Init(CartInfo *info) {
	QTAIHack = TRUE;

	info->Power = Power;
	info->Close = Close;
	MapIRQHook = CPUIRQHook;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, 0);

	if (iNESCart.iNES2) {
		WRAMSIZE = info->PRGRamSize + info->PRGRamSaveSize;
		CHRRAMSIZE = info->CHRRamSize + info->CHRRamSaveSize;
	}

	if (!CHRRAMSIZE) {
		CHRRAMSIZE = 8192;
	}
	if (!WRAMSIZE) {
		WRAMSIZE = 8192 + 8192; /* 8K external + 8K internal RAM */
	}

	CHRRAM = (uint8_t *)FCEU_gmalloc(CHRRAMSIZE);
	SetupCartCHRMapping(0x10, CHRRAM, CHRRAMSIZE, 1);
	AddExState(CHRRAM, CHRRAMSIZE, 0, "CRAM");

	WRAM = (uint8_t *)FCEU_gmalloc(WRAMSIZE);
	SetupCartPRGMapping(0x10, WRAM, WRAMSIZE, 1);
	AddExState(WRAM, WRAMSIZE, 0, "WRAM");

	info->battery = TRUE;
	iNESCart.SaveGame[0] = WRAM;
	iNESCart.SaveGameLen[0] = 8192; /* only bank 0, the external cartridge RAM is battery-backed */
}
