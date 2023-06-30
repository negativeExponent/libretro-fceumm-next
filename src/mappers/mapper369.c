/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2023
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

static uint8 mode;
static uint8 smb2j;
static uint8 IRQa;
static uint16 IRQCount;

static SFORMAT StateRegs[] = {
	{ &mode, 1, "MODE" },
	{ &smb2j, 1, "MBRG" },
	{ &IRQa, 1, "MIRQ" },
	{ &IRQCount, 2, "MIQC" },
	{ 0 }
};

static void M369PW(uint32 A, uint8 V) {
	switch (mode) {
	case 0x00:
	case 0x01: /* NROM */
		setprg32(0x8000, mode & 0x01);
		break;
	case 0x13: /* SMB2J */
		setprg8r(0, 0x6000, 0x0E);
		setprg8(0x8000, 0x0C);
		setprg8(0xa000, 0x0D);
		setprg8(0xc000, 0x08 | (smb2j & 0x07));
		setprg8(0xe000, 0x0F);
		break;
	case 0x37: /* MMC3: 128 KiB PRG */
	case 0xFF: /* MMC3: 256 KiB PRG */
	{
		uint8 mask = (mode == 0xFF) ? 0x1F : 0x0F;
		uint8 base = (mode == 0xFF) ? 0x20 : 0x10;

		setprg8r(0x10, 0x6000, 0);
		setprg8(A, (base & ~mask) | (V & mask));
		break;
	}
	}
}

static void M369CW(uint32 A, uint8 V) {
	switch (mode) {
	case 0x00:
	case 0x01: /* NROM */
	case 0x13: /* SMB2J */
		setchr8(mode & 0x03);
		break;
	case 0x37: /* MMC3: 128 KiB CHR */
	case 0xFF: /* MMC3: 256 KiB CHR */
	{
		uint16 mask = (mode == 0xFF) ? 0xFF : 0x7F;
		uint16 base = (mode == 0xFF) ? 0x100 : 0x80;

		setchr1(A, (base & ~mask) | (V & mask));
		break;
	}
	}
}

static DECLFW(M369WriteLo) {
	if (A & 0x100) {
		mode = V;
		MMC3_FixPRG();
		MMC3_FixCHR();
	}
}

static DECLFW(M369Write) {
	switch (A & 0xE000) {
	case 0x8000:
		if (mode == 0x13) {
			IRQa = 0;
			X6502_IRQEnd(FCEU_IQEXT);
		}
		MMC3_CMDWrite(A, V);
		break;
	case 0xA000:
		if (mode == 0x13) {
			IRQa = (V & 0x02) != 0;
		}
		MMC3_CMDWrite(A, V);
		break;
	case 0xC000:
		MMC3_IRQWrite(A, V);
		break;
	case 0xE000:
		if (mode == 0x13) {
			smb2j = V;
			MMC3_FixPRG();
		} else {
			MMC3_IRQWrite(A, V);
		}
		break;
	}
}

static void FP_FASTAPASS(1) SMB2JIRQHook(int a) {
	if (mode == 0x13) {
		if (IRQa) {
			IRQCount += a;
			if (IRQCount >= 4096) {
				IRQCount -= 4096;
				X6502_IRQBegin(FCEU_IQEXT);
			}
		}
	}
}

static void MMC3IRQHook(void) {
	if (mode != 0x13) {
		MMC3_IRQHBHook();
	}
}

static void M369Reset(void) {
	mode = 0;
	smb2j = 0;
	IRQa = 0;
	IRQCount = 0;
	MMC3_Reset();
}

static void M369Power(void) {
	mode = 0;
	smb2j = 0;
	IRQa = 0;
	IRQCount = 0;
	MMC3_Power();
	SetWriteHandler(0x4100, 0x4FFF, M369WriteLo);
	SetWriteHandler(0x8000, 0xFFFF, M369Write);
}

void Mapper369_Init(CartInfo *info) {
	MMC3_Init(info, 8, 0);
	MMC3_pwrap = M369PW;
	MMC3_cwrap = M369CW;
	info->Power = M369Power;
	info->Reset = M369Reset;
	MapIRQHook = SMB2JIRQHook;
	GameHBIRQHook = MMC3IRQHook;
	AddExState(&StateRegs, ~0, 0, 0);
}
