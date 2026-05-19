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

/* TODO: Things to do:
    1        Read freq low
    2        Read freq mid
    3        Read freq high
    4        Read volume
    ...?
*/

#include "mapinc.h"
#include "n163sound.h"

/* #define USE_OLD_N163 */

#ifndef USE_OLD_N163 
enum SoundReg {
	FREQ_L = 0x00,
	PHASE_L = 0x01,
	FREQ_M = 0x02,
	PHASE_M = 0x03,
	FREQ_H = 0x04,
	WAVELEN = 0x04,
	PHASE_H = 0x05,
	WAVEADDR = 0x06,
	VOLUME = 0x07
};

typedef struct N163Channel {
	uint32_t PlayIndex; /* should be phase? */
	int32_t vcount;
	int16_t output;
} N163Channel;

static struct N163Sound {
	N163Channel channel[8];
	uint8_t soundAddr;
	uint8_t autoIncrement;

	int32_t cvcb;
	int32_t dwave;

	uint8_t *internalRAM;
} N163Sound;

#define VOLADJ            (576716)
#define TOINDEX           (16 + 1)
#define INTERNAL_RAM_SIZE 128

static uint32_t GetFrequency(int P) {
	uint8_t base = 0x40 + P * 0x08;
	return ((N163Sound.internalRAM[base + FREQ_H] & 0x03) << 16) |
	       (N163Sound.internalRAM[base + FREQ_M] << 8) |
	       N163Sound.internalRAM[base + FREQ_L];
}

static uint32_t GetPhase(int P) {
	uint8_t base = 0x40 + P * 0x08;
	return (N163Sound.internalRAM[base + PHASE_H] << 16) |
	       (N163Sound.internalRAM[base + PHASE_M] << 8) |
	       N163Sound.internalRAM[base + PHASE_L];
}

static void SetPhase(int P, uint32_t phase) {
	uint8_t base = 0x40 + P * 0x08;
	N163Sound.internalRAM[base + PHASE_H] = (phase >> 16) & 0xFF;
	N163Sound.internalRAM[base + PHASE_M] = (phase >> 8) & 0xFF;
	N163Sound.internalRAM[base + PHASE_L] = phase & 0xFF;
}

static uint8_t GetWaveAddress(int P) {
	uint8_t base = 0x40 + P * 0x08;
	return N163Sound.internalRAM[base + WAVEADDR];
}

static uint16_t GetWaveLength(int P) {
	uint8_t base = 0x40 + P * 0x08;
	return 256 - (N163Sound.internalRAM[base + WAVELEN] & 0xFC);
}

static uint8_t GetVolume(int P) {
	uint8_t base = 0x40 + P * 0x08;
	return (N163Sound.internalRAM[base + VOLUME] & 0x0F);
}

static uint8_t GetNumberOfChannels() {
	return (N163Sound.internalRAM[0x7F] >> 4) & 0x07;
}

/* 16:15 */
static void SyncHQ(int32_t ts) {
	N163Sound.cvcb = ts;
}

static void DoN163SoundHQ(void) {
	int32_t cyclesuck = (GetNumberOfChannels() + 1) * 15;
	int32_t P, V;

	for (P = 7; P >= (7 - GetNumberOfChannels()); P--) {
		N163Channel *channel = &N163Sound.channel[P];
		int channelOffset = 0x40 + (P * 8);

		if ((N163Sound.internalRAM[FREQ_H + channelOffset] & 0xE0) && (N163Sound.internalRAM[VOLUME + channelOffset] & 0xF)) {
			int32_t vco = channel->vcount;
			uint32_t freq = GetFrequency(P);
			uint16_t length = GetWaveLength(P);
			uint8_t offset = GetWaveAddress(P);
			uint32_t volume = GetVolume(P);

			for (V = N163Sound.cvcb << 1; V < (int)SOUNDTS << 1; V++) {
				if (vco == 0) {
					uint8_t sample, samplePosition;

					vco = cyclesuck;
					channel->PlayIndex += freq;
					while ((channel->PlayIndex >> TOINDEX) >= length) {
						channel->PlayIndex -= length << TOINDEX;
					}
					samplePosition = ((channel->PlayIndex >> TOINDEX) + offset) & 0xFF;
					if (samplePosition & 0x01) {
						sample = N163Sound.internalRAM[samplePosition >> 1] >> 4;
					} else {
						sample = N163Sound.internalRAM[samplePosition >> 1] & 0x0F;
					}
					channel->output = (sample * volume * VOLADJ) >> 16;
				}
				vco--;
				WaveHi[V >> 1] += GetOutput(SND_N163, channel->output);
			}
			channel->vcount = vco;
		}
	}
	N163Sound.cvcb = SOUNDTS;
}

