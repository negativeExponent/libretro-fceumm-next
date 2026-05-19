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
#include "apu.h"
#include "mmc5sound.h"

typedef struct __MMC5PCM {
	uint8_t rawdata;
	uint8_t control;
	struct {
		int32_t cvbc;
	} timer;
} MMC5PCM;

typedef struct __MMC5SOUND {
	SquareUnit square[2]; /* use APU Square struct */
	MMC5PCM pcm;
} MMC5SOUND;

static MMC5SOUND MMC5Sound;

static void (*sfun)(SquareUnit *s);
static void (*psfun)(void);

uint8_t isMMC5Audio = FALSE;

static void Do5PCM(void) {
	int32_t V;
	int32_t start, end;

	start = MMC5Sound.pcm.timer.cvbc;
	end = (SOUNDTS << 16) / soundtsinc;
	if (end <= start) {
		return;
	}
	MMC5Sound.pcm.timer.cvbc = end;

	if (!(MMC5Sound.pcm.control & 0x40) && MMC5Sound.pcm.rawdata) {
		int32_t amp = GetOutput(SND_MMC5, MMC5Sound.pcm.rawdata << 1);

		for (V = start; V < end; V++) {
			Wave[V >> 4] += amp;
		}
	}
}

static void Do5PCMHQ(void) {
	uint32_t V;

	if (!(MMC5Sound.pcm.control & 0x40) && MMC5Sound.pcm.rawdata) {
		int32_t amp = GetOutput(SND_MMC5, MMC5Sound.pcm.rawdata << 5);

		for (V = MMC5Sound.pcm.timer.cvbc; V < SOUNDTS; V++) {
			WaveHi[V] += amp;
		}
	}
	MMC5Sound.pcm.timer.cvbc = SOUNDTS;
}

static INLINE int32_t SquareOutput(SquareUnit *s) {
	if (!s->length.counter) {
		return 0; /* silence */
	}
	if (s->envelope.constant) {
		return s->envelope.speed;
	}
	return s->envelope.decay_volume;
}

static void Do5SQHQ(SquareUnit *s) {
	uint32_t V, amp, wl;
	const uint8_t *dutyTbl = &SquareWaveTable[0][s->duty][0];

	amp = GetOutput(SND_MMC5, SquareOutput(s) << 8);
	wl = (s->timer.period + 1) * 2;

	for (V = s->timer.cvbc; V < SOUNDTS; V++) {
		WaveHi[V] += dutyTbl[s->step] * amp;
		s->timer.counter--;
		if (s->timer.counter <= 0) {
			s->timer.counter += wl;
			s->step = (s->step - 1) & 0x07;
		}
	}

	s->timer.cvbc = SOUNDTS;
}

static void Do5SQ(SquareUnit *s) {
	int32_t V, amp, wl;
	const uint8_t *dutyTbl = &SquareWaveTable[0][s->duty][0];
	int32_t start, end;

	amp = GetOutput(SND_MMC5, SquareOutput(s) << 4);
	wl = (s->timer.period + 1) * 2;
	wl <<= 17;

	start = s->timer.cvbc;
	end = (SOUNDTS << 16) / soundtsinc;
	if (end <= start) {
		return;
	}
	s->timer.cvbc = end;

	for (V = start; V < end; V++) {
		Wave[V >> 4] += dutyTbl[s->step] * amp;
		s->timer.count2 -= nesincsize;
		while (s->timer.count2 <= 0) {
			s->timer.count2 += wl;
			s->step = (s->step - 1) & 0x07;
		}
	}
}

static void MMC5RunSoundHQ(void) {
	Do5SQHQ(&MMC5Sound.square[0]);
	Do5SQHQ(&MMC5Sound.square[1]);
	Do5PCMHQ();
}

static void MMC5HiSync(int32_t ts) {
	MMC5Sound.square[0].timer.cvbc = ts;
	MMC5Sound.square[1].timer.cvbc = ts;
	MMC5Sound.pcm.timer.cvbc = ts;
}

