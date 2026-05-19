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

/* Mapper 369 (BMC-N49C-300) - Super Mario Bros. Party multicart */

#include "mapinc.h"
#include "mmc3.h"

static struct {
	uint8_t reg;
	uint8_t smb2j;
	uint8_t IRQa;
	uint16_t IRQCount;
} m369;

static SFORMAT StateRegs[] = {
	{ &m369.reg, 1, "MODE" },
	{ &m369.smb2j, 1, "SMB2" },
	{ &m369.IRQa, 1, "MIQA" },
	{ &m369.IRQCount, 2 | FCEUSTATE_RLSB, "MIQC" },
	{ 0 }
};

static void SetPRG(uint16_t A, uint16_t V) {
	uint8_t mask = (m369.reg == 0xFF) ? 0x1F : 0x0F;
	uint8_t base = (m369.reg == 0xFF) ? 0x20 : 0x10;

	setprg8(A, (base & ~mask) | (V & mask));
}

static void SetCHR(uint16_t A, uint16_t V) {
	uint16_t mask = (m369.reg == 0xFF) ? 0xFF : 0x7F;
	uint16_t base = (m369.reg == 0xFF) ? 0x100 : 0x80;

	setchr1(A, (base & ~mask) | (V & mask));
}

static void Sync(void) {
	switch (m369.reg) {
	case 0x00:
	case 0x01: /* NROM */
		setprg32(0x8000, m369.reg);
		setchr8(m369.reg & 0x03);
		break;
	case 0x13: /* SMB2J */
		setprg8r(0, 0x6000, 0x0E);
		setprg8(0x8000, 0x0C);
		setprg8(0xA000, 0x0D);
		setprg8(0xC000, 0x08 | (m369.smb2j & 0x03));
		setprg8(0xE000, 0x0F);
		setchr8(m369.reg & 0x03);
		break;
	case 0x37: /* MMC3: 128 KiB CHR */
	case 0xFF: /* MMC3: 256 KiB CHR */
		setprg8r(0x10, 0x6000, 0);
		MMC3_SyncPRG();
		MMC3_SyncCHR();
	}
}

static DECLFW(WriteReg) {
	if (A & 0x100) {
		m369.reg = V;
		Sync();
	}
}

static DECLFW(Write89) {
	if (m369.reg == 0x13) {
		m369.IRQa = 0;
		X6502_IRQEnd(FCEU_IQEXT);
	}
	switch (A & 0xE001) {
	case 0x8000:
		mmc3.cmd = V;
		Sync();
		break;
	case 0x8001:
		mmc3.reg[mmc3.cmd & 0x07] = V;
		Sync();
	}
}

static DECLFW(WriteAB) {
	if (m369.reg == 0x13) {
		m369.IRQa = (V & 0x02) != 0;
	}
	MMC3_Write(A, V);
}

static DECLFW(WriteEF) {
	if (m369.reg == 0x13) {
		m369.smb2j = V;
		Sync();
	}
	MMC3_Write(A, V);
}

static void CPUIRQHook(int a) {
	if (m369.reg == 0x13) {
		if (m369.IRQa) {
			m369.IRQCount += a;
			if (m369.IRQCount >= 4096) {
				m369.IRQCount -= 4096;
				X6502_IRQBegin(FCEU_IQEXT);
			}
		}
	}
}

static void HBIRQHook(void) {
	if (m369.reg != 0x13) {
		MMC3_IRQHBHook();
	}
}

static void Reset(void) {
	memset(&m369, 0, sizeof(m369));
	MMC3_Reset();
	Sync();
}

static void Power(void) {
	memset(&m369, 0, sizeof(m369));
	MMC3_Power();
	SetWriteHandler(0x4100, 0x4FFF, WriteReg);
	SetWriteHandler(0x8000, 0x9FFF, Write89);
	SetWriteHandler(0xA000, 0xBFFF, WriteAB);
	SetWriteHandler(0xE000, 0xFFFF, WriteEF);
	Sync();
}

static void StateRestore(int version) {
	Sync();
}

void Mapper369_Init(CartInfo *info) {
	MMC3_Init(info, MMC3B, 8, 0);
	MMC3_pwrap = SetPRG;
	MMC3_cwrap = SetCHR;
	info->Power = Power;
	info->Reset = Reset;
	MapIRQHook = CPUIRQHook;
	GameHBIRQHook = HBIRQHook;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);
}
