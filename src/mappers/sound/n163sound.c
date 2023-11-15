/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2023-2024 negativeExponent
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
	uint32 PlayIndex; /* should be phase? */
	int32 vcount;
	int16 output;
} N163Channel;

static N163Channel N163Snd[8];
static uint8 IRAM[128];
static uint8 ramPos;
static uint8 autoInc;
static int32 CVBC;

#define VOLADJ (576716)
#define TOINDEX (16 + 1)

static uint32 GetFrequency(int P) {
	uint8 base = 0x40 + P * 0x08;
	return ((IRAM[base + FREQ_H] & 0x03) << 16) |
	       (IRAM[base + FREQ_M] << 8) |
	       IRAM[base + FREQ_L];
}

static uint32 GetPhase(int P) {
	uint8 base = 0x40 + P * 0x08;
	return (IRAM[base + PHASE_H] << 16) |
	       (IRAM[base + PHASE_M] << 8) |
	       IRAM[base + PHASE_L];
}

static void SetPhase(int P, uint32 phase) {
	uint8 base = 0x40 + P * 0x08;
	IRAM[base + PHASE_H] = (phase >> 16) & 0xFF;
	IRAM[base + PHASE_M] = (phase >> 8) & 0xFF;
	IRAM[base + PHASE_L] = phase & 0xFF;
}

static uint8 GetWaveAddress(int P) {
	uint8 base = 0x40 + P * 0x08;
	return IRAM[base + WAVEADDR];
}

static uint8 GetWaveLength(int P) {
	uint8 base = 0x40 + P * 0x08;
	return 256 - (IRAM[base + WAVELEN] & 0xFC);
}

static uint32 GetVolume(int P) {
	uint8 base = 0x40 + P * 0x08;
	return (IRAM[base + VOLUME] & 0x0F) * VOLADJ;
}

static uint8 GetNumberOfChannels() {
	return (IRAM[0x7F] >> 4) & 0x07;
}

static int16 GetSample(uint8 pos, int volume, int outshift) {
	int8 sample;
	if (pos & 0x01) {
		sample = IRAM[pos >> 1] >> 4;
	} else {
		sample = IRAM[pos >> 1] & 0x0F;
	}
	return ((sample * volume) >> outshift);
}

/* 16:15 */
static void SyncHQ(int32 ts) {
	CVBC = ts;
}

static void DoN163SoundHQ(void) {
	int32 cyclesuck = (GetNumberOfChannels() + 1) * 15;
	int32 P, V;

	for (P = 7; P >= (7 - GetNumberOfChannels()); P--) {
		N163Channel *channel = &N163Snd[P];
		int channelOffset = 0x40 + (P * 8);

		if ((IRAM[FREQ_H + channelOffset] & 0xE0) && (IRAM[VOLUME + channelOffset] & 0xF)) {
			int32 vco = channel->vcount;
			
			uint32 phase = GetPhase(P);
			uint32 freq = GetFrequency(P);
			uint16 length = GetWaveLength(P);
			uint8 offset = GetWaveAddress(P);
			uint32 volume = GetVolume(P);

			for (V = CVBC << 1; V < (int)SOUNDTS << 1; V++) {
				if (vco == 0) {
					vco = cyclesuck;
					channel->PlayIndex += freq;
					while ((channel->PlayIndex >> TOINDEX) >= length) {
						channel->PlayIndex -= length << TOINDEX;
					}
					channel->output = GetSample((channel->PlayIndex >> TOINDEX) + offset, volume, 16);
				}
				vco--;
				WaveHi[V >> 1] += GetOutput(SND_N163, channel->output);
			}
			channel->vcount = vco;
		}
	}
	CVBC = SOUNDTS;
}

static void DoN163Sound(int32 *Wave, int Count) {
	int P, V;

	for (P = 7; P >= (7 - GetNumberOfChannels()); P--) {
		N163Channel *channel = &N163Snd[P];
		int channelOffset = 0x40 + (P * 8);

		if ((IRAM[FREQ_H + channelOffset] & 0xE0) && (IRAM[VOLUME + channelOffset] & 0xF)) {
			int32 vco = channel->vcount;

			uint32 phase = GetPhase(P);
			uint32 freq = GetFrequency(P);
			uint16 length = GetWaveLength(P);
			uint8 offset = GetWaveAddress(P);
			uint32 volume = GetVolume(P);

			int32 inc;

			if (!freq) {
				continue;
			}

			inc = (long double)((int64)FSettings.SndRate << 15) /
			      ((long double)freq * 21477272 /
			          ((long double)0x400000 * (GetNumberOfChannels() + 1) * 45));

			for (V = 0; V < Count * 16; V++) {
				if (vco >= inc) {
					vco -= inc;
					channel->PlayIndex++;
					if (channel->PlayIndex >= length) {
						channel->PlayIndex = 0;
					}
					channel->output = GetSample(
						channel->PlayIndex + offset,
						volume, 19);
				}
				vco += 0x8000;
				Wave[V >> 4] += GetOutput(SND_N163, channel->output);
			}
			channel->vcount = vco;
		}
	}
}

