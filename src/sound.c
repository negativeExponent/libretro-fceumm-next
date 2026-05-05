/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2002 Xodnizel
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

#include <stdlib.h>

#include <string.h>

#include "fceu-types.h"
#include "x6502.h"

#include "fceu.h"
#include "sound.h"
#include "filter.h"
#include "state.h"
#include "ppu.h"

#include "cart.h"
#include "nsf.h"
#include "apu.h"
#include "driver.h"

static uint32_t square_mix_table[32]; /* square channel mix table */
static uint32_t tnd_mix_table[203];   /* triangle/noise/dmc channel mix table */

int32_t Wave[8192 + 512];
int32_t WaveHi[40000];
int32_t WaveFinal[8192 + 512];

uint32_t soundtsoffs = 0;

/* Variables exclusively for low-quality sound. */
int32_t nesincsize = 0;
uint32_t soundtsinc = 0;
/* LQ variables segment ends. */

/* FIXME: Very ugly hack and only relevant in multichip NSF playback */
/* Indexing is based on sound channel enum */
EXPSOUND GameExpSound[GAMEEXPSOUND_COUNT] = {
	{ 0, 0, 0, 0, 0, 0 },
	{ 0, 0, 0, 0, 0, 0 },
	{ 0, 0, 0, 0, 0, 0 },
	{ 0, 0, 0, 0, 0, 0 },
	{ 0, 0, 0, 0, 0, 0 },
	{ 0, 0, 0, 0, 0, 0 },
};

static const uint8_t TriangleWaveTable[0x20] = {
	0x0F, 0x0E, 0x0D, 0x0C, 0x0B, 0x0A, 0x09, 0x08,
	0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00,
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
};

static const uint16_t NTSCNoiseFreqTable[0x10] =
{
	0x004, 0x008, 0x010, 0x020, 0x040, 0x060, 0x080, 0x0A0,
	0x0CA, 0x0FE, 0x17C, 0x1FC, 0x2FA, 0x3F8, 0x7F2, 0xFE4
};

static const uint16_t PALNoiseFreqTable[0x10] =
{
	0x004, 0x008, 0x00E, 0x01E, 0x03C, 0x058, 0x076, 0x094,
	0x0BC, 0x0EC, 0x162, 0x1D8, 0x2C4, 0x3B0, 0x762, 0xEC2
};

static const uint16_t NTSCDMCTable[0x10] =
{
	0x1AC, 0x17C, 0x154, 0x140, 0x11E, 0x0FE, 0x0E2, 0x0D6,
	0x0BE, 0x0A0, 0x08E, 0x080, 0x06A, 0x054, 0x048, 0x036
};

static const uint16_t PALDMCTable[0x10] =
{
	0x18E, 0x162, 0x13C, 0x12A, 0x114, 0x0EC, 0x0D2, 0x0C6,
	0x0B0, 0x094, 0x084, 0x076, 0x062, 0x04E, 0x042, 0x032
};

static const uint16_t NTSCFramePeriodTable[2][6] = {
	{ 7457, 7456, 7458, 7457,    1, 1, },
	{ 7457, 7456, 7458, 7458, 7452, 1, },
};

static const uint16_t PALFramePeriodTable[2][6] = {
	{ 8313, 8314, 8312, 8313,    1, 1, },
	{ 8313, 8314, 8312, 8320, 8312, 1,},
};

static const uint8_t frametype[6] = {
	FrameQuarter,
	FrameHalf,
	FrameQuarter,
	FrameNone,
	FrameHalf,
	FrameNone
};

static void Dummyfunc(void) { }
static void (*DoNoise)(void) = Dummyfunc;
static void (*DoTriangle)(void) = Dummyfunc;
static void (*DoPCM)(void) = Dummyfunc;
static void (*DoSQ1)(void) = Dummyfunc;
static void (*DoSQ2)(void) = Dummyfunc;

static SquareUnit   square1;
static SquareUnit   square2;
static TriangleUnit triangle;
static NoiseUnit    noise;
static DMCUnit      dmc;
static FrameCounter frame;

/* Lenght Counter */

static void LengthCounterReset(LengthCount *length, uint8_t hard, uint8_t isTriangle) {
	length->enabled = FALSE;
	if (hard || !isTriangle) {
		length->counter = 0;
		length->delayHalt = FALSE;
		length->halt = FALSE;
		length->nextHalt = FALSE;
		length->delayCounter = FALSE;
		length->nextCounter = 0;
	}
}

static INLINE void LengthCounterSetHalt(LengthCount *length, int isLengthClocking, uint8_t V) {
	if (!isLengthClocking) {
		length->halt = V;
	} else {
		length->delayHalt = TRUE;
		length->nextHalt = V;
	}
}

static INLINE void LengthCounterSet(LengthCount *length, int isLengthClocking, uint8_t V) {
	if (length->enabled) {
		uint8_t period = lengthtable[V];
		if (!isLengthClocking) {
			length->counter = period;
		} else {
			length->delayCounter = TRUE;
			length->nextCounter = period;
		}
	}
}

static INLINE void LengthCounterSetEnabled(LengthCount *length, uint8_t enable) {
	length->enabled = enable;
	if (!length->enabled) {
		length->counter = 0;
	}
}

static INLINE void ClockLengthCounter(LengthCount *length) {
	if (!length->halt && length->counter) {
		length->counter--;
	}
	if (length->delayHalt) {
		length->halt = length->nextHalt;
		length->delayHalt = FALSE;
	}
	if (length->nextCounter) {
		if (!length->counter) {
			length->counter = length->nextCounter;
		}
		length->nextCounter = 0;
	}
}

/* Sweep */

static void SweepReset(Sweep *sweep, int id) {
	sweep->counter = 0;
	sweep->enabled = FALSE;
	sweep->negate = FALSE;
	sweep->period = 0;
	sweep->pulsePeriod = 0;
	sweep->reload = FALSE;
	sweep->shift = 0;
    sweep->id = id;
}

static INLINE void ClockSweep(Sweep *sweep) {
	if (sweep->counter) {
		sweep->counter--;
	}
	if (sweep->counter == 0) {
		sweep->counter = sweep->period + 1;
		if (sweep->enabled && sweep->shift && sweep->pulsePeriod >= 0x08) {
			int32_t delta = sweep->pulsePeriod >> sweep->shift;
			if (sweep->negate) {
				sweep->pulsePeriod -= delta;
				if (sweep->id == 0) {
					sweep->pulsePeriod--;
				}
			} else if ((sweep->pulsePeriod + delta) < 0x800) {
				sweep->pulsePeriod += delta;
			}
		}
	}
	if (sweep->reload) {
		sweep->reload = FALSE;
		sweep->counter = sweep->period + 1;
	}
}

