/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2006 CaH4e3
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

/* iNES Mapper 068 denotes PCBs using the Sunsoft-4 mapper IC. In the US it was
 * only used in the game After Burner. It has the unusual ability to map CHR ROM
 * into the part of the PPU's address space used for nametables.
 *
 * Example games:
 *
 * After Burner
 * Maharaja (J)
 * Nantettatte!! Baseball (J)
 */

#include "mapinc.h"

static struct {
	uint8_t chr[4];
	uint8_t nt[2];
	uint8_t mirror;
	uint8_t prg;
	uint8_t access;
	int32_t timer;
} m068;

static SFORMAT StateRegs[] = {
	{ m068.chr, 4, "CREG" },
	{ m068.nt, 2, "NTAR" },
	{ &m068.mirror, 1, "MIRR" },
	{ &m068.prg, 1, "PREG" },
	{ &m068.access, 1, "ACCS" },
	{ &m068.timer, 4 | FCEUSTATE_RLSB, "TIMR" },
	{ 0 }
};

static void SyncPRG(void) {
	setprg8r(0x10, 0x6000, 0);
	if (iNESCart.submapper == 1) {
		if (!(m068.prg & 0x08)) { /* map external ROM, which can be disabled */
			setprg16(0x8000, 0x08);
		} else { /* else internal 128K ROM */
			setprg16(0x8000, m068.prg & 0x07);
		}
		setprg16(0xC000, 0x07);
	} else {
		setprg16(0x8000, m068.prg);
		setprg16(0xC000, 0xFF);
	}
}

static void SyncCHR(void) {
	setchr2(0x0000, m068.chr[0]);
	setchr2(0x0800, m068.chr[1]);
	setchr2(0x1000, m068.chr[2]);
	setchr2(0x1800, m068.chr[3]);
}

static void SyncMirror(void) {
	if (m068.mirror & 0x10) {
		size_t bank0 = 0x0400 * ((0x80 | m068.nt[0]) & CHRmask1[0]);
		size_t bank1 = 0x0400 * ((0x80 | m068.nt[1]) & CHRmask1[0]);

		switch (m068.mirror & 0x03) {
		case 0:
			setntamem(CHRptr[0] + bank0, FALSE, 0);
			setntamem(CHRptr[0] + bank1, FALSE, 1);
			setntamem(CHRptr[0] + bank0, FALSE, 2);
			setntamem(CHRptr[0] + bank1, FALSE, 3);
			break;
		case 1:
			setntamem(CHRptr[0] + bank0, FALSE, 0);
			setntamem(CHRptr[0] + bank0, FALSE, 1);
			setntamem(CHRptr[0] + bank1, FALSE, 2);
			setntamem(CHRptr[0] + bank1, FALSE, 3);
			break;
		case 2:
			setntamem(CHRptr[0] + bank0, FALSE, 0);
			setntamem(CHRptr[0] + bank0, FALSE, 1);
			setntamem(CHRptr[0] + bank0, FALSE, 2);
			setntamem(CHRptr[0] + bank0, FALSE, 3);
			break;
		case 3:
			setntamem(CHRptr[0] + bank1, FALSE, 0);
			setntamem(CHRptr[0] + bank1, FALSE, 1);
			setntamem(CHRptr[0] + bank1, FALSE, 2);
			setntamem(CHRptr[0] + bank1, FALSE, 3);
			break;
		}
	} else {
		switch (m068.mirror & 0x03) {
		case 0:
			setmirror(MI_V);
			break;
		case 1:
			setmirror(MI_H);
			break;
		case 2:
			setmirror(MI_0);
			break;
		case 3:
			setmirror(MI_1);
			break;
		}
	}
}

static DECLFR(ReadWRAM) {
	if (m068.prg & 0x10) {
		return CartBR(A);
	}
	return cpu.openbus;
}

static DECLFW(WriteWRAM) {
	if (m068.prg & 0x10) {
		CartBW(A, V);
	} else if (iNESCart.submapper == 1) {
		m068.access = TRUE;
		m068.timer = 107520;
		SyncPRG();
	}
}

static DECLFR(ReadExternalROM) {
	if (!(m068.prg & 0x08) && !m068.access) {
		return cpu.openbus;
	}
	return CartBR(A);
}

static DECLFW(WriteCHR) {
	m068.chr[(A >> 12) & 0x03] = V;
	SyncCHR();
}

static DECLFW(WriteNT) {
	m068.nt[(A >> 12) & 0x01] = V;
	SyncMirror();
}

static DECLFW(WriteMirrorControl) {
	m068.mirror = V;
	SyncMirror();
}

static DECLFW(WritePRG) {
	m068.prg = V;
	SyncPRG();
}

static INLINE void CPUCycle(int a) {
	if (m068.timer > 0) {
		m068.timer -= a;
		if (m068.timer <= 0) {
			m068.access = FALSE;
			SyncPRG();
		}
	}
}

static void Power(void) {
	memset(&m068, 0, sizeof(m068));

	m068.chr[0] = 0;
	m068.chr[1] = 1;
	m068.chr[2] = 2;
	m068.chr[3] = 3;

	m068.nt[0] = 0;
	m068.nt[1] = 1;

	SyncPRG();
	SyncCHR();
	SyncMirror();

	SetReadHandler(0x6000, 0x7FFF, ReadWRAM);
	SetWriteHandler(0x6000, 0x7FFF, WriteWRAM);

	SetReadHandler(0x8000, 0xFFFF, CartBR);
	if (iNESCart.submapper == 1) {
		SetReadHandler(0x8000, 0xBFFF, ReadExternalROM);
	}

	SetWriteHandler(0x8000, 0xBFFF, WriteCHR);
	SetWriteHandler(0xC000, 0xDFFF, WriteNT);
	SetWriteHandler(0xE000, 0xEFFF, WriteMirrorControl);
	SetWriteHandler(0xF000, 0xFFFF, WritePRG);

	FCEU_CheatAddRAM(WRAMSIZE >> 10, 0x6000, WRAM);
}

static void StateRestore(int version) {
	SyncPRG();
	SyncCHR();
	SyncMirror();
}

void Mapper068_Init(CartInfo *info) {
	info->Power = Power;
	MapIRQHook = CPUCycle;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);

	WRAMSIZE = 8192;
	WRAM = (uint8_t *)FCEU_gmalloc(WRAMSIZE);
	SetupCartPRGMapping(0x10, WRAM, WRAMSIZE, 1);
	AddExState(WRAM, WRAMSIZE, 0, "WRAM");
	if (info->battery) {
		info->SaveGame[0] = WRAM;
		info->SaveGameLen[0] = WRAMSIZE;
	}
}
