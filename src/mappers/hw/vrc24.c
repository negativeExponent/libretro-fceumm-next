/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2007 CaH4e3
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
 *
 * VRC-2/VRC-4 Konami
 * VRC-4 Pirate
 *
 * VRC2
 * Nickname	PCB		A0	A1	Registers					iNES mapper	submapper
 * VRC2a	351618	A1	A0	$x000, $x002, $x001, $x003	22			0
 * VRC2b	many†	A0	A1	$x000, $x001, $x002, $x003	23			3
 * VRC2c	351948	A1	A0	$x000, $x002, $x001, $x003	25			3
 * VRC4
 * Nickname	PCB		A0	A1	Registers					iNES mapper	submapper
 * VRC4a	352398	A1	A2	$x000, $x002, $x004, $x006	21			1
 * VRC4b	351406	A1	A0	$x000, $x002, $x001, $x003	25			1
 * VRC4c	352889	A6	A7	$x000, $x040, $x080, $x0C0	21			2
 * VRC4d	352400	A3	A2	$x000, $x008, $x004, $x00C	25			2
 * VRC4e	352396	A2	A3	$x000, $x004, $x008, $x00C	23			2
 * VRC4f	-		A0	A1	$x000, $x001, $x002, $x003	23			1
 *
 */

#include "mapinc.h"
#include "vrc24.h"
#include "vrcirq.h"

#define PRGMASK_DEFAULT 0x003F
#define CHRMASK_DEFAULT 0x01FF

void (*VRC24_SyncPRG)(void);
void (*VRC24_SyncCHR)(void);
void (*VRC24_SyncMirror)(void);
void (*VRC24_SyncWires)(void);

void (*VRC24_pwrap)(uint16_t A, uint16_t V);
void (*VRC24_cwrap)(uint16_t A, uint16_t V);

DECLFW((*VRC24_WriteExtSelect));

VRC24 vrc24;

static uint8_t lastPRGBank;

static SFORMAT StateRegs[] = {
	{ vrc24.prg, 2, "PREG" },
	{ &vrc24.chr[0], 2 | FCEUSTATE_RLSB, "VCR0" },
	{ &vrc24.chr[1], 2 | FCEUSTATE_RLSB, "VCR1" },
	{ &vrc24.chr[2], 2 | FCEUSTATE_RLSB, "VCR2" },
	{ &vrc24.chr[3], 2 | FCEUSTATE_RLSB, "VCR3" },
	{ &vrc24.chr[4], 2 | FCEUSTATE_RLSB, "VCR4" },
	{ &vrc24.chr[5], 2 | FCEUSTATE_RLSB, "VCR5" },
	{ &vrc24.chr[6], 2 | FCEUSTATE_RLSB, "VCR6" },
	{ &vrc24.chr[7], 2 | FCEUSTATE_RLSB, "VCR7" },
	{ &vrc24.cmd, 1, "CMDR" },
	{ &vrc24.mirr, 1, "MIRR" },
	{ &vrc24.wire, 1, "MWIR" },

	{ 0 }
};

uint16_t VRC24_GetPRGBank(int bank) {
	if ((vrc24.cmd & 0x02) && (!(bank & 0x01))) {
		bank ^= 0x02;
	}
	if (bank & 0x02) {
		return ((lastPRGBank - 1) + (bank & 0x01));
	}
	return (vrc24.prg[bank & 0x01]);
}

static void GENPWRAP(uint16_t A, uint16_t V) {
	setprg8(A, V & PRGMASK_DEFAULT);
}

void VRC24_SyncPRG_default(void) {
	VRC24_pwrap(0x8000, VRC24_GetPRGBank(0));
	VRC24_pwrap(0xA000, VRC24_GetPRGBank(1));
	VRC24_pwrap(0xC000, VRC24_GetPRGBank(2));
	VRC24_pwrap(0xE000, VRC24_GetPRGBank(3));
}

uint16_t VRC24_GetCHRBank(int bank) {
	return vrc24.chr[bank];
}

static void GENCWRAP(uint16_t A, uint16_t V) {
	setchr1(A, V & CHRMASK_DEFAULT);
}