/* Envelope */

static void EnvelopeReset(Envelope *envelope) {
	envelope->constant = FALSE;
	envelope->counter = 0;
	envelope->decay_volume = 0;
	envelope->loop = FALSE;
	envelope->reload = FALSE;
	envelope->speed = 0;
}

static INLINE int32_t EnvelopeVolume(Envelope *envelope) {
	if (envelope->constant) {
		return envelope->speed;
	}
	return envelope->decay_volume;
}

static INLINE void ClockEnvelope(Envelope *envelope) {
	if (envelope->reload) {
		envelope->reload = FALSE;
		envelope->decay_volume = 0x0F;
		envelope->counter = envelope->speed + 1;
	} else {
		if (envelope->counter) {
			envelope->counter--;
		}
		if (envelope->counter == 0) {
			envelope->counter = envelope->speed + 1;
			if (envelope->decay_volume || envelope->loop) {
				envelope->decay_volume--;
				envelope->decay_volume &= 0x0F;
			}
		}
	}
}

/* Square */

static int CheckFreq(SquareUnit *sq) {
	uint32_t mod;
	uint32_t period = sq->sweep.pulsePeriod;
	if (!sq->sweep.negate) {
		mod = period >> sq->sweep.shift;
		if ((mod + period) & 0x800) {
			return FALSE;
		}
	}
	return TRUE;
}

/* returns output from envelope, unless silenced by
 * sweeo unit overflow, or period < 8 or lengthcounter is 0 */
static INLINE int32_t SquareOutput(SquareUnit *square) {
	if ((square->sweep.pulsePeriod < 8) || !CheckFreq(square) || (square->length.counter == 0)) {
		return 0;
	}
	return EnvelopeVolume(&square->envelope);
}

static void SquareReset(SquareUnit *square, int id, int hard) {
	LengthCounterReset(&square->length, TRUE, FALSE);
	EnvelopeReset(&square->envelope);
	SweepReset(&square->sweep, id);

    square->timer.counter = 2048;
	square->timer.count2 = nesincsize ? (((uint32_t)2048 << 17) / nesincsize) : 1;
	square->timer.period = 0;

	square->duty = 0;
	square->step = 0;
}

/* Triangle */

static void TriangleReset(int hard) {
	LengthCounterReset(&triangle.length, hard, TRUE);

	triangle.timer.counter = 1;
	triangle.timer.count2 = 0;
	triangle.timer.period = 0;

	triangle.linearPeriod = 0;
	triangle.linearCounter = 0;
	triangle.linearReload = FALSE;
	triangle.stepCounter = 0;
}

static INLINE uint16_t TriangleOutput(void) {
	return TriangleWaveTable[triangle.stepCounter & 0x1F] * 3;
}

/* Noise */

static INLINE void LoadNoisePeriod(uint8_t V) {
	if (isPAL) {
		noise.timer.period = PALNoiseFreqTable[V];
	} else {
		noise.timer.period = NTSCNoiseFreqTable[V];
	}
}

/* returns output from envelope, unless lengcounter is 0 */
static INLINE int32_t NoiseOutput(void) {
	if (noise.length.counter == 0) {
		return 0;
	}
	return EnvelopeVolume(&noise.envelope);
}

static void NoiseReset(int hard) {
	LengthCounterReset(&noise.length, hard, FALSE);
	EnvelopeReset(&noise.envelope);

	noise.timer.counter = 2048;
	noise.timer.count2 = nesincsize ? (((uint32_t)2048 << 17) / nesincsize) : 1;
	noise.timer.period = 0;

	noise.periodIndex = 0;
	noise.shiftRegister = 1;
	noise.shortMode = 0;

	LoadNoisePeriod(noise.periodIndex);
}

/* DMC */

static INLINE void LoadDMCPeriod(uint8_t V) {
	if (isPAL) {
		dmc.timer.period = PALDMCTable[V];
	} else {
		dmc.timer.period = NTSCDMCTable[V];
	}
}

static INLINE void PrepDPCM(void) {
	dmc.readAddress = 0x4000 + (dmc.addressLatch << 6);
	dmc.lengthCounter = (dmc.lengthLatch << 4) + 1;
}

static void ClockDMCDMA(int cycles) {
	if (dmc.lengthCounter && !dmc.dmaBufferValid) {
		X6502_DMR(0x8000 + dmc.readAddress);
		X6502_DMR(0x8000 + dmc.readAddress);
		X6502_DMR(0x8000 + dmc.readAddress);
		dmc.dmaBuffer = X6502_DMR(0x8000 + dmc.readAddress);
		dmc.dmaBufferValid = TRUE;
		dmc.readAddress = (dmc.readAddress + 1) & 0x7fff;
		dmc.lengthCounter--;
		if (!dmc.lengthCounter) {
			if (dmc.loop) {
				PrepDPCM();
			} else {
				if (dmc.irqEnabled) {
					dmc.irqPending = TRUE;
					X6502_IRQBegin(FCEU_IQDPCM);
				}
			}
		}
	}

	dmc.timer.counter -= cycles;

	while (dmc.timer.counter <= 0) {
		if (dmc.sampleValid) {
			/* Unbelievably ugly hack */
			if (FSettings.SndRate) {
				const uint32_t fudge = MIN((uint32_t)(-dmc.timer.counter), (uint32_t)(soundtsoffs + timestamp));

				soundtsoffs -= fudge;
				DoPCM();
				soundtsoffs += fudge;
			}

			if (FSettings.ReverseDMCBitOrder) {
				if (dmc.sampleShiftReg & 0x80) {
					if (dmc.rawDataLatch <= 0x7D) {
						dmc.rawDataLatch += 2;
					}
				} else {
					if (dmc.rawDataLatch >= 0x02) {
						dmc.rawDataLatch -= 2;
					}
				}
				dmc.sampleShiftReg <<= 1;
			} else {
				if (dmc.sampleShiftReg & 0x01) {
					if (dmc.rawDataLatch <= 0x7D) {
						dmc.rawDataLatch += 2;
					}
				} else {
					if (dmc.rawDataLatch >= 0x02) {
						dmc.rawDataLatch -= 2;
					}
				}
				dmc.sampleShiftReg >>= 1;
			}
		}

		dmc.timer.counter += dmc.timer.period;
		dmc.bitCounter = (dmc.bitCounter + 1) & 7;
		if (dmc.bitCounter == 0) {
			if (!dmc.dmaBufferValid)
				dmc.sampleValid = FALSE;
			else {
				dmc.sampleValid = TRUE;
				dmc.sampleShiftReg = dmc.dmaBuffer;
				dmc.dmaBufferValid = FALSE;
			}
		}
	}
}