static void DoN163Sound(int32_t *Wave, int Count) {
	int P, V;

	for (P = 7; P >= (7 - GetNumberOfChannels()); P--) {
		N163Channel *channel = &N163Sound.channel[P];
		int channelOffset = 0x40 + (P * 8);

		if ((N163Sound.internalRAM[FREQ_H + channelOffset] & 0xE0) && (N163Sound.internalRAM[VOLUME + channelOffset] & 0xF)) {
			int32_t vco = channel->vcount;
			uint32_t freq = GetFrequency(P);
			uint16_t length = GetWaveLength(P);
			uint8_t offset = GetWaveAddress(P);
			uint32_t volume = GetVolume(P);

			int32_t inc;

			if (!freq) {
				continue;
			}

			inc = (long double)((int64_t)FSettings.SndRate << 15) /
			      ((long double)freq * 21477272 /
			          ((long double)0x400000 * (GetNumberOfChannels() + 1) * 45));

			for (V = 0; V < Count * 16; V++) {
				if (vco >= inc) {
					uint8_t sample, samplePosition;

					vco -= inc;
					channel->PlayIndex++;
					if (channel->PlayIndex >= length) {
						channel->PlayIndex = 0;
					}
					samplePosition = (channel->PlayIndex + offset) & 0xFF;
					if (samplePosition & 0x01) {
						sample = N163Sound.internalRAM[samplePosition >> 1] >> 4;
					} else {
						sample = N163Sound.internalRAM[samplePosition >> 1] & 0x0F;
					}
					channel->output = (sample * volume * VOLADJ) >> 19;
				}
				vco += 0x8000;
				Wave[V >> 4] += GetOutput(SND_N163, channel->output);
			}
			channel->vcount = vco;
		}
	}
}

static void N163SoundFill(int Count) {
	int32_t z, a;

	z = ((SOUNDTS << 16) / soundtsinc) >> 4;
	a = z - N163Sound.dwave;

	if (a) {
		DoN163Sound(&Wave[N163Sound.dwave], a);
	}

	N163Sound.dwave = 0;
}

static void N163SoundHack(void) {
	int32_t z, a;

	if (FSettings.soundq >= 1) {
		DoN163SoundHQ();
		return;
	}

	z = ((SOUNDTS << 16) / soundtsinc) >> 4;
	a = z - N163Sound.dwave;

	if (a) {
		DoN163Sound(&Wave[N163Sound.dwave], a);
	}

	N163Sound.dwave += a;
}

DECLFR(N163Sound_Read) {
	uint8_t ret = N163Sound.internalRAM[N163Sound.soundAddr];
/* Maybe I should call N163SoundHack() here? */
#ifdef FCEUDEF_DEBUGGER
	if (!fceuindbg)
#endif
		N163Sound.soundAddr = (N163Sound.soundAddr + N163Sound.autoIncrement) & 0x7F;
	return ret;
}

DECLFW(N163Sound_Write) {
	switch (A & 0xF800) {
	case 0x4800:
		if (N163Sound.soundAddr & 0x40) {
			if (FSettings.SndRate) {
				N163SoundHack();
				GameExpSound[SND_N163 - 6].Fill = N163SoundFill;
				GameExpSound[SND_N163 - 6].HiFill = DoN163SoundHQ;
				GameExpSound[SND_N163 - 6].HiSync = SyncHQ;
			}
		}
		N163Sound.internalRAM[N163Sound.soundAddr] = V;
		N163Sound.soundAddr = (N163Sound.soundAddr + N163Sound.autoIncrement) & 0x7F;
		break;
	case 0xF800:
		N163Sound.soundAddr = V & 0x7F;
		N163Sound.autoIncrement = (V & 0x80) >> 7;
		break;
	}
}

