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

/* NES 2.0 Mapper 308 - UNL-TH2131-1 */
/* https://wiki.nesdev.com/w/index.php/NES_2.0_Mapper_308
 * NES 2.0 Mapper 308 is used for a bootleg version of the Sunsoft game Batman
 * similar to Mapper 23 Submapper 3) with custom IRQ functionality.
 * UNIF board name is UNL-TH2131-1.
 */

#include "mapinc.h"
#include "vrc24.h"

static struct {
	uint8_t IRQCountHigh, IRQa;
	uint16_t IRQCountLow;
} m308;

static SFORMAT StateRegs[] = {
	{ &m308.IRQCountLow, 2 | FCEUSTATE_RLSB, "IRQL" },
	{ &m308.IRQCountHigh, 1, "IRQH" },
	{ &m308.IRQa, 1, "IRQA" },
	{ 0 }
};

static DECLFW(WriteIRQ) {
	switch (A & 0xF003) {
	case 0xF000:
		X6502_IRQEnd(FCEU_IQEXT);
		m308.IRQa = 0;
		m308.IRQCountLow = 0;
		break;
	case 0xF001:
		m308.IRQa = 1;
		break;
	case 0xF003:
		m308.IRQCountHigh = (V & 0xF0) >> 4;
		break;
	}
}

static void CPUIRQHook(int a) {
	uint16_t prev, curr;

	if (m308.IRQa) {
		prev = m308.IRQCountLow & 0x0FFF;
		m308.IRQCountLow += a;
		curr = m308.IRQCountLow & 0x0FFF;

		if (!(prev & 0x800) && (curr & 0x800)) {
			m308.IRQCountHigh--;
		}
		if ((m308.IRQCountHigh == 0) && !(curr & 0x800)) {
			X6502_IRQBegin(FCEU_IQEXT);
		}
	}
}

static void Power(void) {
	memset(&m308, 0, sizeof(m308));
	VRC24_Power();
	SetWriteHandler(0xF000, 0xFFFF, WriteIRQ);
}

void Mapper308_Init(CartInfo *info) {
	VRC24_Init(info, VRC24_VRC2, 0x01, 0x02, 0, 1);
	info->Power = Power;
	MapIRQHook = CPUIRQHook;
	AddExState(StateRegs, ~0, 0, NULL);
}
