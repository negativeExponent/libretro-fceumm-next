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

#ifndef _FDS_APU_H
#define _FDS_APU_H

#define FDSClock (NTSC_CLOCK_SPEED / 2)

enum { OPENBUS = 0x40 };
enum { EVOL = 0, EMOD };
enum {
	ENV_CTRL_INCREASE = 0x40,
	ENV_CTRL_DISABLE  = 0x80,
	ENVELOPES_DISABLE = 0x40,
	WAVE_DISABLE      = 0x80,
	MOD_WRITE_MODE    = 0x80,
	MASTER_VOLUME     = 0x03,
	WAVE_WRITE_MODE   = 0x80
};
enum {
	VOLUME_MIN        = 0,
	VOLUME_MAX        = 32
};

typedef struct __FDSENVUNIT {
	uint8_t speed;
	uint8_t volume;  /* Current volumes. */
	uint8_t control; /* $4080/$4084 with low 6bits masked */
	int32_t counter; /**/
} FDSENVUNIT;

typedef struct __FDSSOUND {
	int64_t cycles; /* fds cycles to run */
	int64_t count;  /* current cycle count */

	FDSENVUNIT EnvUnits[2];

	int32_t env_divider; /* Main envelope clock divider. */

	uint16_t cwave_freq;   /* $4082 and lower 4 bits of $4083 */
	uint32_t cwave_pos;    /* main phase */
	uint8_t cwave_control; /* $4083 with low 6bits masked */

	uint16_t mod_freq;   /* $4086 and lower 4 bits of $4087 */
	uint32_t mod_pos;    /* Should be named "mwave_pos", but "mod_pos" distinguishes it more. */
	uint8_t mod_disabled; /* $4087 bit7 */

	uint8_t master_control;   /* $4089 with lower 6 bits masked */
	uint8_t master_env_speed; /* Master envelope speed controller($408A). */

	int32_t mwave[0x20]; /* Modulation waveform.  Stored in expanded(after LUT) */
	                   /* form. Set to 0x10 if the original value is */
	                   /* 0x4(reset sweep bias accumulator). */
	uint8_t cwave[0x40]; /* Game-defined waveform(carrier) */
	uint32_t sweep_bias;

	int32_t mod_output;  /* output from modulation engine */
	int32_t sample_out_cache; /* holds the last output from wave carrier */

	uint8_t cwave_pos_shift;
	uint8_t env_count_mul;
	uint8_t mod_pos_shift;
	uint8_t mod_overflow_shift;
} FDSSOUND;

DECLFR(FDSWaveRead);
DECLFW(FDSWaveWrite);
DECLFW(FDSSReg0Write);
DECLFW(FDSSReg1Write);
DECLFW(FDSSReg2Write);
DECLFW(FDSSReg3Write);
DECLFW(FDSSReg4Write);
DECLFW(FDSSReg5Write);
DECLFW(FDSSReg6Write);
DECLFW(FDSSReg7Write);
DECLFW(FDSSReg8Write);
DECLFW(FDSSReg9Write);
DECLFR(FDSEnvVolumeRead);
DECLFR(FDSEnvModRead);

void FDSSound_SC(void);
void FDSSound_Reset(void);
void FDSSoundRegReset(void);
void FDSSound_AddStateInfo(void);

/* For mappers utilizing FDS expansion audio */
void FDSSound_Power(void);
DECLFR(FDSSound_Read);  /* $4040-$4092 */
DECLFW(FDSSound_Write); /* $4040-$408A */

#endif /* _FDS_APU_H */
