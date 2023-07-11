/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2002 Xodnizel
 *  Copyright (C) 2023
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

/* NES 2.0 Mapper 210 - simplified version of Mapper 19 
 * Namco 175 - submapper 1 - optional wram, hard-wired mirroring
 * Namco 340 - submapper 2 - selectable H/V/0 mirroring
 */

#include "mapinc.h"

static uint8 *WRAM;
static uint32 WRAMSIZE;

static uint8 PRG[3];
static uint8 CHR[8];
static uint8 mirr;
static uint8 wram_enable;

static uint8 submapper;

static SFORMAT StateRegs[] = {
	{ PRG, 3, "PRG" },
	{ CHR, 8, "CHR" },
	{ &mirr, 1, "GORK" },
    { &wram_enable, 1, "WREN" },
	{ 0 }
};

static void SyncPRG(void) {
	setprg8(0x8000, PRG[0]);
	setprg8(0xa000, PRG[1]);
	setprg8(0xc000, PRG[2]);
	setprg8(0xe000, 0x3F);
}

static void SyncMirror() {
	switch(mirr & 3) {
	case 0: setmirror(MI_0); break;
	case 1: setmirror(MI_V); break;
	case 2: setmirror(MI_H); break;
	case 3: setmirror(MI_0); break;
	}
}

static void DoCHRRAMROM(int x, uint8 V) {
	CHR[x] = V;
	setchr1(x << 10, V);
}

static void FixCRR(void) {
	int x;
	for (x = 0; x < 8; x++)
		DoCHRRAMROM(x, CHR[x]);
}

static DECLFR(AWRAM) {
    A = ((A - 0x6000) & (WRAMSIZE -1));
	return WRAM[A];
}

static DECLFW(BWRAM) {
    if (wram_enable) {
        A = ((A - 0x6000) & (WRAMSIZE -1));
	    WRAM[A] = V;
    }
}

static DECLFW(M210Write) {
	A &= 0xF800;
	if (A >= 0x8000 && A <= 0xb800)
		DoCHRRAMROM((A - 0x8000) >> 11, V);
	else
		switch (A) {
        case 0xC000:
            wram_enable = V & 0x01;
            break;
		case 0xE000:
			PRG[0] = V & 0x3F;
			SyncPRG();
            if (submapper != 1) {
                mirr = V >> 6;
			    SyncMirror();
            }
			break;
		case 0xE800:
			PRG[1] = V & 0x3F;
			SyncPRG();
			break;
		case 0xF000:
			PRG[2] = V & 0x3F;
			SyncPRG();
			break;
		}
}

static int battery = 0;

static void M210Power(void) {
	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0x8000, 0xffff, M210Write);

	if (WRAM) {
        SetReadHandler(0x6000, 0x7FFF, AWRAM);
	    SetWriteHandler(0x6000, 0x7FFF, BWRAM);
	    FCEU_CheatAddRAM(8, 0x6000, WRAM);
    }

	SyncPRG();
	FixCRR();

	if (WRAM && !battery) {
		FCEU_MemoryRand(WRAM, sizeof(WRAM));
	}
}

static void StateRestore(int version) {
	SyncPRG();
	FixCRR();
}

void Mapper210_Init(CartInfo *info) {
	GameStateRestore = StateRestore;
	info->Power = M210Power;
    submapper = info->submapper;
    battery = info->battery;
    AddExState(StateRegs, ~0, 0, 0);

    WRAMSIZE = 8192;
    if (info->iNES2) {
        WRAMSIZE = info->PRGRamSize + info->PRGRamSaveSize;
    }

    if (WRAMSIZE) {
        WRAM = (uint8 *)FCEU_gmalloc(WRAMSIZE);
	    FCEU_MemoryRand(WRAM, sizeof(WRAM));
	    AddExState(WRAM, WRAMSIZE, 0, "WRAM");
        if (battery) {
            info->SaveGame[0] = WRAM;
            info->SaveGameLen[0] = WRAMSIZE;
        }
    }
}
