/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2025 negativeExponent
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include "mapinc.h"
#include "vrc24.h"

static struct {
	struct {
		uint8 enabled;
		uint8 counter;
		uint8 prescaler;
		uint8 mask;
	} irq;
} m273;

static SFORMAT StateRegs[] = {
	{ &m273.irq.enabled, 1, "IRQE" },
	{ &m273.irq.counter, 1, "CNTR" },
	{ &m273.irq.prescaler, 1, "IRQP" },
	{ &m273.irq.mask, 1, "IRQM" },
	{ 0 }
};

static void SetPRGBank_vrc2(uint16 A, uint16 V) {
	setprg8(A, (V & 0x1F));
}

static void SetCHRBank_vrc2(uint16 A, uint16 V) {
	setchr1(A, (V & 0x1FF));
}

static DECLFW(WriteIRQ) {
	switch (A & 0x08) {
	case 0:
		m273.irq.counter = V;
		X6502_IRQEnd(FCEU_IQEXT);
		break;
	case 8:
		m273.irq.enabled = (V & 0x01) != 0;
		if (!m273.irq.enabled) {
			m273.irq.prescaler = 0;
			m273.irq.mask = 0x7F;
			X6502_IRQEnd(FCEU_IQEXT);
		}
		break;
	}
}

static void CPUCycle(int a) {
	if (!m273.irq.enabled) {
		return;
	}
	while (a--) {
		if (!(++m273.irq.prescaler & m273.irq.mask)) {
			m273.irq.mask = 0xFF;
			if (!++m273.irq.counter) {
				X6502_IRQBegin(FCEU_IQEXT);
			} else {
				X6502_IRQEnd(FCEU_IQEXT);
			}
		}
	}
}

static void Power(void) {
	memset(&m273, 0, sizeof(m273));
	VRC24_Power();
	SetWriteHandler(0xF000, 0xFFFF, WriteIRQ);
}

void Mapper273_Init(CartInfo *info) {
	VRC24_Init(info, VRC24_VRC2, 0x04, 0x08, 0, 0);
	info->Power = Power;
	MapIRQHook = CPUCycle;
	AddExState(StateRegs, ~0, 0, 0);
}
