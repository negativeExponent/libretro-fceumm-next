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

/* Mapper 394: HSK007 circuit board that can simulate J.Y. ASIC, MMC3, and NROM. */
/* submapper 0: Super Value HiK 6-in-1 (Top002) */
/* submapper 1: 6-in-1 (VIP002) (Unl) */

#include "mapinc.h"
#include "jyasic.h"
#include "mmc3.h"

static struct {
	uint8_t reg[4];
} m394;

static SFORMAT StateRegs[] = {
	{ m394.reg, 4, "EXPR" },
	{ 0 }
};

static uint32_t PRGBase(void) {
	return ((m394.reg[1] << 5) & 0x060) | ((m394.reg[3] << 1) & 0x010);
}

static uint32_t CHRBase(void) {
	return ((m394.reg[1] << 8) & 0x200) | ((m394.reg[1] << 6) & 0x100) | ((m394.reg[3] << 1) & 0x080);
}

static uint32_t PRGBank_JY(uint32_t V) {
	uint8_t base = PRGBase();

	return (base | (V & 0x1F));
}

static uint32_t CHRBank_JY(uint32_t V) {
	uint16_t base = CHRBase();

	return (base | (V & 0x0FF));
}

static void SetPRG_jy(uint16_t A, uint32_t V) {
	setprg8(A, PRGBank_JY(V));
}

static void SetCHR_jy(uint16_t A, uint32_t V) {
	setchr1(A, CHRBank_JY(V));
}

static void SetWRAM_jy(uint16_t A, uint32_t V) {
	setprg8(A, PRGBank_JY(V));
}

static void SetMirror_jy(uint16_t A, uint32_t V) {
	setntamem(CHRptr[0] + 0x400 * (CHRBank_JY(V) & CHRmask1[0]), 0, A);
}

static void SetPRG_mmc3(uint16_t A, uint16_t V) {
	uint8_t mask = (m394.reg[3] & 0x10) ? 0x1F : 0x0F;
	uint8_t base = PRGBase();

	if (m394.reg[1] & 0x08) {
		setprg8(A, base | (V & mask));
	} else {
		setprg32(0x8000, (base | ((m394.reg[3] << 1) & 0x0F)) >> 2);
	}
}

static void SetCHR_mmc3(uint16_t A, uint16_t V) {
	uint16_t mask = (m394.reg[3] & 0x80) ? 0xFF : 0x7F;
	uint16_t base = CHRBase();

	if (iNESCart.submapper != 1) {
		base = (((m394.reg[1] << 8) & 0x300) | ((m394.reg[3] << 1) & 0x080));
	}

	setchr1(A, (base & ~mask) | (V & mask));
}

static DECLFW(WriteReg) {
	uint8_t oldMode = m394.reg[1];

	A &= 3;
	m394.reg[A] = V;
	switch (A) {
	case 1:
		if (!(oldMode & 0x10) && (V & 0x10)) {
			JYASIC_Power();
		}
		if ((oldMode & 0x10) && !(V & 0x10)) {
			JYASIC_restoreWriteHandlers();
			MMC3_Power();
		}
		break;
	default:
		if (m394.reg[1] & 0x10) {
			JYASIC_SyncPRG();
			JYASIC_SyncCHR();
			JYASIC_SyncMirror();
		} else {
			MMC3_SyncPRG();
			MMC3_SyncCHR();
			MMC3_SyncMirror();
		}
		break;
	}
}

static void StateRestore(int version) {
	int i;

	JYASIC_restoreWriteHandlers();
	if (m394.reg[1] & 0x10) {
		SetWriteHandler(0x5000, 0x5FFF, JYASIC_WriteALU);
		SetWriteHandler(0x6000, 0x7fff, CartBW);
		SetWriteHandler(0x8000, 0x87FF, JYASIC_WritePRG); /* 8800-8FFF ignored */
		SetWriteHandler(0x9000, 0x97FF, JYASIC_WriteCHRLow); /* 9800-9FFF ignored */
		SetWriteHandler(0xA000, 0xA7FF, JYASIC_WriteCHRHigh); /* A800-AFFF ignored */
		SetWriteHandler(0xB000, 0xB7FF, JYASIC_WriteNT); /* B800-BFFF ignored */
		SetWriteHandler(0xC000, 0xCFFF, JYASIC_WriteIRQ);
		SetWriteHandler(0xD000, 0xD7FF, JYASIC_WriteMode); /* D800-DFFF ignored */
		for (i = 0; i < 0x10000; i++) {
			JYASIC_cpuWrite[i] = GetWriteHandler(i);
		}
		SetWriteHandler(0x0000, 0xFFFF, JYASIC_trapCPUWrite); /* Trap all CPU writes for IRQ clocking purposes */
		JYASIC_CPUWriteHandlersSet = 1;
		SetReadHandler(0x5000, 0x5FFF, JYASIC_ReadALU_DIP);
		SetReadHandler(0x6000, 0xFFFF, CartBR);
		JYASIC_SyncPRG();
		JYASIC_SyncCHR();
		JYASIC_SyncMirror();
	} else {
		SetWriteHandler(0x5000, 0x5FFF, WriteReg);
		SetWriteHandler(0x8000, 0xFFFF, MMC3_Write);
		SetReadHandler(0x8000, 0xFFFF, CartBR);
		MMC3_SyncPRG();
		MMC3_SyncCHR();
	}
}

static void Power(void) {
	memset(&m394, 0, sizeof(m394));
	m394.reg[1] = 0x0F; /* start in MMC3 mode */
	m394.reg[3] = 0x90; /* set default chr/prg mask */
	JYASIC_RegReset();
	MMC3_Power();
	SetWriteHandler(0x5000, 0x5FFF, WriteReg);
}

void Mapper394_Init(CartInfo *info) {
	/* Multicart */
	JYASIC_Init(info, TRUE);
	JYASIC_pwrap = SetPRG_jy;
	JYASIC_cwrap = SetCHR_jy;
	JYASIC_wwrap = SetWRAM_jy;
	JYASIC_mwrap = SetMirror_jy;

	MMC3_Init(info, MMC3B, 0, 0);
	MMC3_pwrap = SetPRG_mmc3;
	MMC3_cwrap = SetCHR_mmc3;

	info->Reset = Power;
	info->Power = Power;

	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);
}