static int dwave = 0;

static void N163Sound(int Count) {
	int32 z, a;

	z = ((SOUNDTS << 16) / soundtsinc) >> 4;
	a = z - dwave;

	if (a) {
		DoN163Sound(&Wave[dwave], a);
	}

	dwave = 0;
}

static void N163SoundHack(void) {
	int32 z, a;

	if (FSettings.soundq >= 1) {
		DoN163SoundHQ();
		return;
	}

	z = ((SOUNDTS << 16) / soundtsinc) >> 4;
	a = z - dwave;

	if (a) {
		DoN163Sound(&Wave[dwave], a);
	}

	dwave += a;
}

DECLFR(N163Sound_Read) {
	uint8 ret = IRAM[ramPos];
/* Maybe I should call N163SoundHack() here? */
#ifdef FCEUDEF_DEBUGGER
	if (!fceuindbg)
#endif
		ramPos = (ramPos + autoInc) & 0x7F;
	return ret;
}

DECLFW(N163Sound_Write) {
	switch (A & 0xF800) {
	case 0x4800:
		if (ramPos & 0x40) {
			if (FSettings.SndRate) {
				N163SoundHack();
				GameExpSound[SND_N163 - 6].Fill = N163Sound;
				GameExpSound[SND_N163 - 6].HiFill = DoN163SoundHQ;
				GameExpSound[SND_N163 - 6].HiSync = SyncHQ;
			}
		}
		IRAM[ramPos] = V;
		ramPos = (ramPos + autoInc) & 0x7F;
		break;
	case 0xF800:
		ramPos = V & 0x7F;
		autoInc = (V & 0x80) >> 7;
		break;
	}
}

uint8 *GetIRAM_ptr(void) {
	return IRAM;
}

uint32 GetIRAM_size(void) {
	return sizeof(IRAM);
}

static void N163SC(void) {
	if (FSettings.SndRate) {
		int i;
		for (i = 0; i < 8; i++) {
			memset(&N163Snd[i].vcount, 0, sizeof(N163Snd[i].vcount));
			memset(&N163Snd[i].PlayIndex, 0, sizeof(N163Snd[i].PlayIndex));
		}
		CVBC = 0;
		ramPos = 0;
	}
}

void N163Sound_ESI(void) {
	GameExpSound[SND_N163 - 6].RChange = N163SC;
	N163SC();

	if (iNESCart.battery) {
		memset(IRAM, 0, sizeof(IRAM));
	} else {
		FCEU_MemoryRand(IRAM, sizeof(IRAM));
	}
}

void N163Sound_AddStateInfo(void) {
	AddExState(IRAM,  0x80, 0, "IRAM");
	AddExState(&CVBC,    4, 0, "BC00");
	AddExState(&ramPos,  1, 0, "INDX");
	AddExState(&autoInc, 1, 0, "INCR");

	AddExState(&N163Snd[0].vcount,    4, 0, "C0VC");
	AddExState(&N163Snd[0].PlayIndex, 4, 0, "C0PI");
	AddExState(&N163Snd[0].output,    2, 0, "C0OP");

	AddExState(&N163Snd[1].vcount,    4, 0, "C1VC");
	AddExState(&N163Snd[1].PlayIndex, 4, 0, "C1PI");
	AddExState(&N163Snd[1].output,    2, 0, "C1OP");

	AddExState(&N163Snd[2].vcount,    4, 0, "C2VC");
	AddExState(&N163Snd[2].PlayIndex, 4, 0, "C2PI");
	AddExState(&N163Snd[2].output,    2, 0, "C2OP");

	AddExState(&N163Snd[3].vcount,    4, 0, "C3VC");
	AddExState(&N163Snd[3].PlayIndex, 4, 0, "C3PI");
	AddExState(&N163Snd[3].output,    2, 0, "C3OP");

	AddExState(&N163Snd[4].vcount,    4, 0, "C4VC");
	AddExState(&N163Snd[4].PlayIndex, 4, 0, "C4PI");
	AddExState(&N163Snd[4].output,    2, 0, "C4OP");

	AddExState(&N163Snd[5].vcount,    4, 0, "C5VC");
	AddExState(&N163Snd[5].PlayIndex, 4, 0, "C5PI");
	AddExState(&N163Snd[5].output,    2, 0, "C5OP");

	AddExState(&N163Snd[6].vcount,    4, 0, "C6VC");
	AddExState(&N163Snd[6].PlayIndex, 4, 0, "C6PI");
	AddExState(&N163Snd[6].output,    2, 0, "C6OP");

	AddExState(&N163Snd[7].vcount,    4, 0, "C7VC");
	AddExState(&N163Snd[7].PlayIndex, 4, 0, "C7PI");
	AddExState(&N163Snd[7].output,    2, 0, "C7OP");
}

#else /* OLD N163 SOUND CODE */
static uint8 ramPos = 0;
static uint8 IRAM[128];

static uint32 FreqCache[8];
static uint32 EnvCache[8];
static uint32 LengthCache[8];

