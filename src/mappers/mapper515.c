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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include "mapinc.h"
#include "latch.h"
#include "vrc7sound.h"

/* TODO: Mic emulation */
static struct {
	uint8_t adc_data;
} m515;

static void Sync(void) {
	uint32_t prg = latch.data & 0x3F;

	if (latch.data & 0x80) {
		/* select internal ROM bank located at last 1MiB of PRG-ROM */
		prg += (PRG_BANK_COUNT(16) - 64);
	}

	setprg16(0x8000, prg);
	setprg16(0xC000, PRG_BANK_COUNT(16) - 1); /* set to last bank */
	setchr8(0);
}

static DECLFR(ReadMic) {
	A &= 0x03;
	if (A == 3) {
		uint8_t ret = m515.adc_data & 0x80;
		m515.adc_data <<= 1;
		return ret;
	}
	return cpu.openbus;
}

static DECLFW(WriteOPLL) {
	VRC7Sound_WriteIO(A & 1, V);
}

static DECLFW(WriteMic) {
	A &= 0x03;
	/* TODO: Mic emulation here */
	if (A == 2) {
		m515.adc_data = 0 * 64.0;
	}
}

static void Power(void) {
	Latch_Power();
	SetReadHandler(0x6000, 0x6FFF, ReadMic);
	SetWriteHandler(0x6000, 0x6FFF, WriteOPLL);
	SetWriteHandler(0xC000, 0xCFFF, WriteMic);
}

void Mapper515_Init(CartInfo *info) {
	Latch_Init(info, Sync, NULL, FALSE, FALSE);
	info->Power = Power;

	VRC7Sound_ESI(TONE_2413);
	VRC7Sound_AddStateInfo();
}