static void N163SC(void) {
	if (FSettings.SndRate) {
		int i;
		for (i = 0; i < 8; i++) {
			memset(&N163Sound.channel[i].vcount, 0, sizeof(N163Sound.channel[i].vcount));
			memset(&N163Sound.channel[i].PlayIndex, 0, sizeof(N163Sound.channel[i].PlayIndex));
		}
	}
	N163Sound.cvcb = 0;
	N163Sound.soundAddr = 0;
}

void N163Sound_ESI(uint8_t *ptr) {
	memset(&N163Sound, 0, sizeof(N163Sound));
	N163Sound.internalRAM = ptr;
	GameExpSound[SND_N163 - 6].RChange = N163SC;
	N163SC();
	memset(N163Sound.internalRAM, 0, INTERNAL_RAM_SIZE);
}

void N163Sound_AddStateInfo(void) {
	AddExState(&N163Sound.internalRAM, 0x80 | FCEUSTATE_INDIRECT, 0, "N163Sound.internalRAM");
	AddExState(&N163Sound.cvcb, 4, 1, "BC00");
	AddExState(&N163Sound.soundAddr, 1, 0, "INDX");
	AddExState(&N163Sound.autoIncrement, 1, 0, "INCR");

	AddExState(&N163Sound.channel[0].vcount, 4, 1, "C0VC");
	AddExState(&N163Sound.channel[0].PlayIndex, 4, 1, "C0PI");
	AddExState(&N163Sound.channel[0].output, 2, 1, "C0OP");

	AddExState(&N163Sound.channel[1].vcount, 4, 1, "C1VC");
	AddExState(&N163Sound.channel[1].PlayIndex, 4, 1, "C1PI");
	AddExState(&N163Sound.channel[1].output, 2, 1, "C1OP");

	AddExState(&N163Sound.channel[2].vcount, 4, 1, "C2VC");
	AddExState(&N163Sound.channel[2].PlayIndex, 4, 1, "C2PI");
	AddExState(&N163Sound.channel[2].output, 2, 1, "C2OP");

	AddExState(&N163Sound.channel[3].vcount, 4, 1, "C3VC");
	AddExState(&N163Sound.channel[3].PlayIndex, 4, 1, "C3PI");
	AddExState(&N163Sound.channel[3].output, 2, 1, "C3OP");

	AddExState(&N163Sound.channel[4].vcount, 4, 1, "C4VC");
	AddExState(&N163Sound.channel[4].PlayIndex, 4, 1, "C4PI");
	AddExState(&N163Sound.channel[4].output, 2, 1, "C4OP");

	AddExState(&N163Sound.channel[5].vcount, 4, 1, "C5VC");
	AddExState(&N163Sound.channel[5].PlayIndex, 4, 1, "C5PI");
	AddExState(&N163Sound.channel[5].output, 2, 1, "C5OP");

	AddExState(&N163Sound.channel[6].vcount, 4, 1, "C6VC");
	AddExState(&N163Sound.channel[6].PlayIndex, 4, 1, "C6PI");
	AddExState(&N163Sound.channel[6].output, 2, 1, "C6OP");

	AddExState(&N163Sound.channel[7].vcount, 4, 1, "C7VC");
	AddExState(&N163Sound.channel[7].PlayIndex, 4, 1, "C7PI");
	AddExState(&N163Sound.channel[7].output, 2, 1, "C7OP");
}

#else /* OLD N163 SOUND CODE */
static uint8_t dopol = 0;
static uint8_t *IRAM;

static uint32_t FreqCache[8];
static uint32_t EnvCache[8];
static uint32_t LengthCache[8];

static void FixCache(int a, int V) {
	int w = (a >> 3) & 0x7;
	switch (a & 0x07) {
	case 0x00: FreqCache[w] &= ~0x000000FF; FreqCache[w] |= V; break;
	case 0x02: FreqCache[w] &= ~0x0000FF00; FreqCache[w] |= V << 8; break;
	case 0x04:
		FreqCache[w] &= ~0x00030000; FreqCache[w] |= (V & 3) << 16;
		/* something wrong here http://www.romhacking.net/forum/index.php?topic=21907.msg306903#msg306903 */
		/* LengthCache[w] = (8 - ((V >> 2) & 7)) << 2; */
		/* fix be like in https://github.com/SourMesen/Mesen/blob/cda0a0bdcb5525480784f4b8c71de6fc7273b570/Core/Namco163Audio.h#L61 */
		LengthCache[w] = 256 - (V & 0xFC);
		break;
	case 0x07: EnvCache[w] = (double)(V & 0xF) * 576716; break;
	}
}