void VRC24_SyncCHR_default(void) {
	VRC24_cwrap(0x0000, VRC24_GetCHRBank(0));
	VRC24_cwrap(0x0400, VRC24_GetCHRBank(1));
	VRC24_cwrap(0x0800, VRC24_GetCHRBank(2));
	VRC24_cwrap(0x0C00, VRC24_GetCHRBank(3));
	VRC24_cwrap(0x1000, VRC24_GetCHRBank(4));
	VRC24_cwrap(0x1400, VRC24_GetCHRBank(5));
	VRC24_cwrap(0x1800, VRC24_GetCHRBank(6));
	VRC24_cwrap(0x1C00, VRC24_GetCHRBank(7));
}

void VRC24_SyncMirror_default(void) {
	if ((vrc24.type == VRC24_VRC4) && (vrc24.mirr & 0x02)) {
		setmirror(MI_0 + (vrc24.mirr & 0x01));
	} else {
		setmirror((vrc24.mirr & 0x01) ^ 1);
	}
}

DECLFR(VRC24_ReadWRAM) {
	if (WRAMSIZE) {
		A = 0x6000 | ((A - 0x6000) & (WRAMSIZE - 1));
		return CartBR(A);
	}
	if ((A <= 0x6000) && (vrc24.type == VRC24_VRC2)) {
		return (cpu.openbus & ~0x01) | (vrc24.wire & 0x01);
	}
	return CartBR(A);
}

DECLFW(VRC24_WriteWRAM) {
	if (WRAMSIZE) {
		A = 0x6000 | ((A - 0x6000) & (WRAMSIZE - 1));
		CartBW(A, V);
	} else if (vrc24.type == VRC24_VRC2) {
		vrc24.wire = V;
		if (VRC24_SyncWires) {
			VRC24_SyncWires();
		}
	}
}

DECLFW(VRC24_Write) {
	uint8_t index, mask;

	switch (A & 0xF000) {
	case 0x8000:
	case 0xA000:
		vrc24.prg[(A >> 13) & 0x01] = V;
		VRC24_SyncPRG();
		break;

	case 0x9000:
		mask = vrc24.type ? 0x03 : 0x00;
		index = ((A & vrc24.A1) ? 0x02 : 0x00) | ((A & vrc24.A0) ? 0x01 : 0x00);
		switch (index & mask) {
		case 0:
		case 1:
			if (V != 0xFF) {
				vrc24.mirr = V;
				VRC24_SyncMirror();
			}
			break;
		case 2:
			vrc24.cmd = V;
			VRC24_SyncPRG();
			break;
		case 3:
			if (VRC24_WriteExtSelect) {
				VRC24_WriteExtSelect(A, V);
			}
			break;
		}
		break;

	case 0xB000:
	case 0xC000:
	case 0xD000:
	case 0xE000:
		index = (((A - 0xB000) >> 11) & 0x06) | ((A & vrc24.A1) ? 0x01 : 0x00);
		if (A & vrc24.A0) {
			/* m25 can be 512K, rest are 256K or less */
			vrc24.chr[index] = (vrc24.chr[index] & 0x000F) | (V << 4);
		} else {
			vrc24.chr[index] = (vrc24.chr[index] & 0x0FF0) | (V & 0x0F);
		}
		VRC24_SyncCHR();
		break;

	case 0xF000:
		index = ((A & vrc24.A1) ? 0x02 : 0x00) | ((A & vrc24.A0) ? 0x01 : 0x00);
		switch (index) {
		case 0x00:
			VRCIRQ_LatchNibble(V, 0);
			break;
		case 0x01:
			VRCIRQ_LatchNibble(V, 1);
			break;
		case 0x02:
			VRCIRQ_Control(V);
			break;
		case 0x03:
			VRCIRQ_Acknowledge();
			break;
		}
		break;
	}
}

void VRC24_IRQCPUHook(int a) {
	VRCIRQ_CPUHook(a);
}

