/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2005 CaH4e3
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
 *
 * iNES Mapper 222 - (CTC-31?) (VRC2 mapper)
 * submapper 1 - Dragon Ninja (CTC-31)
 * Submapper 2 = Super Bros. 8 (JY211 rev0)
 * Submapper 2 = Super Bros. 8 (JY211 rev1)
 *
 */

#include "mapinc.h"
#include "vrc24.h"

enum { IRQ_LOAD_MODE = 0, IRQ_CLOCK_MODE };

static struct {
	uint8_t IRQPrescaler;
	uint8_t IRQMode;
	uint8_t IRQCount[2];
	uint8_t IRQPending;
} m222;

static SFORMAT StateRegs[] = {
	{ &m222.IRQPrescaler, 1, "IRQP" },
	{ &m222.IRQMode, 1, "IRQM" },
	{ &m222.IRQCount[0], 1, "IQC0" },
	{ &m222.IRQCount[1], 1, "IQC1" },
	{ &m222.IRQPending, 1, "IQPN" },
	{ 0 }
};

static void SetPRGBank(uint16_t A, uint16_t V) {
	setprg8(A, V & 0x1F);
}

static void SetCHRBank(uint16_t A, uint16_t V) {
	setchr1(A, V & 0xFFF);
}

static DECLFW(WriteCHR) {
	if (!(A & 0x0001)) {
		VRC24_Write(A, V);
		VRC24_Write(A | 0x0001, V >> 4);
	}
}

static DECLFW(WriteIRQ) {
	switch (A & 0xF003) {
	case 0xF000:
		m222.IRQMode = IRQ_LOAD_MODE;
		break;
	case 0xF001:
		X6502_IRQEnd(FCEU_IQEXT);
		m222.IRQPending = FALSE;
		if (m222.IRQMode == IRQ_LOAD_MODE) {
			m222.IRQCount[0] = V & 0x0F;
			m222.IRQCount[1] = V >> 4;
		}
		break;
	case 0xF002:
		m222.IRQMode = IRQ_CLOCK_MODE;
		break;
	}
}

static void CPUIRQHook(int a) {
	while (a--) {
		uint8_t prevPrescaler = m222.IRQPrescaler;
		if (m222.IRQPending) {
			m222.IRQPrescaler = 0;
		} else {
			m222.IRQPrescaler++;
		}
		if ((m222.IRQMode == IRQ_CLOCK_MODE) && !(prevPrescaler & 0x40) && (m222.IRQPrescaler & 0x40)) {
			m222.IRQCount[0]++;
			if (m222.IRQCount[0] == 0x0F) {
				m222.IRQCount[1]++;
				if (m222.IRQCount[1] == 0x0F) {
					X6502_IRQBegin(FCEU_IQEXT);
					m222.IRQPending = TRUE;
				}
			}
			m222.IRQCount[0] &= 0x0F;
			m222.IRQCount[1] &= 0x0F;
		}
	}
}

static void Power(void) {
	memset(&m222, 0, sizeof(m222));
	m222.IRQMode = IRQ_LOAD_MODE;
	VRC24_Power();
	SetWriteHandler(0xB000, 0xEFFF, WriteCHR);
	SetWriteHandler(0xF000, 0xFFFF, WriteIRQ);
}

void Mapper222_Init(CartInfo *info) {
	VRC24_Init(info, VRC24_VRC2, 0x01, 0x02, 0, 1);
	VRC24_pwrap = SetPRGBank;
	VRC24_cwrap = SetCHRBank;
	info->Power = Power;
	MapIRQHook = CPUIRQHook;
	AddExState(StateRegs, ~0, 0, NULL);
}
