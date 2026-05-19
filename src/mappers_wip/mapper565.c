/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2025-2026 negativeExponent
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
		uint8_t enabled;
		uint8_t counter;
		uint8_t prescaler;
	} irq;
} m565;

static SFORMAT StateRegs[] ={
	{ &m565.irq.enabled, 1, "IQEN" },
	{ &m565.irq.counter, 1, "IQCN" },
	{ &m565.irq.prescaler, 1, "IQPS" },
	{ 0 }
};

static void SetPRG(uint16_t A, uint16_t V) {
	setprg8(A, V & 0x1F);
}

static void SetCHR(uint16_t A, uint16_t V) {
	setchr1(A, V & 0x1FF);
}

static DECLFW(WriteIRQ) {
	switch (A & 0x0C) {
	case 0:
		m565.irq.counter = V;
		m565.irq.prescaler = 0;
		X6502_IRQEnd(FCEU_IQEXT);
		break;
	case 4:
		m565.irq.enabled = (V & 0x01) == 0x01;
		break;
	}
}

static void CPUIRQHook(int a) {
	while (a--) {
		if (m565.irq.enabled) {
			m565.irq.prescaler++;
			if ((m565.irq.prescaler == 64) && !(++m565.irq.counter)) {
				X6502_IRQBegin(FCEU_IQEXT);
			}
			if (m565.irq.prescaler == 112) {
				m565.irq.prescaler = 0;
			}
		} else {
			m565.irq.prescaler = 0;
			X6502_IRQEnd(FCEU_IQEXT);
		}
	}
}

static void Power(void) {
	memset(&m565, 0, sizeof(m565));
	VRC24_Power();
	SetWriteHandler(0xF000, 0xFFFF, WriteIRQ);
}

void Mapper565_Init(CartInfo *info) {
	VRC24_Init(info, VRC24_VRC2, 0x08, 0x04, 0, FALSE);
	MapIRQHook = CPUIRQHook;
	info->Power = Power;
	AddExState(StateRegs, ~0, 0, 0);
}
