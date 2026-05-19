/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2002 Xodnizel
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

#include "mapinc.h"
#include "n163sound.h"

/* libretro saveram workaround, since there is no easy way to save more than 1 save index, so */
/* packed both wram (if enabled) and internal ram into 1 memory block */
static uint8_t libretro_save_ram[8192 + 128]; /* wram 8k + 128 bytes internal ram */
static uint8_t *internalRAM = NULL;

static struct {
	uint8_t prg[4];
	uint8_t chr[8];
	uint8_t nmt[4];
	uint8_t write_protect;
	uint16_t IRQCount;
	uint8_t IRQa;
} m019;

static SFORMAT StateRegs[] = {
	{ m019.prg, 4, "PREG" },
	{ m019.chr, 8, "CREG" },
	{ m019.nmt, 4, "NMTR" },
	{ &m019.IRQCount, 2 | FCEUSTATE_RLSB, "IRQC" },
	{ &m019.IRQa, 1, "IRQA" },
	{ &m019.write_protect, 1, "WPRT" },
	{ 0 }
};

static void DoPRG(int x, uint8_t V) {
	m019.prg[x] = V;
	setprg8(0x8000 + (x << 13), V);
}

static void SyncPRG(void) {
	DoPRG(0, m019.prg[0] & 0x3F);
	DoPRG(1, m019.prg[1] & 0x3F);
	DoPRG(2, m019.prg[2] & 0x3F);
	DoPRG(3, m019.prg[3] & 0x3F);
}

static void DoCHRRAMROM(int x, uint8_t V) {
	uint8_t force_chrrom = FALSE;

	m019.chr[x] = V;
	if ((x < 4) && (m019.prg[1] & 0x40)) {
		force_chrrom = TRUE;
	} else if ((x < 8) && (m019.prg[1] & 0x80)) {
		force_chrrom = TRUE;
	}
	if (force_chrrom || (V < 0xE0)) {
		setchr1(x << 10, V);
	}
}

static void SyncCHR(void) {
	int x;
	for (x = 0; x < 8; x++) {
		DoCHRRAMROM(x, m019.chr[x]);
	}
}

static void DoNMTRAMROM(int w, uint8_t V) {
	m019.nmt[w] = V;
	if (V < 0xE0) {
		V &= CHRmask1[0];
		setntamem(CHRptr[0] + (V << 10), 0, w);
	} else {
		setntamem(NTARAM + ((V & 0x01) << 10), 1, w);
	}
}

static void SyncNMT(void) {
	int x;
	for (x = 0; x < 4; x++) {
		DoNMTRAMROM(x, m019.nmt[x]);
	}
}

static void SyncWRAM(void) {
	setprg2r_access(0x10, 0x6000, 0, TRUE, ((m019.write_protect & 0xF1) == 0x40));
	setprg2r_access(0x10, 0x6800, 1, TRUE, ((m019.write_protect & 0xF2) == 0x40));
	setprg2r_access(0x10, 0x7000, 2, TRUE, ((m019.write_protect & 0xF4) == 0x40));
	setprg2r_access(0x10, 0x7800, 3, TRUE, ((m019.write_protect & 0xF8) == 0x40));
}

static void CPUCycle(int a) {
	if (m019.IRQa) {
		m019.IRQCount += a;
		if (m019.IRQCount >= 0x7FFF) {
			m019.IRQCount = 0x7FFF;
			X6502_IRQBegin(FCEU_IQEXT);
		}
	}
}

static DECLFR(ReadInternalRAM) {
	return N163Sound_Read(A);
}

static DECLFW(WriteInternalRAM) {
	N163Sound_Write(A, V);
}

static DECLFR(ReadIRQCount) {
	if (A & 0x800) {
		return ((m019.IRQCount >> 8) & 0xFF);
	}
	return (m019.IRQCount & 0xFF);
}

static DECLFW(WriteIRQCount) {
	X6502_IRQEnd(FCEU_IQEXT);
	if (A & 0x800) {
		m019.IRQCount = (m019.IRQCount & 0x00FF) | ((V & 0x7F) << 8);
		m019.IRQa = (V & 0x80) ? TRUE : FALSE;
	} else {
		m019.IRQCount = (m019.IRQCount & 0x7F00) | (V & 0xFF);
	}
}

static DECLFW(WriteCHR) {
	DoCHRRAMROM((A - 0x8000) >> 11, V);
}

static DECLFW(WriteNT) {
	DoNMTRAMROM((A - 0xC000) >> 11, V);
}

static DECLFW(WritePRG) {
	DoPRG((A - 0xE000) >> 11, V);
}

static DECLFW(WriteWRAMProtect) {
	m019.write_protect = V;
	SyncWRAM();
	N163Sound_Write(A, V);
}

static void StateRestore(int version) {
	SyncPRG();
	SyncCHR();
	SyncNMT();
	SyncWRAM();
}

static void Power(void) {
	int i;

	for (i = 0; i < 4; i++) m019.prg[i] = 0xFC + i;
	for (i = 0; i < 8; i++) m019.chr[i] = i;
	for (i = 0; i < 4; i++) m019.nmt[i] = 0xE0 + (i & 0x01);

	m019.write_protect = 0xFF;

	SyncPRG();
	SyncCHR();
	SyncNMT();
	SyncWRAM();

	SetReadHandler(0x4800, 0x4FFF, ReadInternalRAM);
	SetWriteHandler(0x4800, 0x4FFF, WriteInternalRAM);

	SetReadHandler(0x5000, 0x5FFF, ReadIRQCount);
	SetWriteHandler(0x5000, 0x5FFF, WriteIRQCount);

	if (WRAMSIZE) {
		SetReadHandler(0x6000, 0x7FFF, CartBR);
		SetWriteHandler(0x6000, 0x7FFF, CartBW);
		FCEU_CheatAddRAM(8, 0x6000, WRAM);
	}

	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0x8000, 0xBFFF, WriteCHR);
	SetWriteHandler(0xC000, 0xDFFF, WriteNT);
	SetWriteHandler(0xE000, 0xF7FF, WritePRG);
	SetWriteHandler(0xF800, 0xFFFF, WriteWRAMProtect);

	if (!iNESCart.battery) {
		FCEU_MemoryRand(WRAM, WRAMSIZE);
		FCEU_MemoryRand(internalRAM, 128);
	}
}

static void Close(void) {
	internalRAM = NULL;
	WRAM = NULL;
}

void Mapper019_Init(CartInfo *info) {
	memset(libretro_save_ram, 0, sizeof(libretro_save_ram));

	info->Power = Power;
	info->Close = Close;

	MapIRQHook = CPUCycle;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, 0);

	if (info->iNES2) {
		WRAMSIZE = info->PRGRamSize + info->PRGRamSaveSize;
	} else if (info->battery) {
		WRAMSIZE = 8192;
	}

	internalRAM = &libretro_save_ram[0];

	if (WRAMSIZE) {
		WRAM = &libretro_save_ram[128];
		SetupCartPRGMapping(0x10, WRAM, WRAMSIZE, 1);
		AddExState(WRAM, WRAMSIZE, 0, "WRAM");
	}

	if (info->battery) {
		info->SaveGame[0] = libretro_save_ram;
		info->SaveGameLen[0] = WRAMSIZE ? (8192 + 128) : 128;
	}

	N163Sound_ESI(internalRAM);
	N163Sound_AddStateInfo();
}