static void MMC5RunSound(int Count) {
	Do5SQ(&MMC5Sound.square[0]);
	Do5SQ(&MMC5Sound.square[1]);
	Do5PCM();
	MMC5Sound.square[0].timer.cvbc = Count;
	MMC5Sound.square[1].timer.cvbc = Count;
	MMC5Sound.pcm.timer.cvbc = Count;
}

static void MMC5Square_Write(SquareUnit *s, uint8_t reg, uint8_t V) {
	switch (reg) {
	case 0:
		s->envelope.speed = V & 0x0F;
		s->envelope.constant = (V & 0x10) ? TRUE : FALSE;
		s->envelope.loop = (V & 0x20) ? TRUE : FALSE;
		s->length.halt = (V & 0x20) ? TRUE : FALSE;
		s->duty = (V & 0xC0) >> 6;
		break;

	case 1:
		/* no sweep unit in MMC5 */
		break;

	case 2:
		s->timer.period = (s->timer.period & 0x0700) | V;
		break;

	case 3:
		s->timer.period = (s->timer.period & 0x00FF) | ((V & 0x07) << 8);
		s->step = 0;
		s->envelope.reload = TRUE;
		if (s->length.enabled) {
			s->length.counter = lengthtable[(V >> 3) & 0x1F];
		}
		break;

	case 4:
		s->length.enabled = V;
		if (!s->length.enabled) {
			s->length.counter = 0;
		}
		break;
	}
}

DECLFR(MMC5Sound_ReadStatus) {
	uint8_t ret = 0;
	if (MMC5Sound.square[0].length.counter) {
		ret |= 0x01;
	}
	if (MMC5Sound.square[1].length.counter) {
		ret |= 0x02;
	}
	return ret;

}

DECLFW(MMC5Sound_Write) {
	GameExpSound[SND_MMC5 - 6].Fill = MMC5RunSound;
	GameExpSound[SND_MMC5 - 6].HiFill = MMC5RunSoundHQ;

	switch (A) {
	case 0x5010:
		if (psfun) {
			psfun();
		}
		MMC5Sound.pcm.control = V;
		break;

	case 0x5011:
		if (psfun) {
			psfun();
		}
		MMC5Sound.pcm.rawdata = V;
		break;

	case 0x5000:
	case 0x5001:
	case 0x5002:
	case 0x5003:
		if (sfun) {
			sfun(&MMC5Sound.square[0]);
		}
		MMC5Square_Write(&MMC5Sound.square[0], A & 0x03, V);
		break;

	case 0x5004:
	case 0x5005:
	case 0x5006:
	case 0x5007:
		if (sfun) {
			sfun(&MMC5Sound.square[1]);
		}
		MMC5Square_Write(&MMC5Sound.square[1], A & 0x03, V);
		break;

	case 0x5015:
		if (sfun) {
			sfun(&MMC5Sound.square[0]);
			sfun(&MMC5Sound.square[1]);
		}
		MMC5Square_Write(&MMC5Sound.square[0], 4, V & 0x01);
		MMC5Square_Write(&MMC5Sound.square[1], 4, V & 0x02);
		break;
	}

}

static void MMC5SC(void) {
	GameExpSound[SND_MMC5 - 6].HiSync = MMC5HiSync;

	MMC5Sound.square[0].timer.counter = 0;
	MMC5Sound.square[1].timer.count2 = 0;

	MMC5Sound.square[0].timer.cvbc = 0;
	MMC5Sound.square[1].timer.cvbc = 0;
	MMC5Sound.pcm.timer.cvbc = 0;

	if (FSettings.SndRate) {
		if (FSettings.soundq >= 1) {
			sfun = Do5SQHQ;
			psfun = Do5PCMHQ;
		} else {
			sfun = Do5SQ;
			psfun = Do5PCM;
		}
	} else {
		sfun = 0;
		psfun = 0;
	}
}