static void DMCReset(int hard) {
	if (hard) {
		dmc.addressLatch = 0;
		dmc.lengthLatch = 0;
	}
	dmc.rawDataLatch = 0;
	
	dmc.irqEnabled = FALSE;
	dmc.loop = FALSE;
	dmc.readAddress = 0;
	dmc.lengthCounter = 0;
	dmc.sampleShiftReg = 0;

	dmc.timer.counter = 1;
	dmc.bitCounter = 0;

	dmc.dmaBufferValid = FALSE;
	dmc.dmaBuffer = 0;
	dmc.sampleValid = FALSE;

	dmc.timer.period = isPAL ? PALDMCTable[0] : NTSCDMCTable[0];
}

/* Frame Counter */

static INLINE uint8_t lengthClocking(void) {
	return ((frame.counter == 1) && ((frame.step == 1) || (frame.step == 4))) ? TRUE : FALSE;
}

static INLINE int32_t GetFramePeriodNext(void) {
	if (isPAL) {
		return PALFramePeriodTable[frame.mode][frame.step];
	}
	return NTSCFramePeriodTable[frame.mode][frame.step];
}

static void FrameSoundStuff(enum FrameType type) {
	SquareUnit *s;
	uint8_t loop_flag;
	int P;

	if (type == FrameNone) {
		return;
	}

	DoSQ1();
	DoSQ2();
	DoNoise();
	DoTriangle();

	/* Envelope decay, linear counter, length counter, freq Sweep */

	/* Length counters and frequency sweep, running at odd frame intervals */
	if (type == FrameHalf) {
		ClockLengthCounter(&square1.length);
		ClockLengthCounter(&square2.length);
		ClockLengthCounter(&triangle.length);
		ClockLengthCounter(&noise.length);

		ClockSweep(&square1.sweep);
		ClockSweep(&square2.sweep);
	}

	/* Now do envelope decay + linear counter. */

	if (triangle.linearReload) {
		triangle.linearCounter = triangle.linearPeriod;
	} else if (triangle.linearCounter) {
		triangle.linearCounter--;
	}

	if (!triangle.length.halt) {
		triangle.linearReload = FALSE;
	}

	ClockEnvelope(&square1.envelope);
	ClockEnvelope(&square2.envelope);
	ClockEnvelope(&noise.envelope);

	if (isMMC5Audio) {
		MMC5SoundFrameTick();
	}
}

static void ClockFrameCounter(int cycles) {
	while (cycles--) {
		frame.counter--;

		if (frame.delay && --frame.delay == 0) {
			frame.step = 0;
			frame.counter = GetFramePeriodNext() + 2;
			frame.mode = (frame.newMode & 0x80) ? FrameFiveStepMode : FrameFourStepMode;
			if (frame.mode == FrameFiveStepMode) {
				FrameSoundStuff(FrameHalf);
			}
		}

		if (frame.counter == 0) {
			enum FrameType type;

			if (frame.step >= 3) {
				if (frame.mode == FrameFourStepMode && frame.irqInhibit == 0) {
					frame.irqPending = 1;
					if (frame.step == 4) {
						X6502_IRQBegin(FCEU_IQFCOUNT);
					}
				}
			}

			type = frametype[frame.step];
			FrameSoundStuff(type);

			frame.step++;
			if (frame.step == 6) {
				frame.step = 0;
			}

			frame.counter = GetFramePeriodNext();
		}
	}
}

static void FrameCounterReset(int hard) {
	frame.irqInhibit = FALSE;
	if (hard) {
		frame.mode = FrameFourStepMode;
	}

	frame.irqPending = 0;
	frame.step = 0;
	frame.counter = GetFramePeriodNext();

	frame.delay = 0;
	frame.newMode = (frame.mode == FrameFiveStepMode) ? 0x80 : 0;
}

static INLINE void SquareWrite(SquareUnit *square, uint8_t reg, uint8_t V) {
	switch (reg) {
	case 0:
		square->envelope.speed = V & 0x0F;
		square->envelope.constant = (V & 0x10) ? TRUE : FALSE;
		square->envelope.loop = (V & 0x20) ? TRUE : FALSE;
		LengthCounterSetHalt(&square->length, lengthClocking(), (V & 0x20) ? TRUE : FALSE);
		square->duty = (V & 0xC0) >> 6;
		break;

	case 1:
		square->sweep.shift = V & 0x07;
		square->sweep.negate = (V & 0x08) ? TRUE : FALSE;
		square->sweep.period = (V & 0x70) >> 4;
		square->sweep.enabled = (V & 0x80) ? TRUE : FALSE;
		square->sweep.reload = TRUE;
		break;

	case 2:
		square->timer.period = (square->timer.period & 0x0700) | V;
		square->sweep.pulsePeriod = (square->sweep.pulsePeriod & 0x0700) | V;
		break;

	case 3:
		square->timer.period = (square->timer.period & 0x00FF) | ((V & 0x07) << 8);
		square->sweep.pulsePeriod = (square->sweep.pulsePeriod & 0x00FF) | ((V & 0x07) << 8);
		square->step = 0;
		square->envelope.reload = TRUE;
		LengthCounterSet(&square->length, lengthClocking(), V >> 3);
		break;
	}
}

static INLINE void TriangleWrite(uint8_t reg, uint8_t V) {
	switch (reg) {
	case 0:
		triangle.linearPeriod = V & 0x7F;
		LengthCounterSetHalt(&triangle.length, lengthClocking(), (V & 0x80) ? TRUE : FALSE);
		break;

	case 2:
		triangle.timer.period = (triangle.timer.period & 0x0700) | V;
		break;

	case 3:
		triangle.timer.period = (triangle.timer.period & 0x00FF) | ((V & 0x07) << 8);
		triangle.linearReload = TRUE;
		LengthCounterSet(&triangle.length, lengthClocking(), V >> 3);
		break;
	}
}

static INLINE void NoiseWrite(uint8_t reg, uint8_t V) {
	switch (reg) {
	case 0:
		noise.envelope.speed = V & 0x0F;
		noise.envelope.constant = (V & 0x10) ? TRUE : FALSE;
		noise.envelope.loop = (V & 0x20) ? TRUE : FALSE;
		LengthCounterSetHalt(&noise.length, lengthClocking(), (V & 0x20) ? TRUE : FALSE);
		break;

	case 2:
		LoadNoisePeriod(V & 0x0F);
		noise.periodIndex = V & 0x0F;
		noise.shortMode = (V & 0x80) ? TRUE : FALSE;
		break;

	case 3:
		noise.envelope.reload = TRUE;
		LengthCounterSet(&noise.length, lengthClocking(), V >> 3);
		break;
	}
}

