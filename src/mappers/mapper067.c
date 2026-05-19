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
 */

#include "mapinc.h"

static struct {
	uint8_t prg, chr[4], mirror;
	uint8_t IRQa, toggle;
	int16_t IRQCount, IRQLatch;
} m067;

static SFORMAT StateRegs[] = {
	{ &m067.prg, 1, "PREG" },
	{ &m067.toggle, 1, "TOGL" },
	{ m067.chr, 4, "CREG" },
	{ &m067.mirror, 1, "MIRR" },
	{ &m067.IRQa, 1, "IRQA" },
	{ &m067.IRQCount, 2 | FCEUSTATE_RLSB, "IRQC" },
	{ &m067.IRQLatch, 2 | FCEUSTATE_RLSB, "IRQL" },
	{ 0 }
};

static void SyncPRG(void) {
	setprg16(0x8000, m067.prg);
	setprg16(0xC000, ~0);
}

static void SyncCHR(void) {
	setchr2(0x0000, m067.chr[0]);
	setchr2(0x0800, m067.chr[1]);
	setchr2(0x1000, m067.chr[2]);
	setchr2(0x1800, m067.chr[3]);
}

static void SyncMirror(void) {
	switch (m067.mirror & 0x03) {
	case 0:
		setmirror(MI_V);
		break;
	case 1:
		setmirror(MI_H);
		break;
	case 2:
		setmirror(MI_0);
		break;
	case 3:
		setmirror(MI_1);
		break;
	}
}

static DECLFW(WriteCHR) {
	switch (A & 0xF800) {
	case 0x8800:
	case 0x9800:
	case 0xA800:
	case 0xB800:
		m067.chr[(A >> 12) & 0x03] = V;
		SyncCHR();
		break;
	}
}

static DECLFW(WriteIRQ) {
	switch (A & 0xF800) {
	case 0xC000:
	case 0xC800:
		if (m067.toggle) {
			m067.IRQCount = (m067.IRQCount & 0xFF00) | V;
		} else {
			m067.IRQCount = (m067.IRQCount & 0x00FF) | (V << 8);
		}
		m067.toggle ^= 1;
		break;
	case 0xD800:
		m067.toggle = 0;
		m067.IRQa = (V & 0x10) != 0;
		X6502_IRQEnd(FCEU_IQEXT);
		break;
	}
}

static DECLFW(WriteMirror) {
	switch (A & 0xF800) {
	case 0xE800:
		m067.mirror = V;
		SyncMirror();
		break;
	}
}

static DECLFW(WritePRG) {
	switch (A & 0xF800) {
	case 0xF800:
		m067.prg = V;
		SyncPRG();
		break;
	}
}

static void CPUIRQHook(int a) {
	if (m067.IRQa) {
		m067.IRQCount -= a;
		if (m067.IRQCount < 0) {
			X6502_IRQBegin(FCEU_IQEXT);
			m067.IRQa = 0;
		}
	}
}


static void Power(void) {
	memset(&m067, 0, sizeof(m067));

	m067.chr[0] = 0;
	m067.chr[1] = 1;
	m067.chr[2] = 2;
	m067.chr[3] = 3;

	SyncPRG();
	SyncCHR();
	SyncMirror();

	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0x8000, 0xBFFF, WriteCHR);
	SetWriteHandler(0xC000, 0xDFFF, WriteIRQ);
	SetWriteHandler(0xE800, 0xEFFF, WriteMirror);
	SetWriteHandler(0xF800, 0xFFFF, WritePRG);
}

static void StateRestore(int version) {
	SyncPRG();
	SyncCHR();
	SyncMirror();
}

void Mapper067_Init(CartInfo *info) {
	info->Power = Power;
	MapIRQHook = CPUIRQHook;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);
}