void MMC5Sound_ESI(void) {
	memset(&MMC5Sound, 0, sizeof(MMC5Sound));
	GameExpSound[SND_MMC5 - 6].RChange = MMC5SC;
	MMC5SC();
	isMMC5Audio = TRUE;
}

static void ClockLengthCounter(LengthCount *lc) {
	if (!lc->halt && lc->counter) {
		lc->counter--;
	}
}

static void ClockEnvelope(Envelope *e) {
	if (e->reload) {
		e->counter = e->speed + 1;
		e->decay_volume = 0x0F;
		e->reload = 0;
	} else {
		if (e->counter) {
			e->counter--;
		}
		if (e->counter == 0) {
			e->counter = e->speed + 1;
			if (e->loop || e->decay_volume) {
				e->decay_volume--;
				e->decay_volume &= 0x0F;
			}
		}
	}
}

void MMC5SoundFrameTick(void) {
	if (sfun) {
		sfun(&MMC5Sound.square[0]);
		sfun(&MMC5Sound.square[1]);
	}

	ClockLengthCounter(&MMC5Sound.square[0].length);
	ClockLengthCounter(&MMC5Sound.square[1].length);

	ClockEnvelope(&MMC5Sound.square[0].envelope);
	ClockEnvelope(&MMC5Sound.square[1].envelope);
}

#define state_var_le(var, varname) AddExState( &var, sizeof(var), 1, varname)
#define state_var(var, varname) AddExState( &var, sizeof(var), 0, varname)

void MMC5Sound_AddStateInfo(void) {
	state_var(MMC5Sound.square[0].length.halt, "M0LH");
	state_var(MMC5Sound.square[0].length.counter, "M0LC");
	state_var(MMC5Sound.square[0].length.enabled, "M0LE");

	state_var(MMC5Sound.square[0].envelope.constant, "M0EC");
	state_var(MMC5Sound.square[0].envelope.counter, "M0E2");
	state_var(MMC5Sound.square[0].envelope.decay_volume, "M0DC");
	state_var(MMC5Sound.square[0].envelope.loop, "M0EL");
	state_var(MMC5Sound.square[0].envelope.reload, "M0RL");
	state_var(MMC5Sound.square[0].envelope.speed, "M0SP");

	state_var_le(MMC5Sound.square[0].timer.counter, "M0TC");
	state_var_le(MMC5Sound.square[0].timer.count2, "M0T2");
	state_var_le(MMC5Sound.square[0].timer.period, "M0PD");
	state_var_le(MMC5Sound.square[0].timer.cvbc, "M0BC");

	state_var(MMC5Sound.square[0].duty, "M0DT");
	state_var(MMC5Sound.square[0].step, "M0ST");

	state_var(MMC5Sound.square[1].length.halt, "M1LH");
	state_var(MMC5Sound.square[1].length.counter, "M1LC");
	state_var(MMC5Sound.square[1].length.enabled, "M1LE");

	state_var(MMC5Sound.square[1].envelope.constant, "M1EC");
	state_var(MMC5Sound.square[1].envelope.counter, "M1E2");
	state_var(MMC5Sound.square[1].envelope.decay_volume, "M1DC");
	state_var(MMC5Sound.square[1].envelope.loop, "M1EL");
	state_var(MMC5Sound.square[1].envelope.reload, "M1RL");
	state_var(MMC5Sound.square[1].envelope.speed, "M1SP");

	state_var_le(MMC5Sound.square[1].timer.counter, "M1TC");
	state_var_le(MMC5Sound.square[1].timer.count2, "M1T2");
	state_var_le(MMC5Sound.square[1].timer.period, "M1PD");
	state_var_le(MMC5Sound.square[1].timer.cvbc, "M1BC");

	state_var(MMC5Sound.square[1].duty, "M1DT");
	state_var(MMC5Sound.square[1].step, "M1ST");

	state_var(MMC5Sound.pcm.control, "PCTL");
	state_var(MMC5Sound.pcm.rawdata, "PRAW");
	state_var_le(MMC5Sound.pcm.timer.cvbc, "PCBC");
}
