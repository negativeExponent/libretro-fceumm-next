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

/*
 * NES 2.0 Mapper 562 denotes ROM image files extracted from disk images for the
 * Venus Turbo Game Doctor 4+, 6+ and 6M RAM cartridges. They represent games
 * whose Doctor Header file denotes a Turbo Game Doctor disk (byte $0 bit 7
 * clear). A clone of the Magic Card 2M, it differs in several aspects.
 */

#include "mapinc.h"
#include "mapper562.h"

static struct {
	uint8_t mc1mode;
	uint8_t mc2mode;
	uint8_t tgdmode;

	uint8_t prg8K[4];
	uint8_t chr1K[8];
	uint8_t chr8k;
	uint8_t latch;
	uint8_t chrlock;

	uint8_t fds_control;
	int16_t fds_IRQCount;

	uint16_t tgd_IRQCount;
	uint16_t tgd_TargetCount;

	uint8_t lastCHRBank;
	uint16_t lastPPUAddr;
} m562;

static writefunc writePPU2007;

static SFORMAT StateRegs[] = {
	{ &m562.mc1mode, 1, "MC1M" },
	{ &m562.mc2mode, 1, "MC2M" },
	{ &m562.tgdmode, 1, "TDGM" },

	{ m562.prg8K, 4, "PRG8" },
	{ m562.chr1K, 8, "CHR1" },
	{ &m562.chr8k, 1, "CHR8" },
	{ &m562.latch, 1, "LATC" },
	{ &m562.chrlock, 1, "CHRL" },

	{ &m562.fds_control, 1, "FDSI" },
	{ &m562.fds_IRQCount, sizeof(m562.fds_IRQCount) | FCEUSTATE_RLSB, "FDSC" },

	{ &m562.tgd_IRQCount, sizeof(m562.tgd_IRQCount) | FCEUSTATE_RLSB, "TGDC" },
	{ &m562.tgd_TargetCount, sizeof(m562.tgd_TargetCount), "TGDT" },

	{ &m562.lastCHRBank, 1, "CHRB"},
	{ &m562.lastPPUAddr, 1, "LADR" },

	{ 0 }
};

static void SyncPRG(void) {
	uint8_t prg_writable = !(m562.mc1mode & 0x02);

	SetupCartPRGMapping(0, PRGptr[0], PRGsize[0], prg_writable);

	if (m562.tgdmode & 0x80) {
		setprg8(0x8000, m562.prg8K[0]);
		setprg8(0xA000, m562.prg8K[1]);
		setprg8(0xC000, m562.prg8K[2]);
		setprg8(0xE000, m562.prg8K[3]);
	} else if (!(m562.mc2mode & 0x01)) {
		setprg8(0x8000, (m562.prg8K[0] & 0x0F) | ((m562.mc2mode >> 2) & 0x10));
		setprg8(0xA000, (m562.prg8K[1] & 0x0F) | ((m562.mc2mode >> 2) & 0x10));
		setprg8(0xC000, (m562.prg8K[2] & 0x0F));
		setprg8(0xE000, (m562.prg8K[3] & 0x0F));
	} else {
		switch (m562.mc1mode >> 5) {
		case 0:
			setprg16(0x8000, m562.latch & 0x07);
			setprg16(0xC000, 0x07);
			break;
		case 1:
			setprg16(0x8000, (m562.latch >> 2) & 0x0F);
			setprg16(0xC000, 0x07);
			break;
		case 2:
			setprg16(0x8000, m562.latch & 0x0F);
			setprg16(0xC000, 0x0F);
			break;
		case 3:
			setprg16(0x8000, 0x0F);
			setprg16(0xC000, m562.latch & 0x0F);
			break;
		case 4:
			setprg32(0x8000, (m562.latch >> 4) & 0x03);
			break;
		case 5:
			setprg32(0x8000, 0x03);
			break;
		case 6:
			setprg8(0x8000, m562.latch & 0x0F);
			setprg8(0xA000, m562.latch >> 4);
			setprg16(0xC000, 0x07);
			break;
		case 7:
			setprg8(0x8000, m562.latch & 0x0E);
			setprg8(0xA000, (m562.latch >> 4) | 0x01);
			setprg16(0xC000, 0x07);
			break;
		}
	}
}

static void SyncCHR(void) {
	uint8_t chr_writable = !((m562.mc1mode & 0x80) || m562.chrlock);

	SetupCartCHRMapping(0, CHRptr[0], CHRsize[0], chr_writable);

	if (!(m562.tgdmode & 0x40)) {
		setchr8(m562.chr8k);
	} else {
		setchr1(0x0000, m562.chr1K[0]);
		setchr1(0x0400, m562.chr1K[1]);
		setchr1(0x0800, m562.chr1K[2]);
		setchr1(0x0C00, m562.chr1K[3]);
		setchr1(0x1000, m562.chr1K[4]);
		setchr1(0x1400, m562.chr1K[5]);
		setchr1(0x1800, m562.chr1K[6]);
		setchr1(0x1C00, m562.chr1K[7]);
	}
}