static void FixCache(int a, int V) {
	int w = (a >> 3) & 0x7;
	switch (a & 0x07) {
	case 0x00: FreqCache[w] &= ~0x000000FF; FreqCache[w] |= V; break;
	case 0x02: FreqCache[w] &= ~0x0000FF00; FreqCache[w] |= V << 8; break;
	case 0x04:
		FreqCache[w] &= ~0x00030000; FreqCache[w] |= (V & 3) << 16;
		LengthCache[w] = 256 - (V & 0xFC);
		break;
	case 0x07: EnvCache[w] = (double)(V & 0xF) * 576716; break;
	}
}

static void NamcoSound(int Count);
static void NamcoSoundHack(void);
static void DoNamcoSound(int32 *Wave, int Count);
static void DoNamcoSoundHQ(void);
static void SyncHQ(int32 ts);

static int dwave = 0;

static void NamcoSoundHack(void) {
	int32 z, a;
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
	int32 z, a;
	z = ((SOUNDTS << 16) / soundtsinc) >> 4;
	a = z - dwave;
	if (a) DoNamcoSound(&Wave[dwave], a);
	dwave = 0;
}

static uint32 PlayIndex[8];
static int32 vcount[8];
static int32 CVBC;

#define TOINDEX        (16 + 1)

static void SyncHQ(int32 ts) {
	CVBC = ts;
}


/* Things to do:
	1        Read freq low
	2        Read freq mid
	3        Read freq high
	4        Read envelope
	...?
*/

static INLINE uint32 FetchDuff(uint32 P, uint32 envelope) {
	uint32 duff;
	duff = IRAM[((IRAM[0x46 + (P << 3)] + (PlayIndex[P] >> TOINDEX)) & 0xFF) >> 1];
	if ((IRAM[0x46 + (P << 3)] + (PlayIndex[P] >> TOINDEX)) & 1)
		duff >>= 4;
	duff &= 0xF;
	duff = (duff * envelope) >> 16;
	return(duff);
}

static void DoNamcoSoundHQ(void) {
	int32 P, V;
	int32 cyclesuck = (((IRAM[0x7F] >> 4) & 7) + 1) * 15;

	for (P = 7; P >= (7 - ((IRAM[0x7F] >> 4) & 7)); P--) {
		if ((IRAM[0x44 + (P << 3)] & 0xE0) && (IRAM[0x47 + (P << 3)] & 0xF)) {
			uint32 freq;
			int32 vco;
			uint32 duff2, lengo, envelope;

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


static void DoNamcoSound(int32 *Wave, int Count) {
	int P, V;
	for (P = 7; P >= 7 - ((IRAM[0x7F] >> 4) & 7); P--) {
		if ((IRAM[0x44 + (P << 3)] & 0xE0) && (IRAM[0x47 + (P << 3)] & 0xF)) {
			int32 inc;
			uint32 freq;
			int32 vco;
			uint32 duff, duff2, lengo, envelope;

			vco = vcount[P];
			freq = FreqCache[P];
			envelope = EnvCache[P];
			lengo = LengthCache[P];

			if (!freq) { /*printf("Ack");*/
				continue;
			}

			{
				int c = ((IRAM[0x7F] >> 4) & 7) + 1;
				inc = (long double)(FSettings.SndRate << 15) / ((long double)freq * 21477272 / ((long double)0x400000 * c * 45));
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
				Wave[V >> 4] += duff2;
				vco += 0x8000;
			}
			vcount[P] = vco;
		}
	}
}

DECLFR(N163Sound_Read)
{
	uint8 ret=IRAM[ramPos&0x7f];

	/* Maybe I should call DoNamcoSound() here? */
	if(!fceuindbg)
	 if(ramPos&0x80)
	  ramPos=(ramPos&0x80)|((ramPos+1)&0x7f);
	return ret;
}

DECLFW(N163Sound_Write) {
	switch (A & 0xF800) {
	case 0x4800:
		if (ramPos & 0x40) {
			if (FSettings.SndRate)
			{
				NamcoSoundHack();
				GameExpSound[SND_N163 - 6].Fill = NamcoSound;
				GameExpSound[SND_N163 - 6].HiFill = DoNamcoSoundHQ;
				GameExpSound[SND_N163 - 6].HiSync = SyncHQ;
			}
			FixCache(ramPos, V);
		}
		IRAM[ramPos & 0x7f] = V;
		if (ramPos & 0x80)
			ramPos = (ramPos & 0x80) | ((ramPos + 1) & 0x7f);
		break;
		case 0xF800:
		ramPos = V;
		break;
	}
}

uint8 *GetIRAM_ptr(void) {
	return IRAM;
}

uint32 GetIRAM_size(void) {
	return sizeof(IRAM);
}

static void M19SC(void) {
	if (FSettings.SndRate)
		N163Sound_ESI();
}

void N163Sound_ESI(void) {
	GameExpSound[SND_N163 - 6].RChange = M19SC;
	memset(vcount, 0, sizeof(vcount));
	memset(PlayIndex, 0, sizeof(PlayIndex));
	CVBC = 0;
}

void N163Sound_AddStateInfo(void) {
}
#endif
