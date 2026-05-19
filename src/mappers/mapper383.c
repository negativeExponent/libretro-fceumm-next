/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 * Copyright (C) 2023-2025-2026 negativeExponent
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

/*	晶太 YY840708C PCB
	Solely used for the "1995 Soccer 6-in-1 足球小将專輯 (JY-014)" multicart.
	MMC3+PAL16L8 combination, resulting in a bizarre mapper that switches banks in part upon *reads*.
*/

#include "mapinc.h"
#include "mmc3.h"

static struct {
	uint8_t A15, A16, A17A18;
} m383;

static SFORMAT StateRegs[] = {
	{ &m383.A15, 1, "PA15" },
	{ &m383.A16, 1, "PA16" },
	{ &m383.A17A18, 1, "A178" },
	{ 0 }
};

static void SetPRG(uint16_t A, uint16_t V) {
	uint16_t base;
	uint16_t mask;

	switch (m383.A17A18) {
	case 0x00:
		/* "Setting 0 provides a round-about means of dividing the first 128 KiB bank into two 32 KiB and one 64 KiB bank." */
		base = m383.A17A18 | m383.A16 | (m383.A16 ? 0x00 : m383.A15);
		mask = m383.A16 ? 0x07 : 0x03;
		break;
	case 0x30:
		/* "Setting 3 provides 128 KiB MMC3 banking with the CPU A14 line fed to the MMC3 clone reversed.
		   This is used for the game Tecmo Cup: Soccer Game (renamed "Tecmo Cup Soccer"),
		   originally an MMC1 game with the fixed bank at $8000-$BFFF and the switchable bank at $C000-$FFFF,
		   a configuration that could not be reproduced with an MMC3 alone." */
		base = m383.A17A18;
		mask = 0x0F;
		A ^= 0x4000;

		/* "It is also used for the menu,
		   which in part executes from PRG-ROM mapped to the CPU $6000-$7FFF address range on the MMC3 clone's fixed
		   banks alone, as no MMC3 PRG bank register is written to before JMPing to this address range." */
		if (A == 0xA000) {
			setprg8(0x6000, base | (V & 0x0B));
		}
		break;
	default:
		/* "Settings 1 and 2 provide normal 128 KiB MMC3 banking." */
		base = m383.A17A18;
		mask = 0x0F;
		break;
	}

	setprg8(A, base | (V & mask));
}

static void SetCHR(uint16_t A, uint16_t V) {
	setchr1(A, (m383.A17A18 << 3) | (V & 0x7F));
}

static DECLFR(Read16V8PAL) {
	if (m383.A17A18 == 0x00) { /* "PAL PRG m383.A16 is updated with the content of the corresponding MMC3 PRG bank bit by reading from the
		 respective address range, which in turn will then be applied across the entire ROM address range." */
		m383.A16 = mmc3.reg[0x06 | ((A >> 13) & 0x01)] & 0x08;
		MMC3_SyncPRG();
	}
	return CartBR(A);
}

static DECLFW(Write16V8PAL) {
	if (A & 0x0100) {
		m383.A15 = (A >> 11) & 0x04;
		m383.A17A18 = A & 0x30;
		MMC3_SyncPRG();
		MMC3_SyncCHR();
	}
	MMC3_Write(A, V);
}

static void Reset(void) {
	memset(&m383, 0, sizeof(m383));
	MMC3_Reset();
}

static void Power(void) {
	memset(&m383, 0, sizeof(m383));
	MMC3_Power();
	SetReadHandler(0x6000, 0x7FFF, CartBR);
	SetReadHandler(0x8000, 0xBFFF, Read16V8PAL);
	SetWriteHandler(0x8000, 0xFFFF, Write16V8PAL);
}

void Mapper383_Init(CartInfo *info) {
	MMC3_Init(info, MMC3B, 0, 0);
	MMC3_pwrap = SetPRG;
	MMC3_cwrap = SetCHR;
	info->Power = Power;
	info->Reset = Reset;
	AddExState(StateRegs, ~0, 0, NULL);
}