static void SyncMirror(void) {
	switch (m562.mc1mode & 0x11) {
	case 0x00:
		setmirror(MI_0);
		break;
	case 0x01:
		setmirror(MI_V);
		break;
	case 0x10:
		setmirror(MI_1);
		break;
	case 0x11:
		setmirror(MI_H);
		break;
	}
}

static void SyncWRAM(void) {
	setprg8r(0x10, 0x6000, 0);
}

extern uint32_t RefreshAddr;
static DECLFW(WritePPU2007) {
	if (!(RefreshAddr & 0x2000)) {
		if ((m562.mc1mode >= 0xA0) && !(m562.mc1mode & 0x01)) {
			m562.chrlock = !!(m562.mc1mode & 0x10);
		}
	}
	writePPU2007(A, V);
}

static DECLFR(ReadReg) {
	switch (A) {
	case 0x4400:
	case 0x4401:
	case 0x4402:
	case 0x4403:
	case 0x4404:
	case 0x4405:
	case 0x4406:
	case 0x4407:
		return m562.chr1K[A & 0x07];
	case 0x4408:
	case 0x4409:
	case 0x440A:
	case 0x440B:
		return (m562.prg8K[A & 0x03] << 2) | (m562.latch & 0x03);
	case 0x440C:
		return (m562.tgd_IRQCount >> 8);
	case 0x440D:
		return (m562.tgd_IRQCount & 0xFF);
	case 0x4411:
		return m562.tgdmode;
	case 0x4415:
		return m562.mc1mode;
	case 0x4420:
		return m562.chr1K[m562.lastCHRBank];
	}
	return cpu.openbus;
}

static DECLFR(ReadTGDBios) {
	return tgd4800[A];
}

static DECLFW(WriteReg) {
	switch (A) {
	case 0x4024:
		X6502_IRQEnd(FCEU_IQEXT);
		break;
	case 0x4025:
		X6502_IRQEnd(FCEU_IQEXT);
		m562.fds_control = V;
		if (m562.fds_control & 0x42) {
			m562.fds_IRQCount = 0;
		}
		break;
	case 0x42FC:
	case 0x42FD:
	case 0x42FE:
	case 0x42FF:
		m562.mc1mode = (V & 0xF0) | (A & 0x03);
		if (m562.mc1mode >= 0x80) {
			m562.chrlock = 0;
		}
		SyncPRG();
		SyncCHR();
		SyncMirror();
		break;
	case 0x43FC:
	case 0x43FD:
	case 0x43FE:
	case 0x43FF:
		m562.mc2mode = (V & 0xF0) | (A & 0x03);
		m562.chr8k = V & 0x03;
		SyncPRG();
		SyncCHR();
		break;
	case 0x4400:
	case 0x4401:
	case 0x4402:
	case 0x4403:
	case 0x4404:
	case 0x4405:
	case 0x4406:
	case 0x4407:
		m562.chr1K[A & 7] = V;
		SyncCHR();
		break;
	case 0x440C:
		X6502_IRQEnd(FCEU_IQEXT);
		if (!(V & 0x80)) {
			m562.tgd_IRQCount = 0x8000;
		}
		m562.tgd_TargetCount = (m562.tgd_TargetCount & 0x00FF) | (V << 8);
		break;
	case 0x440D:
		X6502_IRQEnd(FCEU_IQEXT);
		m562.tgd_TargetCount = (m562.tgd_TargetCount & 0xFF00) | V;
		break;
	case 0x4411:
		m562.tgdmode = V;
		SyncPRG();
		SyncCHR();
		break;
	}
}

static DECLFW(WriteLatch) {
	if (m562.mc1mode & 0x02) {
		m562.latch = V;
		m562.prg8K[(A >> 13) & 0x03] = V >> 2;
		switch (m562.mc1mode >> 5) {
		case 1:
		case 4:
		case 5:
			m562.chr8k = m562.latch & 0x03;
			break;
		case 3:
			m562.chr8k = (m562.latch >> 4) & 0x03;
			break;
		}
		SyncPRG();
		SyncCHR();
	} else {
		CartBW(A, V);
	}
}

static void ClockFDSCounter(int a) {
	m562.fds_IRQCount += 3 * a;
	while ((m562.fds_IRQCount >= 448) && (m562.fds_control & 0x80)) {
		X6502_IRQBegin(FCEU_IQEXT);
		m562.fds_IRQCount -= 448;
	}
}

static void CPUIRQHook(int a) {
	ClockFDSCounter(a);
	while (a--) {
		if (m562.tgd_TargetCount & 0x8000) {
			if ((m562.tgd_IRQCount == m562.tgd_TargetCount) && (m562.tgd_IRQCount != 0xFFFF)) {
				X6502_IRQBegin(FCEU_IQEXT);
			} else {
				m562.tgd_IRQCount++;
			}
		}
	}
}

static void PPUIRQHook(uint32_t A) {
	if ((m562.lastPPUAddr != A) && ((A & 0x3000) != 0x2000)) {
		m562.lastCHRBank = (A >> 13) & 0x07;
	}
	m562.lastPPUAddr = A;
}

