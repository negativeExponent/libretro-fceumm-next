/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2005 CaH4e3
 *  Copyright (C) 2023-2025 negativeExponent
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

static uint8 IRQPrescaler;
static uint8 IRQMode;
static uint8 IRQCount[2];
static uint8 IRQPending;

static SFORMAT StateRegs[] = {
	{ &IRQPrescaler, 1, "IRQP" },
	{ &IRQMode, 1, "IRQM" },
	{ &IRQCount[0], 1, "IQC0" },
	{ &IRQCount[1], 1, "IQC1" },
	{ &IRQPending, 1, "IQPN" },
	{ 0 }
};

static DECLFW(M222WriteCHR) {
	if (!(A & 0x0001)) {
		VRC24_Write(A, V);
		VRC24_Write(A | 0x0001, V >> 4);
	}
}

static DECLFW(M222WriteIRQ) {
	switch (A & 0xF003) {
	case 0xF000:
		IRQMode = IRQ_LOAD_MODE;
		break;
	case 0xF001:
		X6502_IRQEnd(FCEU_IQEXT);
		IRQPending = FALSE;
		if (IRQMode == IRQ_LOAD_MODE) {
			IRQCount[0] = V & 0x0F;
			IRQCount[1] = V >> 4;
		}
		break;
	case 0xF002:
		IRQMode = IRQ_CLOCK_MODE;
		break;
	}
}

static void M222CPUIRQHook(int a) {
	while (a--) {
		uint8 prevPrescaler = IRQPrescaler;
		if (IRQPending) {
			IRQPrescaler = 0;
		} else {
			IRQPrescaler++;
		}
		if ((IRQMode == IRQ_CLOCK_MODE) && !(prevPrescaler & 0x40) && (IRQPrescaler & 0x40)) {
			IRQCount[0]++;
			if (IRQCount[0] == 0x0F) {
				IRQCount[1]++;
				if (IRQCount[1] == 0x0F) {
					X6502_IRQBegin(FCEU_IQEXT);
					IRQPending = TRUE;
				}
			}
			IRQCount[0] &= 0x0F;
			IRQCount[1] &= 0x0F;
		}
	}
}

static void M222Power(void) {
	IRQMode = IRQ_LOAD_MODE;
	IRQPending = IRQCount[0] = IRQCount[1] = IRQPrescaler = 0;
	VRC24_Power();
	SetWriteHandler(0xB000, 0xEFFF, M222WriteCHR);
	SetWriteHandler(0xF000, 0xFFFF, M222WriteIRQ);
}

void Mapper222_Init(CartInfo *info) {
	VRC24_Init(info, VRC2, 0x01, 0x02, 0, 1);
	info->Power = M222Power;
	MapIRQHook = M222CPUIRQHook;
	AddExState(StateRegs, ~0, 0, NULL);
}