static INLINE void DMCWrite(uint8_t reg, uint8_t V) {
	switch (reg) {
	case 0:
		LoadDMCPeriod(V & 0xF);
		if (dmc.irqPending) {
			if ((V & 0xC0) != 0x80) {
				X6502_IRQEnd(FCEU_IQDPCM);
				dmc.irqPending = FALSE;
			}
		}
		dmc.periodIndex = V & 0x0F;
		dmc.loop = (V & 0x40) ? TRUE : FALSE;
		dmc.irqEnabled = (V & 0x80) ? TRUE : FALSE;
		break;

	case 1: {
		uint8_t newval = V & 0x07F;
		uint8_t lastval = dmc.rawDataLatch;
		dmc.rawDataLatch = newval;
		if (FSettings.ReduceDMCPopping) {
			dmc.rawDataLatch -= (dmc.rawDataLatch - lastval) / 2;
		}
		if ((GameInfo->type != GIT_NSF) && FSettings.PPUOverclockEnabled &&
			FSettings.SkipDMC7BitOverclock && V) {
			ppu.overclock.DMC_7bit_in_use = 1;
		}
		break;
	}

	case 2:
		dmc.addressLatch = V;
		if ((GameInfo->type != GIT_NSF) && FSettings.SkipDMC7BitOverclock && V) {
			ppu.overclock.DMC_7bit_in_use = 0;
		}
		break;

	case 3:
		dmc.lengthLatch = V;
		if ((GameInfo->type != GIT_NSF) && FSettings.SkipDMC7BitOverclock && V) {
			ppu.overclock.DMC_7bit_in_use = 0;
		}
		break;
	}
}

static DECLFW(PSGWrite) {
	int index = A & 0x1F;
	int channel = index >> 2;
	int reg = index & 0x03;
	switch (channel) {
	case 0: DoSQ1(); SquareWrite(&square1, reg, V); break;
	case 1: DoSQ2(); SquareWrite(&square2, reg, V); break;
	case 2: DoTriangle(); TriangleWrite(reg, V); break;
	case 3: DoNoise(); NoiseWrite(reg, V); break;
	case 4: DoPCM(); DMCWrite(reg, V); break;
	}
}

static DECLFW(StatusWrite) {
	DoSQ1();
	DoSQ2();
	DoTriangle();
	DoNoise();
	DoPCM();
	LengthCounterSetEnabled(&square1.length, (V & 0x01) ? TRUE : FALSE);
	LengthCounterSetEnabled(&square2.length, (V & 0x02) ? TRUE : FALSE);
	LengthCounterSetEnabled(&triangle.length, (V & 0x04) ? TRUE : FALSE);
	LengthCounterSetEnabled(&noise.length, (V & 0x08) ? TRUE : FALSE);
	if (V & 0X10) {
		if (!dmc.lengthCounter) {
			PrepDPCM();
		}
	} else {
		dmc.lengthCounter = 0;
	}

	dmc.irqPending = FALSE;
	X6502_IRQEnd(FCEU_IQDPCM);
}

static DECLFR(StatusRead) {
	uint8_t ret = 0;
	ret |= square1.length.counter ? 0x01 : 0;
	ret |= square2.length.counter ? 0x02 : 0;
	ret |= triangle.length.counter ? 0x04 : 0;
	ret |= noise.length.counter ? 0x08 : 0;
	ret |= dmc.lengthCounter ? 0x10 : 0;
	ret |= frame.irqPending ? 0x40 : 0;
	ret |= dmc.irqPending ? 0x80 : 0;
	frame.irqPending = FALSE;
	X6502_IRQEnd(FCEU_IQFCOUNT);
	return ret;
}

static DECLFW(IRQFrameWrite) {
	DoSQ1();
	DoSQ2();
	DoTriangle();
	DoNoise();
	DoPCM();
	
	frame.newMode = V;
	frame.delay = ((timestampbase + timestamp) & 0x01) ? 1 : 2;
	frame.irqInhibit = (V & 0x40) ? TRUE : FALSE;
	if (frame.irqInhibit) {
		frame.irqPending = FALSE;
		X6502_IRQEnd(FCEU_IQFCOUNT);
	}
}

void FCEU_SoundCPUHook(int cycles) {
	ClockFrameCounter(cycles);
	ClockDMCDMA(cycles);
}

static void RDoPCM(void) {
	uint32_t V;

	for (V = dmc.timer.cvbc; V < SOUNDTS; V++) {
		int32_t pcmout = GetOutput(SND_DMC, dmc.rawDataLatch);
		WaveHi[V] += ((pcmout << TRINPCM_SHIFT) & (~0xFFFF));
	}

	dmc.timer.cvbc = SOUNDTS;
}

static INLINE void RDoSQ(int x) {
	uint32_t V;
	SquareUnit *square = x ? &square2 : &square1;
	const uint8_t *dutyTbl = &SquareWaveTable[FSettings.SwapDutyCycles][square->duty][0];
	int32_t amp;

	amp = GetOutput(SND_SQUARE1 + x, SquareOutput(square));
	amp <<= SQ_SHIFT;

	for (V = square->timer.cvbc; V < SOUNDTS; V++) {
		WaveHi[V] += dutyTbl[square->step] * amp;
		square->timer.counter--;
		if (square->timer.counter == 0) {
			square->timer.counter = (square->sweep.pulsePeriod + 1) * 2;
			square->step = (square->step - 1) & 0x07;
		}
	}

	square->timer.cvbc = SOUNDTS;
}

static void RDoSQ1(void) {
	RDoSQ(0);
}

static void RDoSQ2(void) {
	RDoSQ(1);
}

