/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2012 CaH4e3
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
 * FDS Conversion
 * - Submapper 1 - Ai Senshi Nicol (256K PRG, 128K CHR, fixed Mirroring)
 * - Submapper 3 - Bio Miracle Bokutte Upa (J) (128K PRG, 0K CHR, IRQ)
 *
 * - Submapper 2
 * - KS-018/AC-08/LH09
 * - UNIF UNL-AC08
 * - [UNIF] Green Beret (FDS Conversion, LH09) (Unl) [U][!][t1] (160K PRG)
 * - Green Beret (FDS Conversion) (Unl) (256K PRG)
 */

#include "mapinc.h"
#include "fdssound.h"

static void (*WSync)(void);

static struct {
	uint8_t reg[2];
	uint8_t IRQa;
	uint16_t IRQCount;
} m042;

static SFORMAT StateRegs[] = {
	{ m042.reg, 2, "REGS" },
	{ &m042.IRQa, 1, "IRQA" },
	{ &m042.IRQCount, 2 | FCEUSTATE_RLSB, "IRQC" },
	{ 0 }
};

/* Submapper 1 - Ai Senshi Nicol */

static void Sync_sub1(void) {
	setprg8(0x6000, m042.reg[1] & 0x0F);
	setprg32(0x8000, ~0);
	setchr8(m042.reg[0] & 0x0F);
}

static DECLFW(WriteReg_sub1) {
	switch (A & 0xE000) {
	case 0x8000:
		m042.reg[0] = V;
		Sync_sub1();
		break;
	case 0xE000:
		m042.reg[1] = V;
		Sync_sub1();
		break;
	}
}

static void Power_sub1(void) {
	m042.reg[1] = 0;
	m042.reg[0] = 0;
	FDSSound_Power();
	Sync_sub1();
	SetReadHandler(0x6000, 0xFFFF, CartBR);
	SetWriteHandler(0x8000, 0xFFFF, WriteReg_sub1);
}

/* Submapper 2 - Green Beret */

static void Sync_sub2(void) {
	setprg8(0x6000, (m042.reg[0] >> 1) & 0x0F);
	setprg32(0x8000, (PRG_BANK_COUNT(16) & 0x07) ? 4 : 7);
	setchr8(0);
	setmirror(((m042.reg[1] >> 3) & 1) ^ 1);
}

static DECLFW(WriteReg_sub2) {
	switch (A & 0xF001) {
	case 0x4001:
	case 0x4000:
		if ((A & 0xFF) != 0x25) {
			break;
		}
		m042.reg[1] = V;
		Sync_sub2();
		break;
	case 0x8001:
		m042.reg[0] = V;
		Sync_sub2();
		break;
	}
}

static void Power_sub2(void) {
	m042.reg[0] = 0;
	m042.reg[1] = 0;
	Sync_sub2();
	SetReadHandler(0x6000, 0xFFFF, CartBR);
	SetWriteHandler(0x4020, 0xFFFF, WriteReg_sub2);
}

/* Submapper 3 - Mario Baby */

static void Sync_sub3(void) {
	setprg8(0x6000, m042.reg[0] & 0x0F);
	setprg32(0x8000, ~0);
	setchr8(0);
	setmirror(((m042.reg[1] >> 3) & 1) ^ 1);
}

static DECLFW(WriteReg_sub3) {
	switch (A & 0xE003) {
	case 0xE000:
		m042.reg[0] = V;
		Sync_sub3();
		break;
	case 0xE001:
		m042.reg[1] = V;
		Sync_sub3();
		break;
	case 0xE002:
		m042.IRQa = V;
		if (!(m042.IRQa & 0x02)) {
			m042.IRQCount = 0;
			X6502_IRQEnd(FCEU_IQEXT);
		}
		break;
	}
}

static void CPUIRQHook_sub3(int a) {
	if (m042.IRQa & 0x02) {
		while (a--) {
			if ((++m042.IRQCount & 0x6000) == 0x6000) {
				X6502_IRQBegin(FCEU_IQEXT);
			} else {
				X6502_IRQEnd(FCEU_IQEXT);
			}
		}
	}
}

static void Power_sub3(void) {
	m042.reg[0] = 0;
	m042.reg[1] = 0;
	m042.IRQa = m042.IRQCount = 0;
	Sync_sub3();
	FDSSound_Power();
	SetReadHandler(0x6000, 0xFFFF, CartBR);
	SetWriteHandler(0xE000, 0xFFFF, WriteReg_sub3);
}

/* Mapper common */

static void StateRestore(int version) {
	switch (iNESCart.submapper) {
	case 1: Sync_sub1(); break;
	case 2: Sync_sub2(); break;
	default: Sync_sub3(); break;
	}
}

void Mapper042_Init(CartInfo *info) {
	if (info->submapper == 0 || info->submapper > 3) {
		if (ROM.chr.size) {
			/* Ai Senshi Nicol, only cart with CHR-ROM, all others use CHR-RAM */
			info->submapper = 1;
		} else {
			if (ROM.prg.size > (128 * 1024)) {
				/* Green Beret LH09 FDS Conversion can be 160K or 256K */
				info->submapper = 2;
			} else {
				/* Mario Baby has only 128K PRG */
				info->submapper = 3;
			}
		}
	}

	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);

	switch (info->submapper) {
	case 1: info->Power = Power_sub1; break;
	case 2: info->Power = Power_sub2; break;
	default:
		info->Power = Power_sub3;
		MapIRQHook = CPUIRQHook_sub3;
		break;
	}
}
