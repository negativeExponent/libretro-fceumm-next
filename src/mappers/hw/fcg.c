/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2007 CaH4e3
 *  Copyright (C) 2011 FCEUX team
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
 * INES Mapper 016
 * iNES Mapper 016 is used for some of the Bandai FCG boards, namely, boards with
 * the FCG-1 ASIC that supports no EEPROM, and the LZ93D50 ASIC with no or 256 bytes of EEPROM.
 *
 * INES Mapper 016 submapper table
 * Submapper #	Meaning											Note
 * 0			Unspecified										Emulate both FCG-1/2 and LZ93D50 chips in their respective CPU address ranges.
 * 1			LZ93D50 with 128 byte serial EEPROM (24C01)		Deprecated, use INES Mapper 159 instead.
 * 2			Datach 	Joint ROM System						Deprecated, use INES Mapper 157 instead.
 * 3			8 KiB of WRAM instead of serial EEPROM			Deprecated, use INES Mapper 153 instead.
 * 4			FCG-1/2											Responds only in the CPU $6000-$7FFF address range; IRQ counter is not latched.
 * 5			LZ93D50 with no or 256-byte serial EEPROM (24C02)		Responds only in the CPU $8000-$FFFF address range; IRQ counter is latched.
 *
 */

#include "mapinc.h"
#include "eeprom_24C0x.h"
#include "fcg.h"

static X24C0X *eeprom = NULL;

static uint8_t FCGType = FCG_TYPE_Unknown;

void (*FCG_pwrap)(uint16_t A, uint16_t V);
void (*FCG_cwrap)(uint16_t A, uint16_t V);

static FCG fcg;

static SFORMAT StateRegs[] = {
	{ &fcg.prg, 1, "PREG" },
	{ fcg.chr, 8, "CREG" },
	{ &fcg.mirror, 1, "MIRR" },
	{ &fcg.wramEnabled, 1, "WREN" },
	{ &fcg.IRQa, 1, "IRQA" },
	{ &fcg.IRQCount, 2 | FCEUSTATE_RLSB, "IRQC" },
	{ &fcg.IRQLatch, 2 | FCEUSTATE_RLSB, "IRQL" }, /* need for Famicom Jump II - Saikyou no 7 Nin (J) [!] */
	{ 0 }
};

static void FCG_SetPRG_default(uint16_t A, uint16_t V) {
	setprg16(A, V & 0x0F);
}

static void FCG_SetCHR_default(uint16_t A, uint16_t V) {
	setchr1(A, V);
}

void FCG_SyncPRG(void) {
	FCG_pwrap(0x8000, fcg.prg);
	FCG_pwrap(0xC000, ~0);
}

void FCG_SyncCHR(void) {
	FCG_cwrap(0x0000, fcg.chr[0]);
	FCG_cwrap(0x0400, fcg.chr[1]);
	FCG_cwrap(0x0800, fcg.chr[2]);
	FCG_cwrap(0x0C00, fcg.chr[3]);
	FCG_cwrap(0x1000, fcg.chr[4]);
	FCG_cwrap(0x1400, fcg.chr[5]);
	FCG_cwrap(0x1800, fcg.chr[6]);
	FCG_cwrap(0x1C00, fcg.chr[7]);
}

void FCG_SyncMirror(void) {
	switch (fcg.mirror & 0x03) {
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

void FCG_SyncWRAM(void) {
	if (fcg.wramEnabled) {
		setprg8r(0x10, 0x6000, 0);
	} else {
		unsetcpu8(0x6000);
	}
}

DECLFR(FCG_Read) {
	if (eeprom) {
		return (cpu.openbus & ~0x10) | eeprom_read(eeprom);
	}
	return cpu.openbus;
}

DECLFW(FCG_Write) {
	switch (A & 0x0F) {
	case 0x00:
	case 0x01:
	case 0x02:
	case 0x03:
	case 0x04:
	case 0x05:
	case 0x06:
	case 0x07:
		fcg.chr[A & 0x07] = V;
		FCG_SyncCHR();
		break;
	case 0x08:
		fcg.prg = V;
		FCG_SyncPRG();
		break;
	case 0x09:
		fcg.mirror = V;
		FCG_SyncMirror();
		break;
	case 0x0A:
		fcg.IRQa = V & 0x01;
		if (FCGType != FCG_TYPE_FCG) {
			fcg.IRQCount = fcg.IRQLatch;
		}
		if (fcg.IRQa && !fcg.IRQCount) {
			X6502_IRQBegin(FCEU_IQEXT);
		} else {
			X6502_IRQEnd(FCEU_IQEXT);
		}
		break;
	case 0x0B:
		if (FCGType == FCG_TYPE_FCG) {
			fcg.IRQCount &= 0xFF00;
			fcg.IRQCount |= V;
		} else {
			fcg.IRQLatch &= 0xFF00;
			fcg.IRQLatch |= V;
		}
		break;
	case 0x0C:
		if (FCGType == FCG_TYPE_FCG) {
			fcg.IRQCount &= 0x00FF;
			fcg.IRQCount |= V << 8;
		} else {
			fcg.IRQLatch &= 0x00FF;
			fcg.IRQLatch |= V << 8;
		}
		break;
	case 0x0D:
		if (eeprom) {
			eeprom_i2c_step(eeprom, (V & 0x20) >> 5, (V & 0x40) >> 6);
		} else if (FCGType != FCG_TYPE_FCG) {
			fcg.wramEnabled = V & 0x20;
			FCG_SyncWRAM();
		}
		break;
	default:
		break;
	}
}

void FCG_CPUIRQHook(int a) {
	if (fcg.IRQa) {
		fcg.IRQCount -= a;
		if (fcg.IRQCount < 0) {
			X6502_IRQBegin(FCEU_IQEXT);
		}
	}
}

static void StateRestore(int version) {
	FCG_SyncPRG();
	FCG_SyncCHR();
	FCG_SyncMirror();
	FCG_SyncWRAM();
}

void FCG_Power(void) {
	memset(&fcg, 0, sizeof(fcg));

	fcg.chr[0] = 0;
	fcg.chr[1] = 1;
	fcg.chr[2] = 2;
	fcg.chr[3] = 3;
	fcg.chr[4] = 4;
	fcg.chr[5] = 5;
	fcg.chr[6] = 6;
	fcg.chr[7] = 7;

	FCG_SyncPRG();
	FCG_SyncCHR();
	FCG_SyncMirror();
	FCG_SyncWRAM();

	SetReadHandler(0x8000, 0xFFFF, CartBR);
	if (eeprom) {
		SetReadHandler(0x6000, 0x7FFF, FCG_Read);
	}
	SetWriteHandler(
		((FCGType == FCG_TYPE_LZ93D50) ? 0x8000 : 0x6000),
		((FCGType == FCG_TYPE_FCG    ) ? 0x7FFF : 0xFFFF),
		FCG_Write);
}

void FCG_Init(CartInfo *info, uint8_t _FCGType) {
	FCG_pwrap = FCG_SetPRG_default;
	FCG_cwrap = FCG_SetCHR_default;
	FCGType = _FCGType;

	info->Power = FCG_Power;
	MapIRQHook = FCG_CPUIRQHook;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);

	eeprom = NULL;
}

void FCG_SetEeprom(X24C0X *e) {
	eeprom = e;
}
