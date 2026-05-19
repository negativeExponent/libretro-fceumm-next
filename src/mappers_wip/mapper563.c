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
	} irq;
} m563;

static SFORMAT StateRegs[] = {
	{ &m563.irq.enabled, 1, "IQEN" },
	{ &m563.irq.counter, 1, "IQCN" },
	{ 0 }
};

static void SetPRG(uint16_t A, uint16_t V) {
	setprg8(A, V & 0x1F);
}

static void SetCHR(uint16_t A, uint16_t V) {
	setchr1(A, V & 0x1FF);
}

static DECLFW(WriteIRQ) {
	X6502_IRQEnd(FCEU_IQEXT);
	switch (A & 0x1C) {
	case 0x0C:
		m563.irq.enabled = FALSE;
		break;
	case 0x08:
		m563.irq.enabled = TRUE;
		break;
	case 0x1C:
		m563.irq.counter = 0;
		break;
	}
}

static void HBIRQHook(void) {
	if (!(++m563.irq.counter & 0x01) && m563.irq.enabled) {
		X6502_IRQBegin(FCEU_IQEXT);
	}
}

static void Power(void) {
	memset(&m563, 0, sizeof(m563));
	VRC24_Power();
	SetWriteHandler(0xF000, 0xFFFF, WriteIRQ);
}

void Mapper563_Init(CartInfo *info) {
	VRC24_Init(info, VRC24_VRC2, 0x01, 0x02, 0, FALSE);
	VRC24_pwrap = SetPRG;
	VRC24_cwrap = SetCHR;
	info->Power = Power;
	GameHBIRQHook = HBIRQHook;
	AddExState(StateRegs, ~0, 0, 0);
}