static void NamcoSound(int Count);
static void NamcoSoundHack(void);
static void DoNamcoSound(int32_t *Wave, int Count);
static void DoNamcoSoundHQ(void);
static void SyncHQ(int32_t ts);

static int dwave = 0;

static void NamcoSoundHack(void) {
	int32_t z, a;
	if (FSettings.soundq >= 1) {
		DoNamcoSoundHQ();
		return;
	}
	z = ((SOUNDTS << 16) / soundtsinc) >> 4;
	a = z - dwave;
	if (a) DoNamcoSound(&Wave[dwave], a);
	dwave += a;
}

static void NamcoSound(int Count) {
	int32_t z, a;
	z = ((SOUNDTS << 16) / soundtsinc) >> 4;
	a = z - dwave;
	if (a) DoNamcoSound(&Wave[dwave], a);
	dwave = 0;
}

static uint32_t PlayIndex[8];
static int32_t vcount[8];
static int32_t CVBC;

#define TOINDEX        (16 + 1)

static void SyncHQ(int32_t ts) {
	CVBC = ts;
}

/* Things to do:
	1        Read freq low
	2        Read freq mid
	3        Read freq high
	4        Read envelope
	...?
*/

static INLINE uint32_t FetchDuff(uint32_t P, uint32_t envelope) {
	uint32_t duff;
	duff = IRAM[((IRAM[0x46 + (P << 3)] + (PlayIndex[P] >> TOINDEX)) & 0xFF) >> 1];
	if ((IRAM[0x46 + (P << 3)] + (PlayIndex[P] >> TOINDEX)) & 1)
		duff >>= 4;
	duff &= 0xF;
	duff = (duff * envelope) >> 16;
	return(duff);
}

static void DoNamcoSoundHQ(void) {
	int32_t P, V;
	int32_t cyclesuck = (((IRAM[0x7F] >> 4) & 7) + 1) * 15;

	for (P = 7; P >= (7 - ((IRAM[0x7F] >> 4) & 7)); P--) {
		if ((IRAM[0x44 + (P << 3)] & 0xE0) && (IRAM[0x47 + (P << 3)] & 0xF)) {
			uint32_t freq;
			int32_t vco;
			uint32_t duff2, lengo, envelope;

			vco = vcount[P];
			freq = FreqCache[P];
			envelope = EnvCache[P];
			lengo = LengthCache[P];

			duff2 = FetchDuff(P, envelope);
			for (V = CVBC << 1; V < (int)SOUNDTS << 1; V++) {
				WaveHi[V >> 1] += duff2;
				if (!vco) {
					PlayIndex[P] += freq;
					while ((PlayIndex[P] >> TOINDEX) >= lengo) PlayIndex[P] -= lengo << TOINDEX;
					duff2 = FetchDuff(P, envelope);
					vco = cyclesuck;
				}
				vco--;
			}
			vcount[P] = vco;
		}
	}
	CVBC = SOUNDTS;
}


static void DoNamcoSound(int32_t *WaveBuf, int Count) {
	int P, V;
	for (P = 7; P >= 7 - ((IRAM[0x7F] >> 4) & 7); P--) {
		if ((IRAM[0x44 + (P << 3)] & 0xE0) && (IRAM[0x47 + (P << 3)] & 0xF)) {
			int32_t inc;
			uint32_t freq;
			int32_t vco;
			uint32_t duff, duff2, lengo, envelope;

			vco = vcount[P];
			freq = FreqCache[P];
			envelope = EnvCache[P];
			lengo = LengthCache[P];

			if (!freq)
				continue;

			{
				int c = ((IRAM[0x7F] >> 4) & 7) + 1;
				/* Use double rather than long double for cross-platform
				 * FP determinism (long double is 80-bit on x87, 64-bit
				 * with SSE math, 128-bit elsewhere - the truncated
				 * int32 result varies). */
				inc = (int32_t)((double)(FSettings.SndRate << 15) / ((double)freq * 21477272.0 / ((double)0x400000 * c * 45)));
			}

			duff = IRAM[(((IRAM[0x46 + (P << 3)] + PlayIndex[P]) & 0xFF) >> 1)];
			if ((IRAM[0x46 + (P << 3)] + PlayIndex[P]) & 1)
				duff >>= 4;
			duff &= 0xF;
			duff2 = (duff * envelope) >> 19;
			for (V = 0; V < Count * 16; V++) {
				if (vco >= inc) {
					PlayIndex[P]++;
					if (PlayIndex[P] >= lengo)
						PlayIndex[P] = 0;
					vco -= inc;
					duff = IRAM[(((IRAM[0x46 + (P << 3)] + PlayIndex[P]) & 0xFF) >> 1)];
					if ((IRAM[0x46 + (P << 3)] + PlayIndex[P]) & 1)
						duff >>= 4;
					duff &= 0xF;
					duff2 = (duff * envelope) >> 19;
				}
				WaveBuf[V >> 4] += duff2;
				vco += 0x8000;
			}
			vcount[P] = vco;
		}
	}
}