static void RDoSQLQ(void) {
	int32_t start, end;
	int32_t V;
	int32_t inie[2];

	int32_t amp[2];		/* channel volume */
	int32_t ttable[2][8]; /* volume table based on duty */
	int32_t totalout; /* output taken from pulse table, from sq1 + sq2 outputs */

	int32_t freq[2]; /* shifted period value */

	int x;

	start = square1.timer.cvbc;
	end = (SOUNDTS << 16) / soundtsinc;
	if (end <= start) {
		return;
	}
	square1.timer.cvbc = end;

	for (x = 0; x < 2; x++) {
		SquareUnit *square = x ? &square2 : &square1;

		int y;
		int dutyCycle;
		const uint8_t *dutyTbl = &SquareWaveTable[FSettings.SwapDutyCycles][square->duty][0];

		inie[x] = nesincsize;

		amp[x] = SquareOutput(square);
		amp[x] = GetOutput(SND_SQUARE1 + x, amp[x]);

		for (y = 0; y < 8; y++) {
			ttable[x][y] = dutyTbl[y] * amp[x];
		}

		freq[x] = (square->sweep.pulsePeriod + 1) << 1;
		freq[x] <<= 17;
	}

	totalout = square_mix_table[
		ttable[0][square1.step] +
		ttable[1][square2.step]];

	for (V = start; V < end; V++) {
		Wave[V >> 4] += totalout;

		square1.timer.count2 -= inie[0];
		square2.timer.count2 -= inie[1];

		if (square1.timer.count2 <= 0) {
rea:
			square1.timer.count2 += freq[0];
			square1.step = (square1.step - 1) & 0x07;
			if (square1.timer.count2 <= 0) {
				goto rea;
			}
			totalout = square_mix_table[
				ttable[0][square1.step] +
				ttable[1][square2.step]];
		}

		if (square2.timer.count2 <= 0) {
rea2:
			square2.timer.count2 += freq[1];
			square2.step = (square2.step - 1) & 0x07;
			if (square2.timer.count2 <= 0) {
				goto rea2;
			}
			totalout = square_mix_table[
				ttable[0][square1.step] +
				ttable[1][square2.step]];
		}
	}
}

static void RDoTriangle(void) {
	uint32_t V;
	int32_t triout;

	triout = GetOutput(SND_TRIANGLE, TriangleOutput());
	triout = (triout << TRINPCM_SHIFT) & (~0xFFFF);

	if ((triangle.length.counter == 0) || (triangle.linearCounter == 0) || (FSettings.RemoveTriangleNoise && (triangle.timer.period <= 4))) {
		/* Counter is halted, but we still need to output. */
		for (V = triangle.timer.cvbc; V < SOUNDTS; V++) {
			WaveHi[V] += triout;
		}
	} else {
		for (V = triangle.timer.cvbc; V < SOUNDTS; V++) {
			WaveHi[V] += triout;
			triangle.timer.counter--;
			if (triangle.timer.counter == 0) {
				triangle.timer.counter = triangle.timer.period + 1;
				triangle.stepCounter = (triangle.stepCounter + 1) & 0x1F;
				triout = GetOutput(SND_TRIANGLE, TriangleOutput());
				triout = (triout << TRINPCM_SHIFT) & (~0xFFFF);
			}
		}
	}

	triangle.timer.cvbc = SOUNDTS;
}

static void RDoTriangleNoisePCMLQ(void) {
	int32_t V;
	int32_t start, end;
	int32_t freq[2];
	int32_t inie[2];
	uint32_t amptab[2];
	uint32_t triout;
	uint32_t noiseout;
	uint32_t pcmout;
	int nshift;

	int32_t totalout;
	int32_t wl;

	start = triangle.timer.cvbc;
	end = (SOUNDTS << 16) / soundtsinc;
	if (end <= start) {
		return;
	}
	triangle.timer.cvbc = end;

	inie[0] = inie[1] = nesincsize;

	/* setup triangle params */
	freq[0] = (triangle.timer.period + 1) << 17;
	if ((triangle.length.counter == 0) || (triangle.linearCounter == 0) || (FSettings.RemoveTriangleNoise && (triangle.timer.period <= 4))) {
		inie[0] = 0;
	}

	/* setup noise params */
	wl = noise.timer.period << 17;

	amptab[0] = GetOutput(SND_NOISE, NoiseOutput() * 2);
	amptab[1] = 0;

	if (noise.length.counter == 0) {
		amptab[0] = inie[1] = 0;	/* Quick hack speedup, set inie[1] to 0 */
	}

	triout   = GetOutput(SND_TRIANGLE, TriangleOutput());
	noiseout = amptab[noise.shiftRegister & 0x01];
	pcmout   = GetOutput(SND_DMC, dmc.rawDataLatch);

	totalout = tnd_mix_table[triout + noiseout + pcmout];

	if (inie[0] && inie[1]) {
		for (V = start; V < end; V++) {
			Wave[V >> 4] += totalout;

			triangle.timer.count2 -= inie[0];
			noise.timer.count2 -= inie[1];

			if (triangle.timer.count2 <= 0) {
 rea:
				triangle.timer.count2 += freq[0];	/* t; */
				triangle.stepCounter = (triangle.stepCounter + 1) & 0x1F;
				if (triangle.timer.count2 <= 0) {
					goto rea;
				}
				triout   = GetOutput(SND_TRIANGLE, TriangleOutput());
				totalout = tnd_mix_table[triout + noiseout + pcmout];
			}

			if (noise.timer.count2 <= 0) {
				uint32_t feedback;
 rea2:
				noise.timer.count2 += wl;
				feedback = ((noise.shiftRegister >> 0) & 0x01) ^ ((noise.shiftRegister >> (noise.shortMode ? 6 : 1)) & 0x01);
				noise.shiftRegister = (noise.shiftRegister >> 1) | (feedback << 14);
				if (noise.timer.count2 <= 0) {
					goto rea2;
				}
				noiseout = amptab[noise.shiftRegister & 0x01];
				totalout = tnd_mix_table[triout + noiseout + pcmout];
			}
		}
	} else if (inie[0]) {
		for (V = start; V < end; V++) {
			Wave[V >> 4] += totalout;

			triangle.timer.count2 -= inie[0];

			if (triangle.timer.count2 <= 0) {
 area:
				triangle.timer.count2 += freq[0];	/* t; */
				triangle.stepCounter = (triangle.stepCounter + 1) & 0x1F;
				if (triangle.timer.count2 <= 0) {
					goto area;
				}
				triout   = GetOutput(SND_TRIANGLE, TriangleOutput());
				totalout = tnd_mix_table[triout + noiseout + pcmout];
			}
		}
	} else if (inie[1]) {
		for (V = start; V < end; V++) {
			Wave[V >> 4] += totalout;
			noise.timer.count2 -= inie[1];
			if (noise.timer.count2 <= 0) {
				uint32_t feedback;
 area2:
				noise.timer.count2 += wl;
				feedback = ((noise.shiftRegister >> 0) & 0x01) ^ ((noise.shiftRegister >> (noise.shortMode ? 6 : 1)) & 0x01);
				noise.shiftRegister = (noise.shiftRegister >> 1) | (feedback << 14);
				if (noise.timer.count2 <= 0) {
					goto area2;
				}
				noiseout = amptab[noise.shiftRegister & 0x01];
				totalout = tnd_mix_table[triout + noiseout + pcmout];
			}
		}
	} else {
		for (V = start; V < end; V++) {
			Wave[V >> 4] += totalout;
		}
	}
}

