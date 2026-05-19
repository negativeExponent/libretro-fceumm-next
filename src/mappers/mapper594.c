/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2025-2026 negativeExponent
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
#include "mmc3.h"
#include "fifo.h"
#include "msm6585.h"

static struct {
	uint8_t reg[4];
} m594;

static FIFO fifo;
static MSM6585 adpcm;
static int32_t cvbc = 0;

static SFORMAT StateRegs[] = {
	{ m594.reg, 4, "EXPR" },
	{ 0 }
};

static void SetPRG(uint16_t A, uint16_t V) {
	uint16_t mask = 0x3F;
	uint16_t base = (m594.reg[2] & 0x40 ? 0x0C0 : 0x000) |
	              (m594.reg[2] & 0x80 ? 0x100 : 0x000);
	
	setprg8(A, (base & ~mask) | (V & mask));
	setprg8(0x6000, base | m594.reg[0]);
}

static void SetCHR(uint16_t A, uint16_t V) {
	uint16_t mask = m594.reg[2] & 0xC0 ? 0x0FF : 0x1FF;
	uint16_t base = (m594.reg[2] & 0x40 ? 0x200 : 0x000) |
	              (m594.reg[2] & 0x80 ? 0x300 : 0x000);

	V |= ((A >> 4) & 0x100);
	setchr1(A, (base & ~mask) | (V & mask));
}

static DECLFR(ReadADPCM) {
	return FIFO_halfFull(&fifo) ? 0x00 : 0x40;
}

static DECLFW(WriteADPCM) {
	if (A & 0x01) {
		MSM6585_setRate(&adpcm, V >> 6);
		FIFO_reset(&fifo);
	} else {
		FIFO_add(&fifo, V);
	}
}

static int ServeADPCM(void) {
	return FIFO_retrieve(&fifo);
}

static DECLFW(WriteReg) {
	m594.reg[((A >> 12) & 0x02) | (A & 0x01)] = V;
	MMC3_SyncPRG();
	MMC3_SyncCHR();
}

static void Reset(void) {
	memset(&m594, 0, sizeof(m594));
	MMC3_Reset();
	FIFO_reset(&fifo);
	MSM6585_reset(&adpcm);
}

static void Power(void) {
	memset(&m594, 0, sizeof(m594));
	MMC3_Power();
	FIFO_reset(&fifo);
	MSM6585_reset(&adpcm);
	SetReadHandler(0x5000, 0x5FFF, ReadADPCM);
	SetReadHandler(0x6000, 0x7FFF, CartBR);
	SetWriteHandler(0x5000, 0x5FFF, WriteADPCM);
	SetWriteHandler(0x9000, 0x9FFF, WriteReg);
	SetWriteHandler(0xB000, 0xBFFF, WriteReg);
}

static void mapperSound_fillBufferLow(int count) {
	int V;
	int start = cvbc;
	int end = (SOUNDTS << 16) / soundtsinc;

	for (V = start; V < end; V++) {
		MSM6585_run(&adpcm);
		Wave[V >> 4] += MSM6585_getOutput(&adpcm) >> 1;
	}
	cvbc = count;
}

static void mapperSound_fillBufferHigh() {
	uint32_t V;
	for (V = cvbc; V < SOUNDTS; V++) {
		MSM6585_run(&adpcm);
		WaveHi[V] += MSM6585_getOutput(&adpcm) * 8 + 16384;
	}
	cvbc = SOUNDTS;
}

static void mapperSound_setSoundOffset(int32_t ts) {
	cvbc = ts;
}

static void mapperSound_init(void) {
	if (FSettings.SndRate) {
		GameExpSound[0].Fill = mapperSound_fillBufferLow;
		GameExpSound[0].HiFill = mapperSound_fillBufferHigh;
		GameExpSound[0].HiSync = mapperSound_setSoundOffset;
		GameExpSound[0].RChange = mapperSound_init;
	}
	MSM6585_init(&adpcm,
	             ((FSettings.soundq >= 1) ? 1789773 : FSettings.SndRate * 16),
	             ServeADPCM);
}

static void Close() {
	MMC3_Close();
	FIFO_close(&fifo);
}

void Mapper594_Init(CartInfo *info) {
	MMC3_Init(info, MMC3B, 0, 0);
	MMC3_pwrap = SetPRG;
	MMC3_cwrap = SetCHR;
	info->Power = Power;
	info->Reset = Reset;
	info->Close = Close;
	FIFO_init(&fifo, 1024);
	mapperSound_init();
	AddExState(StateRegs, ~0, 0, NULL);
	FIFO_AddStateInfo(&fifo);
	MSM6585_AddStateInfo(&adpcm);
}
