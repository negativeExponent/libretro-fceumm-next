/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2023
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

#include "vrc7sound.h"

static int32 dwave = 0;
static uint8 vrc7idx;

static OPLL *VRC7Sound = NULL;

void DoVRC7Sound(void) {
	int32 z, a;
	if (FSettings.soundq >= 1) {
		return;
    }
	z = ((SOUNDTS << 16) / soundtsinc) >> 4;
	a = z - dwave;
	OPLL_fillbuf(VRC7Sound, &Wave[dwave], a, 1);
	dwave += a;
}

static void UpdateOPLNEO(int32 *Wave, int Count) {
	OPLL_fillbuf(VRC7Sound, Wave, Count, 4);
}

static void UpdateOPL(int Count) {
	int32 z, a;
	z = ((SOUNDTS << 16) / soundtsinc) >> 4;
	a = z - dwave;
	if (VRC7Sound && a) {
		OPLL_fillbuf(VRC7Sound, &Wave[dwave], a, 1);
    }
	dwave = 0;
}

static void VRC7SC(void) {
	if (VRC7Sound) {
		OPLL_set_rate(VRC7Sound, FSettings.SndRate);
    }
}

static void VRC7SKill(void) {
	if (VRC7Sound) {
		OPLL_delete(VRC7Sound);
    }
	VRC7Sound = NULL;
}

DECLFW(VRC7SW) {
    switch (A & 0xF030) {
    case 0x9010:
        vrc7idx = V;
        break;
	case 0x9030:
		if (FSettings.SndRate) {
			OPLL_writeReg(VRC7Sound, vrc7idx, V);
			GameExpSound.Fill = UpdateOPL;
			GameExpSound.NeoFill = UpdateOPLNEO;
		}
		break;
	}
}

void VRC7SoundRestore(void) {
    OPLL_forceRefresh(VRC7Sound);
}

void VRC7_ESI(void) {
	GameExpSound.RChange = VRC7SC;
	GameExpSound.Kill = VRC7SKill;
	VRC7Sound = OPLL_new(3579545, FSettings.SndRate ? FSettings.SndRate : 44100);

	/* Sound states */
    AddExState(&vrc7idx, sizeof(vrc7idx), 0, "VRCI");
	AddExState(&VRC7Sound->adr, sizeof(VRC7Sound->adr), 0, "ADDR");
	AddExState(&VRC7Sound->out, sizeof(VRC7Sound->out), 0, "OUT0");
	AddExState(&VRC7Sound->realstep, sizeof(VRC7Sound->realstep), 0, "RTIM");
	AddExState(&VRC7Sound->oplltime, sizeof(VRC7Sound->oplltime), 0, "TIME");
	AddExState(&VRC7Sound->opllstep, sizeof(VRC7Sound->opllstep), 0, "STEP");
	AddExState(&VRC7Sound->prev, sizeof(VRC7Sound->prev), 0, "PREV");
	AddExState(&VRC7Sound->next, sizeof(VRC7Sound->next), 0, "NEXT");
	AddExState(&VRC7Sound->LowFreq, sizeof(VRC7Sound->LowFreq), 0, "LFQ0");
	AddExState(&VRC7Sound->HiFreq, sizeof(VRC7Sound->HiFreq), 0, "HFQ0");
	AddExState(&VRC7Sound->InstVol, sizeof(VRC7Sound->InstVol), 0, "VOLI");
	AddExState(&VRC7Sound->CustInst, sizeof(VRC7Sound->CustInst), 0, "CUSI");
	AddExState(&VRC7Sound->slot_on_flag, sizeof(VRC7Sound->slot_on_flag), 0, "FLAG");
	AddExState(&VRC7Sound->pm_phase, sizeof(VRC7Sound->pm_phase), 0, "PMPH");
	AddExState(&VRC7Sound->lfo_pm, sizeof(VRC7Sound->lfo_pm), 0, "PLFO");
	AddExState(&VRC7Sound->am_phase, sizeof(VRC7Sound->am_phase), 0, "AMPH");
	AddExState(&VRC7Sound->lfo_am, sizeof(VRC7Sound->lfo_am), 0, "ALFO");
	AddExState(&VRC7Sound->patch_number, sizeof(VRC7Sound->patch_number), 0, "PNUM");
	AddExState(&VRC7Sound->key_status, sizeof(VRC7Sound->key_status), 0, "KET");
	AddExState(&VRC7Sound->mask, sizeof(VRC7Sound->mask), 0, "MASK");
    AddExState(&VRC7Sound->slot[0], sizeof(VRC7Sound->slot[0]), 0, "SL00");
    AddExState(&VRC7Sound->slot[1], sizeof(VRC7Sound->slot[1]), 0, "SL01");
    AddExState(&VRC7Sound->slot[2], sizeof(VRC7Sound->slot[2]), 0, "SL02");
    AddExState(&VRC7Sound->slot[3], sizeof(VRC7Sound->slot[3]), 0, "SL03");
    AddExState(&VRC7Sound->slot[4], sizeof(VRC7Sound->slot[4]), 0, "SL04");
    AddExState(&VRC7Sound->slot[5], sizeof(VRC7Sound->slot[5]), 0, "SL05");
    AddExState(&VRC7Sound->slot[6], sizeof(VRC7Sound->slot[6]), 0, "SL06");
    AddExState(&VRC7Sound->slot[7], sizeof(VRC7Sound->slot[7]), 0, "SL07");
    AddExState(&VRC7Sound->slot[8], sizeof(VRC7Sound->slot[8]), 0, "SL08");
    AddExState(&VRC7Sound->slot[9], sizeof(VRC7Sound->slot[9]), 0, "SL09");
    AddExState(&VRC7Sound->slot[10], sizeof(VRC7Sound->slot[10]), 0, "SL10");
    AddExState(&VRC7Sound->slot[11], sizeof(VRC7Sound->slot[11]), 0, "SL11");
}

/* VRC7 Sound */
