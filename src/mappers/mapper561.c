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
 * NES 2.0 Mapper 561 denotes ROM image files extracted from disk images for the
 * Bung Super Game Doctor 2M and 4M RAM cartridges. They represent games whose
 * Doctor Header file denotes a Super Game Doctor disk (byte $0 bit 7 set, byte
 * $7!=$00). A clone of the Magic Card 2M, it differs in several aspects.
 */

#include "mapinc.h"

static struct {
	uint8_t mc1mode; /* 1M Mode register */
	uint8_t mc2mode; /* 2M/4M Mode register */

	uint8_t prg8K[4];
	uint8_t chr8K;
	uint8_t latch;

	uint8_t fds_control;
	int16_t fds_IRQCount;

	int16_t sgd_IRQCount;
} m561;

static SFORMAT StateRegs[] = {
	{ &m561.mc1mode, 1, "MC1M" },
	{ &m561.mc2mode, 1, "MC2M" },

	{ m561.prg8K, 4, "PRG8" },
	{ &m561.chr8K, 1, "CHR8" },
	{ &m561.latch, 1, "LATC" },

	{ &m561.fds_control, 1, "FDSI" },
	{ &m561.fds_IRQCount, sizeof(m561.fds_IRQCount) | FCEUSTATE_RLSB, "FDSC" },

	{ &m561.sgd_IRQCount, sizeof(m561.sgd_IRQCount) | FCEUSTATE_RLSB, "SGDC" },

	{ 0 }
};

static void SyncPRG(void) {
	uint8_t prg_writable = !(m561.mc1mode & 0x02);

	SetupCartPRGMapping(0, PRGptr[0], PRGsize[0], prg_writable);

	if (!(m561.mc2mode & 0x01)) {
		setprg8(0x8000, m561.prg8K[0]);
		setprg8(0xA000, m561.prg8K[1]);
		setprg8(0xC000, m561.prg8K[2]);
		setprg8(0xE000, m561.prg8K[3]);
	} else {
		switch (m561.mc1mode >> 5) {
		case 0:
			setprg16(0x8000, m561.latch & 0x07);
			setprg16(0xC000, 0x07);
			break;
		case 1:
			setprg16(0x8000, (m561.latch >> 2) & 0x0F);
			setprg16(0xC000, 0x07);
			break;
		case 2:
			setprg16(0x8000, m561.latch & 0x0F);
			setprg16(0xC000, 0x0F);
			break;
		case 3:
			setprg16(0x8000, 0x0F);
			setprg16(0xC000, m561.latch & 0x0F);
			break;
		case 4:
			setprg32(0x8000, (m561.latch >> 4) & 0x03);
			break;
		case 5:
			setprg32(0x8000, 0x03);
			break;
		case 6:
			setprg8(0x8000, m561.latch & 0x0F);
			setprg8(0xA000, m561.latch >> 4);
			setprg16(0xC000, 0x07);
			break;
		case 7:
			setprg8(0x8000, m561.latch & 0x0E);
			setprg8(0xA000, (m561.latch >> 4) | 0x01);
			setprg16(0xC000, 0x07);
			break;
		}
	}
}

static void SyncCHR(void) {
	uint8_t chr_writable = !((m561.mc1mode & 0xE0) & 0x80);

	SetupCartCHRMapping(0, CHRptr[0], CHRsize[0], chr_writable);

	setchr8(m561.chr8K);
}

