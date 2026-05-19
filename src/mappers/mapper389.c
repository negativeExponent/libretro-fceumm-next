/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2020
 *  Copyright (C) 2023-2025-2026 negativeExponent
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

/* Mapper 389 - Caltron 9-in-1 multicart */

#include "mapinc.h"

static struct {
	uint8_t reg[3];
} m389;

static SFORMAT StateRegs[] = {
	{ &m389.reg, 3, "REGS" },
	{ 0 }
};

static void SyncPRG(void) {
	if (m389.reg[1] & 0x02) {
		/* UNROM-064 */
		setprg16(0x8000, (m389.reg[0] >> 2) | ((m389.reg[2] >> 2) & 0x03));
		setprg16(0xC000, (m389.reg[0] >> 2) | 0x03);
	} else {
		/* NROM-256 */
		setprg32(0x8000, m389.reg[0] >> 3);
	}
}

static void SyncCHR(void) {
	setchr8(((m389.reg[1] >> 1) & 0x1C) | (m389.reg[2] & 0x03));
}

static void SyncMirror(void) {
	setmirror((m389.reg[0] & 0x01) ^ 1);
}

static DECLFW(WriteReg0) {
	m389.reg[0] = (A & 0xFF);
	SyncPRG();
	SyncMirror();
}

static DECLFW(WriteReg1) {
	m389.reg[1] = (A & 0xFF);
	SyncPRG();
	SyncCHR();
}

static DECLFW(WriteReg2) {
	m389.reg[2] = (A & 0x0F);
	SyncPRG();
	SyncCHR();
}

static void Reset(void) {
	memset(&m389, 0, sizeof(m389));
	SyncPRG();
	SyncCHR();
	SyncMirror();
}

static void Power(void) {
	memset(&m389, 0, sizeof(m389));
	SyncPRG();
	SyncCHR();
	SyncMirror();
	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0x8000, 0x8FFF, WriteReg0);
	SetWriteHandler(0x9000, 0x9FFF, WriteReg1);
	SetWriteHandler(0xA000, 0xFFFF, WriteReg2);
}

static void StateRestore(int version) {
	SyncPRG();
	SyncCHR();
	SyncMirror();
}

void Mapper389_Init(CartInfo *info) {
	info->Power = Power;
	info->Reset = Reset;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);
}
