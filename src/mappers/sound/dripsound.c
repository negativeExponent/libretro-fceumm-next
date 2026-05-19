/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2023-2024-2026 negativeExponent
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
#include "dripsound.h"

#define TIMER_SHIFT ((FSettings.soundq >= 1) ? 0 : 17)

typedef struct DRIPSOUND {
	uint8_t buffer[256], readPos, writePos;
	uint8_t bufferFull, bufferEmpty;
	uint8_t volume;
	uint16_t freq;
	int32_t timer;
	int16_t out;
} DRIPSOUND;

static DRIPSOUND drip[2];
static int32_t cvbc = 0;

static void ChannelReset(DRIPSOUND *ds) {
	memset(ds->buffer, 0, 256);
	ds->readPos = 0;
	ds->writePos = 0;
	ds->bufferFull = FALSE;
	ds->bufferEmpty = TRUE;
}

static uint8_t ChannelRead(DRIPSOUND *ds) {
	uint8_t result = 0;

	if (ds->bufferFull) {
		result |= 0x80;
	}

	if (ds->bufferEmpty) {
		result |= 0x40;
	}

	return result;
}

static void ChannelWrite(DRIPSOUND *ds, uint16_t A, uint8_t V) {
	switch (A & 0x03) {
	case 0:
		ChannelReset(ds);
		ds->out = 0;
		ds->timer = ds->freq << TIMER_SHIFT;
		break;

	case 1:
		if (ds->readPos == ds->writePos) {
			ds->bufferEmpty = FALSE;
			ds->out = V * ds->volume;
			ds->timer = ds->freq << TIMER_SHIFT;
		}

		ds->buffer[ds->writePos++] = V;
		if (ds->readPos == ds->writePos) {
			ds->bufferFull = TRUE;
		}
		break;

	case 2:
		ds->freq = (ds->freq & 0xF00) | V;
		break;

	case 3:
		ds->freq = (ds->freq & 0xFF) | ((V & 0xF) << 8);
		ds->volume = (V & 0xF0) >> 4;
		if (!ds->bufferEmpty) {
			ds->out = ds->buffer[ds->readPos] * ds->volume;
		}
		break;
	}
}

static void SyncHQ(int32_t ts) {
	cvbc = ts;
}

static int32_t GenerateWaveHQ(DRIPSOUND *ds) {
	if (!ds->bufferEmpty) {
		ds->timer--;
		if (ds->timer <= 0) {
			ds->timer = ds->freq << TIMER_SHIFT;
			if (ds->readPos == ds->writePos) {
				ds->bufferFull = FALSE;
			}

			ds->readPos++;

			if (ds->readPos == ds->writePos) {
				ds->bufferEmpty = TRUE;
			}

			ds->out = ds->buffer[ds->readPos] * ds->volume;
		}
	}
	return ds->out;
}

static int32_t GenerateWaveLQ(DRIPSOUND *ds) {
	if (!ds->bufferEmpty) {
		ds->timer -= nesincsize;

		while (ds->timer <= 0) {
			ds->timer += ds->freq << TIMER_SHIFT;

			if (ds->readPos == ds->writePos) {
				ds->bufferFull = FALSE;
			}

			ds->readPos++;

			if (ds->readPos == ds->writePos) {
				ds->bufferEmpty = TRUE;
			}

			ds->out = ds->buffer[ds->readPos] * ds->volume;
		}
	}
	return ds->out;
}

static void DoDRIPSoundHQ(void) {
	int V;

	for (V = cvbc; V < (int)SOUNDTS; V++) {
		int32_t out = 0;

		out += GenerateWaveHQ(&drip[0]);
		out += GenerateWaveHQ(&drip[1]);

		WaveHi[V] += out;
	}

	cvbc = SOUNDTS;
}

static void DRIPSound(void) {
	int V;
	int start = cvbc;
	int end = ((int)SOUNDTS << 16) / soundtsinc;

	if (end <= start) {
		return;
	}

	for (V = start; V < end; V++) {
		int32_t out = 0;

		out += GenerateWaveLQ(&drip[0]) >> 4;
		out += GenerateWaveLQ(&drip[1]) >> 4;

		Wave[V >> 4] += out;
	}

	cvbc = end;
}

static void DoDripSound(int Count) {
	DRIPSound();
	cvbc = Count;
}

DECLFR(DRIPSound_Read) {
	int idx = (A & 0x800) ? 1 : 0;

	return (ChannelRead(&drip[idx]));
}

DECLFW(DRIPSound_Write) {
	int idx = (A & 0x04) ? 1 : 0;

	if (FSettings.SndRate > 0) {
		if (FSettings.soundq >= 1) {
			DoDRIPSoundHQ();
		} else {
			DRIPSound();
		}
	}

	ChannelWrite(&drip[idx], A, V);
}

static void DRIPSound_SC(void) {
	GameExpSound[0].Fill = DoDripSound;
	GameExpSound[0].HiSync = SyncHQ;
	GameExpSound[0].HiFill = DoDRIPSoundHQ;
	GameExpSound[0].RChange = DRIPSound_SC;
	drip[0].timer = 0;
	drip[1].timer = 0;
	cvbc = 0;
}

void DRIPSound_ESI(void) {
	GameExpSound[0].RChange = DRIPSound_SC;

	ChannelReset(&drip[0]);
	ChannelReset(&drip[1]);

	DRIPSound_SC();
}

void DRIPSound_AddStateInfo(void) {
	AddExState(drip[0].buffer, 256, 0, "FF00");
	AddExState(&drip[0].readPos, 1, 0, "RDP0");
	AddExState(&drip[0].writePos, 1, 0, "WRP0");
	AddExState(&drip[0].bufferFull, 1, 0, "FUL0");
	AddExState(&drip[0].bufferEmpty, 1, 0, "EMT0");
	AddExState(&drip[0].freq, 2, TRUE, "FRQ0");
	AddExState(&drip[0].volume, 1, 0, "VOL0");
	AddExState(&drip[0].timer, 4, TRUE, "TIM0");
	AddExState(&drip[0].out, 2, TRUE, "POS0");

	AddExState(drip[1].buffer, 256, 0, "FF01");
	AddExState(&drip[1].readPos, 1, 0, "RDP1");
	AddExState(&drip[1].writePos, 1, 0, "WRP1");
	AddExState(&drip[1].bufferFull, 1, 0, "FUL1");
	AddExState(&drip[1].bufferEmpty, 1, 0, "EMT1");
	AddExState(&drip[1].freq, 2, TRUE, "FRQ1");
	AddExState(&drip[1].volume, 1, 0, "VOL1");
	AddExState(&drip[1].timer, 4, TRUE, "TIM1");
	AddExState(&drip[1].out, 2, TRUE, "POS1");

	AddExState(&cvbc, 4, TRUE, "CVBC");
}