void VRC24_Reset(void) {
	vrc24.prg[0] = 0;
	vrc24.prg[1] = 1;

	vrc24.chr[0] = 0;
	vrc24.chr[1] = 1;
	vrc24.chr[2] = 2;
	vrc24.chr[3] = 3;
	vrc24.chr[4] = 4;
	vrc24.chr[5] = 5;
	vrc24.chr[6] = 6;
	vrc24.chr[7] = 7;

	vrc24.cmd = vrc24.mirr = 0;

	if (PRG_BANK_COUNT(16) & 0x01) {
		lastPRGBank = PRG_BANK_COUNT(8) - 1;
	} else {
		lastPRGBank = ~0;
	}

	VRC24_SyncPRG();
	VRC24_SyncCHR();
	VRC24_SyncMirror();
}

void VRC24_Power(void) {
	VRC24_Reset();

	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0x8000, 0xFFFF, VRC24_Write);

	SetReadHandler(0x6000, 0x7FFF, VRC24_ReadWRAM);
	SetWriteHandler(0x6000, 0x7FFF, VRC24_WriteWRAM);

	if (WRAMSIZE) {
		setprg8r(0x10, 0x6000, 0);
		FCEU_CheatAddRAM(WRAMSIZE >> 10, 0x6000, WRAM);
	}

	if (!ROM.chr.size) {
		setchr8(0);
	}
}

static void StateRestore(int version) {
	VRC24_SyncPRG();
	VRC24_SyncCHR();
	VRC24_SyncMirror();
}

void VRC24_Close(void) {
}

void VRC24_Init(CartInfo *info, VRC24TYPE _vrc4, uint32_t _A0, uint32_t _A1, int wram, int irqRepeated) {
	VRC24_SyncPRG = VRC24_SyncPRG_default;
	VRC24_SyncCHR = VRC24_SyncCHR_default;
	VRC24_SyncMirror = VRC24_SyncMirror_default;
	VRC24_SyncWires = NULL;

	VRC24_pwrap = GENPWRAP;
	VRC24_cwrap = GENCWRAP;
	VRC24_WriteExtSelect = NULL;

	vrc24.A0 = _A0;
	vrc24.A1 = _A1;
	vrc24.type = _vrc4;

	WRAMSIZE = 0;
	if (wram) {
		WRAMSIZE = iNESCart.iNES2 ? (info->PRGRamSize + info->PRGRamSaveSize) : 8192;

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

	AddExState(StateRegs, ~0, 0, NULL);

	info->Power = VRC24_Power;
	info->Close = VRC24_Close;
	GameStateRestore = StateRestore;

	if (vrc24.type == VRC24_VRC4) {
		VRCIRQ_Init(irqRepeated);
		MapIRQHook = VRCIRQ_CPUHook;
		AddExState(&VRCIRQ_StateRegs, ~0, 0, 0);
	}
}

void VRC2_SetConfig(uint8_t clear, uint32_t _A0, uint32_t _A1) {
	vrc24.A0 = _A0;
	vrc24.A1 = _A1;
	vrc24.type = VRC24_VRC2;
	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0x8000, 0xFFFF, VRC24_Write);
	SetReadHandler(0x6000, 0x7FFF, VRC24_ReadWRAM);
	SetWriteHandler(0x6000, 0x7FFF, VRC24_WriteWRAM);
	if (clear) {
		VRC24_Reset();
	} else {
		VRC24_SyncPRG();
		VRC24_SyncCHR();
		VRC24_SyncMirror();
	}
}

void VRC4_SetConfig(uint8_t clear, uint32_t _A0, uint32_t _A1, int irqRepeated) {
	vrc24.A0 = _A0;
	vrc24.A1 = _A1;
	vrc24.type = VRC24_VRC4;
	VRCIRQ_Init(irqRepeated);
	MapIRQHook = VRCIRQ_CPUHook;
	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0x8000, 0xFFFF, VRC24_Write);
	SetReadHandler(0x6000, 0x7FFF, VRC24_ReadWRAM);
	SetWriteHandler(0x6000, 0x7FFF, VRC24_WriteWRAM);
	if (clear) {
		VRC24_Reset();
	} else {
		VRC24_SyncPRG();
		VRC24_SyncCHR();
		VRC24_SyncMirror();
	}
}