static void SetTrainer(void) {
	if (ROM.misc.data && (ROM.misc.size >= 4)) {
		uint16_t trainerLoadAddr = 0x7000;
		uint16_t trainerInitAddr = 0x7003;
		uint32_t trainerSize = 512;
		uint8_t *trainerSource = ROM.misc.data;
		uint8_t *trainerData;
		uint32_t i;

		if (ROM.misc.size != 512) {
			trainerLoadAddr = (ROM.misc.data[1] << 8) | ROM.misc.data[0];
			trainerInitAddr = (ROM.misc.data[3] << 8) | ROM.misc.data[2];
			trainerSize = ROM.misc.size - 4;
			trainerSource = ROM.misc.data + 4;
		}

		if (trainerLoadAddr < 0x2000) {
			trainerData = &RAM[trainerLoadAddr & 0x7FF];
		} else if ((trainerLoadAddr >= 0x6000) && (trainerLoadAddr <= 0x7FFF)) {
			trainerData = &WRAM[trainerLoadAddr & 0x1FFF];
		} else {
			trainerSize = 0;
		}

		FCEU_printf(" load addr : %04x\n", trainerLoadAddr);
		FCEU_printf(" init addr : %04x\n", trainerInitAddr);
		FCEU_printf(" data size : %d\n", trainerSize);

		if (trainerSize) {
			for (i = 0; i < trainerSize; i++) {
				trainerData[i] = trainerSource[i];
			}
		}

		if (trainerInitAddr) {
			/* JSR init */
			(GetWriteHandler(0x0700))(0x0700, 0x20);
			(GetWriteHandler(0x0701))(0x0701, trainerInitAddr & 0xFF);
			(GetWriteHandler(0x0702))(0x0702, trainerInitAddr >> 8);

			/* JMP ($FFFC) */
			(GetWriteHandler(0x0703))(0x0703, 0x6C);
			(GetWriteHandler(0x0704))(0x0704, 0xFC);
			(GetWriteHandler(0x0705))(0x0705, 0xFF);

			X6502_SetNewPC(0x700);
		}
	}
	(GetWriteHandler(0x4017))(0x4017, 0x40);
}

static void Reset(void) {
	m562.fds_control = 0;
	m562.fds_IRQCount = 0;
	m562.tgd_IRQCount = 0xFFFF;
	m562.tgd_TargetCount = 0;

	SyncPRG();
	SyncCHR();
	SyncMirror();
	SyncWRAM();
}

static void Power(void) {
	m562.mc1mode = (iNESCart.submapper << 5) |
	    ((iNESCart.mirror == MI_V) ? 0x01 : 0x11) | 0x04 | 0x02;
	m562.mc2mode = 0x03;
	m562.tgdmode = 0x03;

	m562.latch = 0;
	m562.chr8k = 0;
	m562.chrlock = FALSE;

	m562.prg8K[0] = 0x1C;
	m562.prg8K[1] = 0x1D;
	m562.prg8K[2] = 0x1E;
	m562.prg8K[3] = 0x1F;

	m562.chr1K[0] = 0;
	m562.chr1K[1] = 1;
	m562.chr1K[2] = 2;
	m562.chr1K[3] = 3;
	m562.chr1K[4] = 4;
	m562.chr1K[5] = 5;
	m562.chr1K[6] = 6;
	m562.chr1K[7] = 7;

	Reset();

	writePPU2007 = GetWriteHandler(0x2007);
	SetWriteHandler(0x2007, 0x2007, WritePPU2007);

	SetReadHandler(0x4020, 0x47FF, ReadReg);
	SetWriteHandler(0x4020, 0x47FF, WriteReg);

	SetReadHandler(0x4800, 0x4FFF, ReadTGDBios);

	SetReadHandler(0x6000, 0x7FFF, CartBR);
	SetWriteHandler(0x6000, 0x7FFF, CartBW);
	FCEU_CheatAddRAM(WRAMSIZE >> 10, 0x6000, WRAM);

	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0x8000, 0xFFFF, WriteLatch);

	SetTrainer();
}

static void StateRestore(int version) {
	SyncPRG();
	SyncCHR();
	SyncMirror();
	SyncWRAM();
}

void Mapper562_Init(CartInfo *info) {
	uint32_t wramsize = info->PRGRamSize + info->PRGRamSaveSize;
	uint32_t prgsize = ROM.prg.size;

	info->Power = Power;
	info->Reset = Reset;

	MapIRQHook = CPUIRQHook;
	PPU_hook = PPUIRQHook;

	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);

	WRAMSIZE = wramsize ? wramsize : 8192;
	WRAM = (uint8_t *)FCEU_malloc(WRAMSIZE);
	SetupCartPRGMapping(0x10, WRAM, WRAMSIZE, TRUE);
	AddExState(WRAM, WRAMSIZE, 0, "WRAM");

	if (ROM.chr.size) {
		/* need data in CHR-RAM, not CHR-ROM */
		SetupCartCHRMapping(0, ROM.chr.data, ROM.chr.size, TRUE);
		AddExState(ROM.chr.data, ROM.chr.size, 0, "CRAM");
	}

	/* PRG memory can be writable, so add to states */
	AddExState(ROM.prg.data, prgsize, 0, "PRAM");
}
