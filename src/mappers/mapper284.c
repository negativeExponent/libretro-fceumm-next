/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
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

/* FIXME:
 * The extender nametable mirroring only works in NEWPPU.
 */

#include "mapinc.h"
#include "dripsound.h"

static struct {
	uint8_t extattrib[2][1024];
	uint8_t jumper;
	uint8_t control;
	uint8_t prg;
	uint8_t chr[4];
	uint8_t IRQa;
	uint8_t IRQLatch;
	int16_t IRQCount;
	uint16_t lastAddr;
} m284;

static uint8_t DRIPHack = FALSE;

static SFORMAT StateRegs[] = {
	{ &m284.jumper, 1, "JUMP" },
	{ &m284.control, 1, "CTRL" },
	{ &m284.prg, 1, "PREG" },
	{ m284.chr, 4, "CREG" },
	{ &m284.IRQa, 1, "IRQA" },
	{ &m284.IRQLatch, 1, "IRQL" },
	{ &m284.IRQCount, 2, "IRQC" },
	{ &m284.lastAddr, 2 | FCEUSTATE_RLSB, "LADD" },
	{ 0 }
};

static void SyncPRG(void) {
	setprg16(0x8000, m284.prg);
	setprg16(0xC000, 0x0F);
}

static void SyncCHR(void) {
	setchr2(0x0000, m284.chr[0]);
	setchr2(0x0800, m284.chr[1]);
	setchr2(0x1000, m284.chr[2]);
	setchr2(0x1800, m284.chr[3]);
}

static void SyncMirror(void) {
	switch (m284.control & 0x03) {
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

static void SyncWRAM(void) {
	if (m284.control & 0x08) {
		setprg8r(0x10, 0x6000, 0);
	}
}

static DECLFR(Read4) {
	switch (A & 0xF800) {
	case 0x4000:
		return cpu.openbus;
	case 0x4800:
		return (m284.jumper | 'd');
	}
}

static DECLFR(Read5) {
	switch (A & 0xF800) {
	case 0x5000:
	case 0x5800:
		return DRIPSound_Read(A);
	}
}

static DECLFW(WriteL) {
	if (A & 0x08) { /* $xxx8 - $xxxF */
		switch (A & 0x07) {
		case 0:
			m284.IRQLatch = V;
			break;
		case 1:
			m284.IRQCount = ((V & 0x7F) << 8) | m284.IRQLatch;
			m284.IRQa = V & 0x80;
			X6502_IRQEnd(FCEU_IQEXT);
			break;
		case 2:
			m284.control = V & 0x0F;
			SyncMirror();
			SyncWRAM();
			break;
		case 3:
			m284.prg = V & 0x0F;
			SyncPRG();
			break;
		case 4:
		case 5:
		case 6:
		case 7:
			m284.chr[A & 0x03] = V & 0x0F;
			SyncCHR();
			break;
		}
	} else { /* $xxx0 - $xxx7*/
		switch (A & 0x07) {
		case 0:
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
		case 7:
			DRIPSound_Write(A, V);
			break;
		}

	}
}

static DECLFW(WriteH) {
	uint8_t idx = (A & 0x400) >> 10;
	m284.extattrib[idx][A & 0x3FF] = V & 0x03;
}

static uint8_t newppu_PPUNMTRead(uint32_t A) {
	if ((A > 0x2000) && (A < 0x3F00)) {
		if (m284.control & 0x04) {
			if ((A & 0x3FF) < 0x3C0) {
				m284.lastAddr = A & 0x3FF;
			} else {
				const uint8_t ext_attrib[4] = { 0x00, 0x55, 0xAA, 0xFF };
				uint8_t bank = 0;

				A &= 0x0FFF;

				switch (iNESCart.mirror) {
				default:
				case MI_0:
					bank = 0;
					break;
				case MI_1:
					bank = 1;
					break;
				case MI_V:
					bank = (A & 0x800) ? 1 : 0;
					break;
				case MI_H:
					bank = (A & 0x400) ? 1 : 0;
					break;
				}
				return (ext_attrib[(m284.extattrib[bank][m284.lastAddr & 0x3FF]) & 0x03]);
			}
		}
	}
	return FFCEUX_PPURead_Default(A);
}

static void Reset(void) {
	m284.jumper = !m284.jumper ? 0x80 : 0;

	SyncPRG();
	SyncCHR();
	SyncMirror();
	SyncWRAM();
}

static void Power(void) {
	memset(&m284, 0, sizeof(m284));
	m284.chr[0] = 0;
	m284.chr[1] = 1;
	m284.chr[2] = 2;
	m284.chr[3] = 3;

	SyncPRG();
	SyncCHR();
	SyncMirror();
	SyncWRAM();

	SetReadHandler(0x4800, 0x4FFF, Read4);
	SetReadHandler(0x5000, 0x5FFF, Read5);
	SetReadHandler(0x6000, 0xFFFF, CartBR);
	SetWriteHandler(0x6000, 0x7FFF, CartBW);
	SetWriteHandler(0x8000, 0xBFFF, WriteL);
	SetWriteHandler(0xC000, 0xFFFF, WriteH);

	FFCEUX_PPURead = newppu_PPUNMTRead;
}

static void CPUIRQHook(int a) {
	if (m284.IRQa) {
		m284.IRQCount -= a;
		if (m284.IRQCount <= 0) {
			m284.IRQa = FALSE;
			X6502_IRQBegin(FCEU_IQEXT);
		}
	}
}

static void StateRestore(int version) {
	SyncPRG();
	SyncCHR();
	SyncMirror();
	SyncWRAM();
}

void Mapper284_Init(CartInfo *info) {
	info->Power = Power;
	MapIRQHook = CPUIRQHook;

	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);
	AddExState(m284.extattrib[0], 1024, 0, "ATT0");
	AddExState(m284.extattrib[1], 1024, 0, "ATT1");

	WRAMSIZE = 8192;
	WRAM = FCEU_malloc(WRAMSIZE);
	SetupCartPRGMapping(0x10, WRAM, WRAMSIZE, TRUE);
	AddExState(WRAM, WRAMSIZE, 0, "WRAM");

	DRIPSound_ESI();
	DRIPSound_AddStateInfo();

	DRIPHack = TRUE;

	FCEU_printf(" DRIPGAME Mapper warning: Set video to use NEWPPU for game to work properly.\n");
}
