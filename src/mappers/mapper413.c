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

#include "mapinc.h"

static struct {
	uint8_t reg[4];
	uint8_t IRQCount;
	uint8_t IRQReload;
	uint8_t IRQa;
	uint8_t serialControl;
	uint32_t serialAddress;
} m413;

static SFORMAT StateRegs[] = {
	{ m413.reg, 4, "REGS" },
	{ &m413.IRQCount, 1, "IRQC" },
	{ &m413.IRQReload, 1, "IRQR" },
	{ &m413.IRQa, 1, "IRQA" },
	{ &m413.serialAddress, 4 | FCEUSTATE_RLSB, "ADDR" },
	{ &m413.serialControl, 1, "CTRL" },
	{ 0 }
};

static void SyncPRG(void) {
	setprg8(0x8000, m413.reg[1]);
	setprg8(0xA000, m413.reg[2]);
	setprg4(0xD000, 0x07);
	setprg8(0xE000, 0x04);
}

static void SyncCHR(void) {
	setchr4(0x0000, m413.reg[3]);
	setchr4(0x1000, ~0x02);
}

static void SyncWRAM(void) {
	setprg4(0x5000, 0x01);
	setprg8(0x6000, m413.reg[0]);
}

static uint64_t lreset = 0;

static DECLFR(ReadPCM) {
	uint8_t ret = ROM.misc.data[m413.serialAddress & (ROM.misc.size - 1)];
	uint64_t ts = timestampbase + timestamp;

	if ((ts >= lreset) && (ts < (lreset + 6))) {
		return ret;
	}
	if (m413.serialControl & 0x02) {
		m413.serialAddress++;
	}
	lreset = ts;
	return ret;
}

static DECLFW(WriteIRQ) {
	switch (A & 0xF000) {
	case 0x8000:
		m413.IRQReload = V;
		break;
	case 0x9000:
		m413.IRQCount = 0;
		break;
	case 0xA000:
	case 0xB000:
		m413.IRQa = (A & 0x1000) != 0;
		if (!m413.IRQa) {
			X6502_IRQEnd(FCEU_IQEXT);
		}
		break;
	}
}

static DECLFW(WriteSerialAddress) {
	m413.serialAddress = (m413.serialAddress << 1) | (V >> 7);
}

static DECLFW(WriteSerialControl) {
	m413.serialControl = V;
}

static DECLFW(WriteReg) {
	uint8_t index = V >> 6;
	m413.reg[index] = V;
	switch (index) {
	case 0:
		SyncWRAM();
		break;
	case 1:
	case 2:
		SyncPRG();
		break;
	case 3:	
		SyncCHR();
		break;
	}
}

static void HBIRQHook(void) {
	if (m413.IRQCount == 0) {
		m413.IRQCount = m413.IRQReload;
	} else {
		m413.IRQCount--;
	}
	if ((m413.IRQCount == 0) && m413.IRQa) {
		X6502_IRQBegin(FCEU_IQEXT);
	}
}

static void Power(void) {
	memset(&m413, 0, sizeof(m413));
	lreset = 0;

	SyncPRG();
	SyncCHR();
	SyncWRAM();
	
	SetReadHandler(0x5000, 0xBFFF, CartBR);
	SetReadHandler(0xD000, 0xFFFF, CartBR);

	SetReadHandler(0x4800, 0x4FFF, ReadPCM);
	SetReadHandler(0xC000, 0xCFFF, ReadPCM);

	SetWriteHandler(0x8000, 0xBFFF, WriteIRQ);
	SetWriteHandler(0xC000, 0xCFFF, WriteSerialAddress);
	SetWriteHandler(0xD000, 0xDFFF, WriteSerialControl);
	SetWriteHandler(0xE000, 0xFFFF, WriteReg);
}

static void StateRestore(int version) {
	SyncPRG();
	SyncCHR();
	SyncWRAM();
}

void Mapper413_Init(CartInfo *info) {
	info->Power = Power;
	GameHBIRQHook = HBIRQHook;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, 0);
}
