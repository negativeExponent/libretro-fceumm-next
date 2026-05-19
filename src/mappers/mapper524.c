/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2007 CaH4e3
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

/* NES 2.0 Mapper 524 - BTL-900218 */
/* http://wiki.nesdev.com/w/index.php/UNIF/900218
 * NES 2.0 Mapper 524 describes the PCB used for the pirate port Lord of King or Axe of Fight.
 * UNIF board name is BTL-900218.
 */

#include "mapinc.h"
#include "vrc24.h"

static struct {
	uint8_t IRQa;
	uint16_t IRQCount;
} m524;

static SFORMAT StateRegs[] = {
	{ &m524.IRQCount, 2 | FCEUSTATE_RLSB, "IRQC" },
	{ &m524.IRQa, 1, "IRQA" },
	{ 0 }
};

static DECLFW(WriteIRQ) {
	switch (A & 0xF00C) {
	case 0xF008:
		m524.IRQa = TRUE;
		break;
	case 0xF00C:
		m524.IRQa = FALSE;
		m524.IRQCount = 0;
		X6502_IRQEnd(FCEU_IQEXT);
		break;
	}
}

static void CPUIRQHook(int a) {
	if (m524.IRQa) {
		m524.IRQCount += a;
		if (m524.IRQCount & 1024) {
			X6502_IRQBegin(FCEU_IQEXT);
		}
	}
}

static void Power(void) {
	memset(&m524, 0, sizeof(m524));
	VRC24_Power();
	SetWriteHandler(0xF000, 0xFFFF, WriteIRQ);
}

void Mapper524_Init(CartInfo *info) {
	VRC24_Init(info, VRC24_VRC2, 0x01, 0x02, 0, 1);
	info->Power = Power;
	MapIRQHook = CPUIRQHook;
	AddExState(StateRegs, ~0, 0, NULL);
}
