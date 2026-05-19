/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2015 CaH4e3
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
 * NES 2.0 Mapper 292 - PCB BMW8544
 * UNIF UNL-DRAGONFIGHTER
 * "Dragon Fighter" protected MMC3 based custom mapper board
 * mostly hacky implementation, I can't verify if this mapper can read a RAM of the
 * console or watches the bus writes somehow.
 *
 * TODO: needs updating
 *
 */

#include "mapinc.h"
#include "mmc3.h"

static struct {
	uint8_t reg[3];
	uint8_t reglatch[2];
	uint8_t cpuLatch;
} m292;

static writefunc cpuwrite[0x10000];

static SFORMAT StateRegs[] = {
	{ m292.reg, 3, "EXPR" },
	{ m292.reglatch, 2, "REGL" },
	{ &m292.cpuLatch, 1, "CPUL" },
	{ 0 }
};

static void SetPRG(uint16_t A, uint16_t V) {
	setprg8(A, V);
}

static void SyncCHR(void) {
	setchr2(0x0000, m292.reg[1] ^ (MMC3_GetCHRBank(0) >> 1));
	setchr2(0x0800, (m292.reg[2] << 1 & 0x80) ^ (MMC3_GetCHRBank(2) >> 1));
	setchr4(0x1000, m292.reg[2] & 0x3F);
}

static DECLFW(WriteCPULatch) {
	/*if (A == 0x4014) {
		m292.reg[1] = m292.reglatch[0];
		m292.reg[2] = m292.reglatch[1];
		m292.reg[3] = m292.cpuLatch;
		MMC3_SyncCHR();
	}*/
	m292.cpuLatch = V;
	cpuwrite[A](A, V);
}

static DECLFW(ProtectionWrite) {
	m292.reg[0] = V;
}

static DECLFR(ProtectionRead) {
	if (m292.reg[0] & 0x20) {
		/* PPU 0800/1000 */
		/* m292.reglatch[1] = m292.cpuLatch; */
		m292.reg[2] = m292.cpuLatch;
		MMC3_SyncCHR();
	} else {
		/* PPU 0000 */
		/* m292.reglatch[0] = m292.cpuLatch; */
		m292.reg[1] = m292.cpuLatch;
		MMC3_SyncCHR();
	}
	return cpu.openbus;
}

static DECLFW(WriteASIC) {
	switch (A & 0xE001) {
	case 0x8001:
		switch (mmc3.cmd & 0x07) {
		case 0:
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
			mmc3.reg[mmc3.cmd & 0x07] = V;
			MMC3_SyncCHR();
			break;
		default:
			MMC3_Write(A, V);
			break;
		}
		break;
	default:
		MMC3_Write(A, V);
		break;
	}
}

static void Power(void) {
	int i;
	memset(&m292, 0x00, sizeof(m292));
	MMC3_Power();
	SetWriteHandler(0x6000, 0x7FFF, ProtectionWrite);
	SetReadHandler(0x6000, 0x7FFF, ProtectionRead);
	SetWriteHandler(0x8000, 0x9FFF, WriteASIC);
	for (i = 0; i < 65536; i++) {
		cpuwrite[i] = GetWriteHandler(i);
	}
	SetWriteHandler(0x0000, 0xFFFF, WriteCPULatch);
}

void Mapper292_Init(CartInfo *info) {
	MMC3_Init(info, MMC3B, 0, 0);
	MMC3_pwrap = SetPRG;
	MMC3_SyncCHR = SyncCHR;
	info->Power = Power;
	AddExState(StateRegs, ~0, 0, NULL);
}