static void SyncMirror(void) {
	switch (m561.mc1mode & 0x11) {
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

static DECLFW(WriteReg) {
	switch (A) {
	case 0x4024:
		X6502_IRQEnd(FCEU_IQEXT);
		break;
	case 0x4025:
		X6502_IRQEnd(FCEU_IQEXT);
		m561.fds_control = V;
		if (m561.fds_control & 0x42) {
			m561.fds_IRQCount = 0;
		}
		break;
	case 0x4100:
		X6502_IRQEnd(FCEU_IQEXT);
		m561.sgd_IRQCount = (int16_t)((m561.sgd_IRQCount & 0xFF00) | V);
		if (!V) {
			m561.sgd_IRQCount = V;
		}
		break;
	case 0x4101:
		X6502_IRQEnd(FCEU_IQEXT);
		m561.sgd_IRQCount = (int16_t)((m561.sgd_IRQCount & 0x00FF) | (V << 8));
		break;
	case 0x42FC:
	case 0x42FD:
	case 0x42FE:
	case 0x42FF:
		m561.mc1mode = (V & 0xF0) | (A & 0x03);
		SyncPRG();
		SyncCHR();
		SyncMirror();
		break;
	case 0x43FC:
	case 0x43FD:
	case 0x43FE:
	case 0x43FF:
		m561.mc2mode = (V & 0xF0) | (A & 0x03);
		m561.chr8K = V & 0x03;
		SyncPRG();
		SyncCHR();
		break;
	}
}

static DECLFW(WriteLatch) {
	if (m561.mc1mode & 0x02) {
		m561.latch = V;
		m561.prg8K[(A >> 13) & 0x03] = V >> 2;
		switch (m561.mc1mode >> 5) {
		case 1:
		case 4:
		case 5:
			m561.chr8K = m561.latch & 0x03;
			break;
		case 3:
			m561.chr8K = (m561.latch >> 4) & 0x03;
			break;
		default:
			/* keep m561.chr8K bank from last mode */
			break;
		}
		SyncPRG();
		SyncCHR();
	} else {
		CartBW(A, V);
	}
}

static void ClockFDSCounter(int a) {
	m561.fds_IRQCount += 3 * a;
	while ((m561.fds_IRQCount >= 448) && (m561.fds_control & 0x80)) {
		X6502_IRQBegin(FCEU_IQEXT);
		m561.fds_IRQCount -= 448;
	}
}

static void ClockSGDCounter(int a) {
	if (m561.sgd_IRQCount < 0) {
		m561.sgd_IRQCount += a;
		if (m561.sgd_IRQCount >= 0) {
			X6502_IRQBegin(FCEU_IQEXT);
		}
	}
}

static void CPUIRQHook(int a) {
	ClockFDSCounter(a);
	ClockSGDCounter(a);
}

static void SetTrainer(void) {
	if (ROM.misc.data && (ROM.misc.size >= 4)) {
		uint16_t trainerLoadAddr = 0x7000;
		uint16_t trainerInitAddr = 0x7003;
		uint32_t trainerSize = 512;
		uint8_t *trainerData = ROM.misc.data;
		uint32_t i;

		if (ROM.misc.size != 512) {
			trainerLoadAddr = (ROM.misc.data[1] << 8) | ROM.misc.data[0];
			trainerInitAddr = (ROM.misc.data[3] << 8) | ROM.misc.data[2];
			trainerSize = ROM.misc.size - 4;
			trainerData = ROM.misc.data + 4;
		}

		FCEU_printf(" load addr : %04x\n", trainerLoadAddr);
		FCEU_printf(" init addr : %04x\n", trainerInitAddr);
		FCEU_printf(" data size : %d\n", trainerSize);

		for (i = 0; i < ROM.misc.size; i++) {
			WRAM[(trainerLoadAddr & 0x1FFF) + i] = trainerData[i];
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
	m561.fds_control = m561.fds_IRQCount = m561.sgd_IRQCount = 0;
	SyncPRG();
	SyncCHR();
	SyncMirror();
	SyncWRAM();
}

static void Power(void) {
	m561.mc1mode = (iNESCart.submapper << 5) | ((iNESCart.mirror == MI_V) ? 0x01 : 0x11) | 0x02;
	m561.mc2mode = 0x03;

	m561.prg8K[0] = 0x1C;
	m561.prg8K[1] = 0x1D;
	m561.prg8K[2] = 0x1E;
	m561.prg8K[3] = 0x1F;
	m561.chr8K = 0;

	Reset();

	SetWriteHandler(0x4020, 0x47FF, WriteReg);

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

void Mapper561_Init(CartInfo *info) {
	uint32_t wramsize = info->PRGRamSize + info->PRGRamSaveSize;
	uint32_t prgsize = ROM.prg.size;

	info->Power = Power;
	info->Reset = Reset;

	MapIRQHook = CPUIRQHook;

	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);

	WRAMSIZE = wramsize ? wramsize : 8192;
	WRAM = (uint8_t *)FCEU_malloc(WRAMSIZE);
	SetupCartPRGMapping(0x10, WRAM, WRAMSIZE, TRUE);
	AddExState(WRAM, WRAMSIZE, 0, "WRAM");

	if ((info->submapper == 3) && (prgsize == SIZE_128K)) {
		uint8_t *newbuffer;

		prgsize = SIZE_256K;
		newbuffer = (uint8_t *)FCEU_malloc(prgsize);
		memset(newbuffer, 0xFF, prgsize);
		memcpy(newbuffer, ROM.prg.data, SIZE_128K);

		FCEU_free(ROM.prg.data);

		/* setup and map new prg data */
		ROM.prg.data = newbuffer;
		SetupCartPRGMapping(0, ROM.prg.data, prgsize, FALSE);
	}

	if (ROM.chr.size) {
		/* need data in CHR-RAM, not CHR-ROM */
		SetupCartCHRMapping(0, ROM.chr.data, ROM.chr.size, TRUE);
		AddExState(ROM.chr.data, ROM.chr.size, 0, "CRAM");
	}

	/* PRG memory can be writable, so add to states */
	AddExState(ROM.prg.data, prgsize, 0, "PRAM");
}