DECLFR(N163Sound_Read) {
	uint8_t ret = IRAM[dopol & 0x7f];
	/* Maybe I should call NamcoSoundHack() here? */
	if (dopol & 0x80)
		dopol = (dopol & 0x80) | ((dopol + 1) & 0x7f);
	return ret;
}

DECLFW(N163Sound_Write) {
	switch (A) {
	case 0x4800:
		if (dopol & 0x40) {
			if (FSettings.SndRate) {
				NamcoSoundHack();
				GameExpSound[SND_N163 - 6].Fill = NamcoSound;
				GameExpSound[SND_N163 - 6].HiFill = DoNamcoSoundHQ;
				GameExpSound[SND_N163 - 6].HiSync = SyncHQ;
			}
			FixCache(dopol, V);
		}
		IRAM[dopol & 0x7f] = V;
		if (dopol & 0x80)
			dopol = (dopol & 0x80) | ((dopol + 1) & 0x7f);
		break;
	case 0xf800:
		dopol = V;
		break;
	}
}

static void M19SC(void) {
	if (FSettings.SndRate) {
		memset(vcount, 0, sizeof(vcount));
		memset(PlayIndex, 0, sizeof(PlayIndex));
		CVBC = 0;
	}
}

void N163Sound_ESI(uint8_t *ptr) {
	int i;

	IRAM = ptr;
	memset(FreqCache, 0, sizeof(FreqCache));
	memset(EnvCache, 0, sizeof(EnvCache));
	memset(LengthCache, 0, sizeof(LengthCache));
	for (i = 0x40; i < 0x80; i++) {
		FixCache(i, IRAM[i]);
	}
	GameExpSound[SND_N163 - 6].RChange = M19SC;
	M19SC();
}

static SFORMAT N163_SStateRegs[] = {
	{ &PlayIndex[0], 4 | FCEUSTATE_RLSB, "IDX0" },
	{ &PlayIndex[1], 4 | FCEUSTATE_RLSB, "IDX1" },
	{ &PlayIndex[2], 4 | FCEUSTATE_RLSB, "IDX2" },
	{ &PlayIndex[3], 4 | FCEUSTATE_RLSB, "IDX3" },
	{ &PlayIndex[4], 4 | FCEUSTATE_RLSB, "IDX4" },
	{ &PlayIndex[5], 4 | FCEUSTATE_RLSB, "IDX5" },
	{ &PlayIndex[6], 4 | FCEUSTATE_RLSB, "IDX6" },
	{ &PlayIndex[7], 4 | FCEUSTATE_RLSB, "IDX7" },
	{ &vcount[0], 4 | FCEUSTATE_RLSB, "VCT0" },
	{ &vcount[1], 4 | FCEUSTATE_RLSB, "VCT1" },
	{ &vcount[2], 4 | FCEUSTATE_RLSB, "VCT2" },
	{ &vcount[3], 4 | FCEUSTATE_RLSB, "VCT3" },
	{ &vcount[4], 4 | FCEUSTATE_RLSB, "VCT4" },
	{ &vcount[5], 4 | FCEUSTATE_RLSB, "VCT5" },
	{ &vcount[6], 4 | FCEUSTATE_RLSB, "VCT6" },
	{ &vcount[7], 4 | FCEUSTATE_RLSB, "VCT7" },
	{ &CVBC, 4 | FCEUSTATE_RLSB, "BC01" },
	{ 0 }
};

void N163Sound_AddStateInfo(void) {
	AddExState( N163_SStateRegs, ~0, 0, 0);
}
#endif
