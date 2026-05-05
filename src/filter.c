#include <math.h>
#include "fceu-types.h"

#include "sound.h"
#include "x6502.h"
#include "fceu.h"
#include "filter.h"

#include "fcoeffs.h"

static int32_t sq2coeffs[SQ2NCOEFFS];
static int32_t coeffs[NCOEFFS];

static uint32_t mrindex;
static uint32_t mrratio;

const int32_t out_max = 32767;
const int32_t out_min = -32768;

int64_t sexyfilter_acc1 = 0;
int64_t sexyfilter_acc2 = 0;
int64_t sexyfilter_acc3 = 0;

#define clamp_sample(V) (((V) < out_min) ? out_min : (((V) > out_max) ? out_max : (V)))

void SexyFilter2(int32_t *in, int32_t count) {
	while (count) {
		int64_t dropcurrent = ((*in << 16) - sexyfilter_acc3) >> (FSettings.lowpass & 0x03);
		sexyfilter_acc3 += dropcurrent;
		*in = sexyfilter_acc3 >> 16;
		in++;
		count--;
	}
}

void SexyFilter(int32_t *in, int32_t *out, int32_t count) {
	int32_t mul1 = (94 << 16) / FSettings.SndRate;
	int32_t mul2 = (24 << 16) / FSettings.SndRate;
	int32_t vmul = (FSettings.volume[SND_MASTER] << 16) * 3 / 4 / 100;

	if (FSettings.soundq) {
		vmul /= 4;
	} else {
		vmul *= 2; /* TODO:  Increase volume in low quality sound rendering code itself */
	}

	while (count) {
		int64_t ino = (int64_t)*in * vmul;
		int32_t t;

		sexyfilter_acc1 += ((ino - sexyfilter_acc1) * mul1) >> 16;
		sexyfilter_acc2 += ((ino - sexyfilter_acc1 - sexyfilter_acc2) * mul2) >> 16;
		*in = 0;
		t = (sexyfilter_acc1 - ino + sexyfilter_acc2) >> 16;
		*out = clamp_sample(t);
		in++;
		out++;
		count--;
	}
}

/* Returns number of samples written to out. */
/* leftover is set to the number of samples that need to be copied
    from the end of in to the beginning of in.
*/

/* static uint32_t mva=1000; */

/* This filtering code assumes that almost all input values stay below 32767.
    Do not adjust the volume in the wlookup tables and the expansion sound
    code to be higher, or you *might* overflow the FIR code.
*/

int32_t NeoFilterSound(int32_t *in, int32_t *out, uint32_t inlen, int32_t *leftover) {
	uint32_t x;
	int32_t *outsave = out;
	int32_t count = 0;
	uint32_t max = (inlen - 1) << 16;

	if (FSettings.soundq == 2) {
		for (x = mrindex; x < max; x += mrratio) {
			int32_t acc = 0, acc2 = 0;
			uint32_t c = SQ2NCOEFFS;
			int32_t *S = &in [(x >> 16) - SQ2NCOEFFS];
			int32_t *D = sq2coeffs;

			while (c) {
				acc += (S[c] * *D) >> 6;
				acc2 += (S[1 + c] * *D) >> 6;
				D++;
				c--;
			}

			acc = ((int64_t)acc * (65536 - (x & 65535)) + (int64_t)acc2 * (x & 65535)) >> (16 + 11);
			*out = acc;
			out++;
			count++;
		}
	} else {
		for (x = mrindex; x < max; x += mrratio) {
			int32_t acc = 0, acc2 = 0;
			uint32_t c = NCOEFFS;
			int32_t *S = &in [(x >> 16) - NCOEFFS];
			int32_t *D = coeffs;

			while (c) {
				acc += (S[c] * *D) >> 6;
				acc2 += (S[1 + c] * *D) >> 6;
				D++;
				c--;
			}

			acc = ((int64_t)acc * (65536 - (x & 65535)) + (int64_t)acc2 * (x & 65535)) >> (16 + 11);
			*out = acc;
			out++;
			count++;
		}
	}

	mrindex = x - max;

	if (FSettings.soundq == 2) {
		mrindex += SQ2NCOEFFS * 65536;
		*leftover = SQ2NCOEFFS + 1;
	} else {
		mrindex += NCOEFFS * 65536;
		*leftover = NCOEFFS + 1;
	}

	for (x = 0; x < GAMEEXPSOUND_COUNT; x++) {
		if (GameExpSound[x].NeoFill) {
			GameExpSound[x].NeoFill(outsave, count);
		}
	}

	SexyFilter(outsave, outsave, count);
	if (FSettings.lowpass) {
		SexyFilter2(outsave, count);
	}
	return (count);
}

void MakeFilters(int32_t rate) {
	int32_t *tabs[6] = { C44100NTSC, C44100PAL, C48000NTSC, C48000PAL, C96000NTSC, C96000PAL };
	int32_t *sq2tabs[6] = { SQ2C44100NTSC, SQ2C44100PAL, SQ2C48000NTSC, SQ2C48000PAL, SQ2C96000NTSC, SQ2C96000PAL };

	int32_t *tmp;
	int32_t x;
	uint32_t nco;

	if (FSettings.soundq == 2) {
		nco = SQ2NCOEFFS;
	} else {
		nco = NCOEFFS;
	}

	mrindex = (nco + 1) << 16;
	mrratio = (isPAL ? (int64_t)(PAL_CPU * 65536) : (int64_t)(NTSC_CPU * 65536)) / rate;

	if (FSettings.soundq == 2) {
		tmp = sq2tabs[(isPAL ? 1 : 0) | (rate == 48000 ? 2 : 0) | (rate >= 96000 ? 4 : 0)];
	} else {
		tmp = tabs[(isPAL ? 1 : 0) | (rate == 48000 ? 2 : 0) | (rate >= 96000 ? 4 : 0)];
	}

	if (FSettings.soundq == 2) {
		for (x = 0; x < (SQ2NCOEFFS >> 1); x++) {
			sq2coeffs[x] = sq2coeffs[SQ2NCOEFFS - 1 - x] = tmp[x];
		}
	} else {
		for (x = 0; x < (NCOEFFS >> 1); x++) {
			coeffs[x] = coeffs[NCOEFFS - 1 - x] = tmp[x];
		}
	}
}