static void RDoNoise(void) {
	uint32_t V;
	int32_t noiseout;
	uint32_t amptab[2];

	amptab[0] = GetOutput(SND_NOISE, NoiseOutput() * 2);
	amptab[0] <<= TRINPCM_SHIFT;
	amptab[1] = 0;

	if (noise.length.counter == 0) {
		noiseout = amptab[0] = 0;
	}

	noiseout = amptab[noise.shiftRegister & 0x01];

	for (V = noise.timer.cvbc; V < SOUNDTS; V++) {
		WaveHi[V] += noiseout;
		noise.timer.counter--;
		if (noise.timer.counter == 0) {
			uint32_t feedback;

			noise.timer.counter = noise.timer.period;
			feedback = ((noise.shiftRegister >> 0) & 0x01) ^ ((noise.shiftRegister >> (noise.shortMode ? 6 : 1)) & 0x01);
			noise.shiftRegister = (noise.shiftRegister >> 1) | (feedback << 14);
			noiseout = amptab[noise.shiftRegister & 0x01];
		}
	}
	noise.timer.cvbc = SOUNDTS;
}

static int32_t inbuf = 0;
int FlushEmulateSound(void) {
	int x;
	int32_t end, left;

	if (!sound_timestamp) {
		return(0);
	}

	if (!FSettings.SndRate) {
		left = 0;
		end = 0;
		goto nosoundo;
	}

	DoSQ1();
	DoSQ2();
	DoTriangle();
	DoNoise();
	DoPCM();

	if (FSettings.soundq >= 1) {
		int32_t *tmpo = &WaveHi[soundtsoffs];

		for (x = 0; x < GAMEEXPSOUND_COUNT; x++) {
			if (GameExpSound[x].HiFill) {
				GameExpSound[x].HiFill();
			}
		}

		for (x = sound_timestamp; x; x--) {
			uint32_t b = *tmpo;
			int32_t square_out = square_mix_table[(b >> SQ_SHIFT) & 0x1F];
			int32_t tnd_out = tnd_mix_table[(b >> TRINPCM_SHIFT) & 0xFF];
			int32_t exp_out = (b & 0xFFFF);

			*tmpo = square_out + tnd_out + exp_out;
			tmpo++;
		}

		end = NeoFilterSound(WaveHi, WaveFinal, SOUNDTS, &left);

		memmove(WaveHi, WaveHi + SOUNDTS - left, left * sizeof(uint32_t));
		memset(WaveHi + left, 0, sizeof(WaveHi) - left * sizeof(uint32_t));

		for (x = 0; x < GAMEEXPSOUND_COUNT; x++) {
			if (GameExpSound[x].HiSync) {
				GameExpSound[x].HiSync(left);
			}
		}
		square1.timer.cvbc = left;
		square2.timer.cvbc = left;
		triangle.timer.cvbc = left;
		noise.timer.cvbc = left;
		dmc.timer.cvbc = left;
	} else {
		end = (SOUNDTS << 16) / soundtsinc;
		for (x = 0; x < GAMEEXPSOUND_COUNT; x++) {
			if (GameExpSound[x].Fill) {
				GameExpSound[x].Fill(end & 0xF);
			}
		}

		SexyFilter(Wave, WaveFinal, end >> 4);

		if (FSettings.lowpass) {
			SexyFilter2(WaveFinal, end >> 4);
		}

		if (end & 0xF) {
			Wave[0] = Wave[(end >> 4)];
		}
		Wave[end >> 4] = 0;
	}
 nosoundo:

	if (FSettings.soundq >= 1) {
		soundtsoffs = left;
	} else {
		square1.timer.cvbc = end & 0xF;
		square2.timer.cvbc = end & 0xF;
		triangle.timer.cvbc = end & 0xF;
		noise.timer.cvbc = end & 0xF;
		dmc.timer.cvbc = end & 0xF;
		soundtsoffs = (soundtsinc * (end & 0xF)) >> 16;
		end >>= 4;
	}
	inbuf = end;

	return(end);
}

int GetSoundBuffer(int32_t **W) {
	*W = WaveFinal;
	return(inbuf);
}

static void APUReset(int hard) {
	SquareReset(&square1, 0, hard);
	SquareReset(&square2, 1, hard);
	TriangleReset(hard);
	NoiseReset(hard);
	DMCReset(hard);
	FrameCounterReset(hard);
}

void FCEUSND_Reset(void) {
	APUReset(0);
}

void FCEUSND_Power(void) {
	soundtsoffs = 0;
	memset(Wave, 0, sizeof(Wave));
	memset(WaveHi, 0, sizeof(WaveHi));

	APUReset(1);
	
	SetWriteHandler(0x4000, 0x4013, PSGWrite);

	SetReadHandler (0x4015, 0x4015, StatusRead);
	SetWriteHandler(0x4015, 0x4015, StatusWrite);
	SetWriteHandler(0x4017, 0x4017, IRQFrameWrite);
}

void SetSoundVariables(void) {
	int x;

	if (FSettings.SndRate) {
		square_mix_table[0] = 0;
		for (x = 1; x < 32; x++) {
			square_mix_table[x] = (uint32_t)(16384.0 * 95.52 / (8128.0 / (double)x + 100.0));
			if (!FSettings.soundq) {
				square_mix_table[x] >>= 4;
			}
		}
		tnd_mix_table[0] = 0;
		for (x = 1; x < 203; x++) {
			tnd_mix_table[x] = (uint32_t)(16384.0 * 163.67 / (24329.0 / (double)x + 100.0));
			if (!FSettings.soundq) {
				tnd_mix_table[x] >>= 4;
			}
		}
		if (FSettings.soundq >= 1) {
			DoNoise = RDoNoise;
			DoTriangle = RDoTriangle;
			DoPCM = RDoPCM;
			DoSQ1 = RDoSQ1;
			DoSQ2 = RDoSQ2;
		} else {
			DoNoise = DoTriangle = DoPCM = DoSQ1 = DoSQ2 = Dummyfunc;
			DoSQ1 = RDoSQLQ;
			DoSQ2 = RDoSQLQ;
			DoTriangle = RDoTriangleNoisePCMLQ;
			DoNoise = RDoTriangleNoisePCMLQ;
			DoPCM = RDoTriangleNoisePCMLQ;
		}
	} else {
		DoNoise = DoTriangle = DoPCM = DoSQ1 = DoSQ2 = Dummyfunc;
		return;
	}

	MakeFilters(FSettings.SndRate);

	for (x = 0; x < GAMEEXPSOUND_COUNT; x++) {
		if (GameExpSound[x].RChange) {
			GameExpSound[x].RChange();
		}
	}

	square1.timer.count2 = 0;
	square2.timer.count2 = 0;

	square1.timer.cvbc = 0;
	square2.timer.cvbc = 0;
	triangle.timer.cvbc = 0;
	noise.timer.cvbc = 0;
	dmc.timer.cvbc = 0;

	LoadDMCPeriod(dmc.periodIndex);	/* For changing from PAL to NTSC */
	LoadNoisePeriod(noise.periodIndex);

	nesincsize = (int64_t)(((int64_t)1 << 17) * (double)(isPAL ? PAL_CPU : NTSC_CPU) / (FSettings.SndRate * 16));
	soundtsinc = (uint32_t)((uint64_t)(isPAL ? (long double)PAL_CPU * 65536 : (long double)NTSC_CPU * 65536) / (FSettings.SndRate * 16));
}

