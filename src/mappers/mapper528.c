/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
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
 */

/* NES 2.0 Mapper 528
 * UNIF BMC-831128C
 */

#include "mapinc.h"
#include "fme7.h"
#include "vrcirq.h"

static struct {
	uint8_t reg;
} m528;

static SFORMAT StateRegs[] = {
	{ &m528.reg, 1, "REGS" },
	{ 0 }
};

static void SetPRG(uint16_t A, uint16_t V) {
	uint16_t base = m528.reg;
	uint16_t mask = base | 0x0F;

	setprg8(A, base + (V & mask));
}

static void SetCHR(uint16_t A, uint16_t V) {
	uint16_t mask = 0xFF;
	uint16_t base = m528.reg << 4;

	setchr1(A, (base | (V & mask)));
}

static void SycnWRAM(void) {
	uint16_t base = m528.reg;
	uint16_t mask = base | 0x0F;

	if (fme7.prg[0] == 1) {
		setprg8r(0x10, 0x6000, 0);
	} else {
		setprg8(0x6000, (base + (fme7.prg[0] & mask)));
	}
}

static DECLFW(WriteAC) {
	switch (A & 0x0F) {
	case 0x0B:
		break;
	case 0x0D:
		VRCIRQ_Control(V);
		break;
	case 0x0E:
		VRCIRQ_Acknowledge();
		break;
	case 0x0F:
		VRCIRQ_Latch(V);
		break;
	default:
		FME7_WriteIndex(0x8000, A & 0x0F);
		switch (fme7.cmd & 0x0F) {
		case 0x00:
		case 0x01:
		case 0x02:
		case 0x03:
		case 0x04:
		case 0x05:
		case 0x06:
		case 0x07:
			fme7.chr[fme7.cmd] = V;
			break;
		case 0x08:
		case 0x09:
		case 0x0A:
		case 0x0B:
			fme7.prg[fme7.cmd & 0x03] = V;
			break;
		case 0x0C:
			fme7.mirr = V;
			break;
		default:
			FME7_WriteReg(0x9000, V);
			break;
		}
		break;
	}
	m528.reg = (A & 0x4000) >> 10;
	FME7_SyncPRG();
	FME7_SyncWRAM();
	FME7_SyncCHR();
	FME7_SyncMirror();
}

static void Power(void) {
	memset(&m528, 0, sizeof(m528));
	FME7_Power();
	SetReadHandler(0x6000, 0x7FFF, CartBR);
	SetWriteHandler(0x6000, 0x7FFF, CartBW);
	SetWriteHandler(0xA000, 0xAFFF, WriteAC);
	SetWriteHandler(0xC000, 0xCFFF, WriteAC);
}

void Mapper528_Init(CartInfo *info) {
	FME7_Init(info, TRUE, info->battery);
	FME7_SyncWRAM = SycnWRAM;
	FME7_pwrap = SetPRG;
	FME7_cwrap = SetCHR;
	info->Power = Power;
	AddExState(StateRegs, ~0, 0, NULL);

	VRCIRQ_Init(TRUE);
	MapIRQHook = VRCIRQ_CPUHook;
	AddExState(&VRCIRQ_StateRegs, ~0, 0, 0);
}