void FCEUI_Sound(int Rate) {
	FSettings.SndRate = Rate;
	SetSoundVariables();
}

void FCEUI_SetLowPass(int q) {
	FSettings.lowpass = q;
}

void FCEUI_SetSoundQuality(int quality) {
	FSettings.soundq = quality;
	SetSoundVariables();
}

void FCEUI_ReduceDmcPopping(int d) {
	FSettings.ReduceDMCPopping = d ? TRUE : FALSE;
}

void FCEUI_ReverseDMCBitOrder(int d) {
	FSettings.ReverseDMCBitOrder = d ? TRUE : FALSE;
}

void FCEUI_RemoveTringleNoise(int d) {
	FSettings.RemoveTriangleNoise = d ? TRUE : FALSE;
}

void FCEUI_SetSoundVolume(int channel, int volume) {
	if (volume > 256) {
		volume = 256;
	}
	if (channel == SND_MASTER) {
		if (volume == 256) {
			volume = 255;
		}
	}
	FSettings.volume[channel] = volume;
}

int FCEUI_GetSoundVolume(int channel) {
	if ((channel >= SND_MASTER) && (channel < SND_LAST)) {
		return FSettings.volume[channel];
	}
	return 0;
}

int32_t GetOutput(int channel, int32_t in) {
	if ((channel > SND_MASTER) && (channel < SND_LAST)) {
		int mod = FCEUI_GetSoundVolume(channel);

		if (in == 0 || mod == 0) {
			return 0; /* silence */
		}
		if (mod != 256) {
			return (int32_t)((in * mod) / 256);
		}
	}
	return in;
}

#define RLSB FCEUSTATE_RLSB
#define state_var(var, varname) { &var, sizeof(var) | RLSB, varname }

SFORMAT FCEUSND_STATEINFO[] = {
	state_var(square1.length.halt, "LHL0"),
	state_var(square1.length.counter, "LCN0"),
	state_var(square1.length.enabled, "LEN0"),
	state_var(square1.length.delayHalt, "LDH0"),
	state_var(square1.length.delayCounter, "LDC0"),
	state_var(square1.length.nextHalt, "LNH0"),
	state_var(square1.length.nextCounter, "LNC0"),

	state_var(square1.envelope.constant, "ECN0"),
	state_var(square1.envelope.counter, "ECT0"),
	state_var(square1.envelope.decay_volume, "EDV0"),
	state_var(square1.envelope.loop, "ELP0"),
	state_var(square1.envelope.reload, "ERL0"),
	state_var(square1.envelope.speed, "ESP0"),

	state_var(square1.sweep.counter, "SCT0"),
	state_var(square1.sweep.enabled, "SEN0"),
	state_var(square1.sweep.negate, "SNG0"),
	state_var(square1.sweep.period, "SPD0"),
	state_var(square1.sweep.pulsePeriod, "SPP0"),
	state_var(square1.sweep.reload, "SRL0"),
	state_var(square1.sweep.shift, "SSH0"),

	state_var(square1.timer.counter, "TCT0"),
	state_var(square1.timer.count2, "TCN0"),
	state_var(square1.timer.period, "TPD0"),

	state_var(square1.duty, "DTY0"),
	state_var(square1.step, "STP0"),

	state_var(square2.length.halt, "LHL1"),
	state_var(square2.length.counter, "LCN1"),
	state_var(square2.length.enabled, "LEN1"),
	state_var(square2.length.delayHalt, "LDH1"),
	state_var(square2.length.delayCounter, "LDC1"),
	state_var(square2.length.nextHalt, "LNH1"),
	state_var(square2.length.nextCounter, "LNC1"),

	state_var(square2.envelope.constant, "ECN1"),
	state_var(square2.envelope.counter, "ECT1"),
	state_var(square2.envelope.decay_volume, "EDV1"),
	state_var(square2.envelope.loop, "ELP1"),
	state_var(square2.envelope.reload, "ERL1"),
	state_var(square2.envelope.speed, "ESP1"),

	state_var(square2.sweep.counter, "SCT1"),
	state_var(square2.sweep.enabled, "SEN1"),
	state_var(square2.sweep.negate, "SNG1"),
	state_var(square2.sweep.period, "SPD1"),
	state_var(square2.sweep.pulsePeriod, "SPP1"),
	state_var(square2.sweep.reload, "SRL1"),
	state_var(square2.sweep.shift, "SSH1"),

	state_var(square2.timer.counter, "TCT1"),
	state_var(square2.timer.count2, "TCN1"),
	state_var(square2.timer.period, "TPD1"),

	state_var(square2.duty, "DTY1"),
	state_var(square2.step, "STP1"),

	state_var(triangle.length.halt, "LHL2"),
	state_var(triangle.length.counter, "LCN2"),
	state_var(triangle.length.enabled, "LEN2"),
	state_var(triangle.length.delayHalt, "LDH2"),
	state_var(triangle.length.delayCounter, "LDC2"),
	state_var(triangle.length.nextHalt, "LNH2"),
	state_var(triangle.length.nextCounter, "LNC2"),

	state_var(triangle.timer.counter, "TCT2"),
	state_var(triangle.timer.count2, "TCN2"),
	state_var(triangle.timer.period, "TPD2"),

	state_var(triangle.linearPeriod, "LLN2"),
	state_var(triangle.linearCounter, "LLC2"),
	state_var(triangle.linearReload, "REL2"),
	state_var(triangle.stepCounter, "STP2"),

	state_var(noise.length.halt, "LHL3"),
	state_var(noise.length.counter, "LCN3"),
	state_var(noise.length.enabled, "LEN3"),
	state_var(noise.length.delayHalt, "LDH3"),
	state_var(noise.length.delayCounter, "LDC3"),
	state_var(noise.length.nextHalt, "LNH3"),
	state_var(noise.length.nextCounter, "LNC3"),

	state_var(noise.envelope.constant, "ECN3"),
	state_var(noise.envelope.counter, "ECT3"),
	state_var(noise.envelope.decay_volume, "EDV3"),
	state_var(noise.envelope.loop, "ELP3"),
	state_var(noise.envelope.reload, "ERL3"),
	state_var(noise.envelope.speed, "ESP3"),

	state_var(noise.timer.counter, "TCT3"),
	state_var(noise.timer.count2, "TCN3"),
	state_var(noise.timer.period, "TPD3"),

	state_var(noise.shiftRegister, "SHR3"),
	state_var(noise.shortMode, "MOD3"),
	state_var(noise.periodIndex, "PID3"),

	state_var(dmc.periodIndex, "PID4"),

	state_var(dmc.rawDataLatch, "DTA4"),

	state_var(dmc.addressLatch, "ADL4"),
	state_var(dmc.lengthLatch, "LNL4"),
	state_var(dmc.irqEnabled, "IRQ4"),
	state_var(dmc.irqPending, "IQP4"),
	state_var(dmc.loop, "LOP4"),
	state_var(dmc.readAddress, "RAD4"),
	state_var(dmc.lengthCounter, "LCN4"),
	state_var(dmc.sampleShiftReg, "SAM4"),

	state_var(dmc.timer.counter, "TCT4"),
	state_var(dmc.timer.count2, "TCN4"),
	state_var(dmc.timer.period, "TPD4"),

	state_var(dmc.bitCounter, "BCN4"),

	state_var(dmc.dmaBufferValid, "BVL4"),
	state_var(dmc.dmaBuffer, "BUF4"),
	state_var(dmc.sampleValid, "SVL4"),

	state_var(frame.counter, "IQFM"),
	state_var(frame.irqInhibit, "IQIH"),
	state_var(frame.irqPending, "IQPN"),
	state_var(frame.mode, "MDFM"),
	state_var(frame.step, "STPF"),
	state_var(frame.delay, "DELF"),
	state_var(frame.newMode, "NMDF"),

	/* less important but still necessary */
	{ &square1.timer.cvbc, sizeof(square1.timer.cvbc) | FCEUSTATE_RLSB, "CBC1" },
	{ &square2.timer.cvbc, sizeof(square2.timer.cvbc) | FCEUSTATE_RLSB, "CBC2" },
	{ &triangle.timer.cvbc, sizeof(triangle.timer.cvbc) | FCEUSTATE_RLSB, "CBC3" },
	{ &noise.timer.cvbc, sizeof(noise.timer.cvbc) | FCEUSTATE_RLSB, "CBC4" },
	{ &dmc.timer.cvbc, sizeof(dmc.timer.cvbc) | FCEUSTATE_RLSB, "CBC5" },
	{ &sound_timestamp, sizeof(sound_timestamp) | FCEUSTATE_RLSB, "SNTS" },
	{ &soundtsoffs, sizeof(soundtsoffs) | FCEUSTATE_RLSB, "TSOF"},
	{ &sexyfilter_acc1, sizeof(sexyfilter_acc1) | FCEUSTATE_RLSB, "FAC1" },
	{ &sexyfilter_acc2, sizeof(sexyfilter_acc2) | FCEUSTATE_RLSB, "FAC2" },
	{ &sexyfilter_acc3, sizeof(sexyfilter_acc3) | FCEUSTATE_RLSB, "FAC3" },

	/* wave buffer is used for filtering, only need first 17 values from it */
	{ &Wave, 32 * sizeof(int32_t), "WAVE"},

	{ 0 }
};

#undef state_var

void FCEUSND_SaveState(void) {
}

void FCEUSND_LoadState(int version) {
	int i;

	LoadNoisePeriod(noise.periodIndex);
	LoadDMCPeriod(dmc.periodIndex);
	dmc.rawDataLatch &= 0x7F;
	dmc.readAddress &= 0x7FFF;

	/* minimal validation */
	for (i = 0; i < 5; i++) {
		uint32_t BC_max = 15;

		if (FSettings.soundq == 2) {
			BC_max = 1025;
		} else if (FSettings.soundq == 1) {
			BC_max = 485;
		}
		if (square1.timer.cvbc > BC_max) square1.timer.cvbc = 0;
		if (square2.timer.cvbc > BC_max) square2.timer.cvbc = 0;
		if (triangle.timer.cvbc > BC_max) triangle.timer.cvbc = 0;
		if (noise.timer.cvbc > BC_max) noise.timer.cvbc = 0;
		if (dmc.timer.cvbc > BC_max) dmc.timer.cvbc = 0;
	}
	if (dmc.timer.counter <= 0) {
		dmc.timer.counter = 1;
	}
	triangle.timer.counter = MAX((int32_t)(1), (int32_t)MIN((int32_t)(0xFFFF), (int32_t)(triangle.timer.counter)));
	noise.timer.counter    = MAX((int32_t)(1), (int32_t)MIN((int32_t)(0xFFFF), (int32_t)(noise.timer.counter)));

	square1.timer.counter = MAX((int32_t) (1), (int32_t) MIN((int32_t) (0xFFFF), (int32_t) (square1.timer.counter)));
	square1.step &= 0x07;
	square1.timer.period &= 0xFFFF;
	square1.sweep.pulsePeriod &= 0xFFFF;
	square1.sweep.shift &= 0x07;
	if (!square1.sweep.shift) {
		square1.sweep.enabled = FALSE;
	}

	square2.timer.counter = MAX((int32_t) (1), (int32_t) MIN((int32_t) (0xFFFF), (int32_t) (square2.timer.counter)));
	square2.step &= 0x07;
	square2.timer.period &= 0xFFFF;
	square2.sweep.pulsePeriod &= 0xFFFF;
	square2.sweep.shift &= 0x07;
	if (!square2.sweep.shift) {
		square2.sweep.enabled = FALSE;
	}

	/* Comparison is always false because access to array >= 0. */
	/* if (sound_timestamp < 0)
	{
		sound_timestamp = 0;
	}
	if (soundtsoffs < 0)
	{
		soundtsoffs = 0;
	} */
	if (soundtsoffs + sound_timestamp >= soundtsinc) {
		soundtsoffs = 0;
		sound_timestamp = 0;
	}
	triangle.stepCounter &= 0x1F;
}
