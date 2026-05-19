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
 *
 */

/*
 * NES 2.0 Mapper 517 is used for 까치와 노래친구 (Kkachi-wa Nolae Chingu), a
 * Korean Karaoke game. It uses banking similar to UNROM without bus conflicts,
 * but has additional ADC hardware (different from NES 2.0 Mapper 515) for its
 * custom microphone socket at $6000 and $6001.
 */

#include "mapinc.h"
#include "latch.h"

static struct {
	int32_t adc_data;
	int32_t adc_high;
	int32_t adc_low;
	uint8_t adc_state;
} m517;

static SFORMAT StateRegs[] = {
	{ &m517.adc_data, sizeof(m517.adc_data) | FCEUSTATE_RLSB, "DATA" },
	{ &m517.adc_high, sizeof(m517.adc_high) | FCEUSTATE_RLSB, "DTHI" },
	{ &m517.adc_low, sizeof(m517.adc_low) | FCEUSTATE_RLSB, "DTLO" },
	{ &m517.adc_state, sizeof(m517.adc_state), "STAT" },
	{ 0 }
};

static void Sync(void) {
	setprg16(0x8000, latch.data);
	setprg16(0xC000, ~0);
	setchr8(0);
}

static DECLFR(Read6000) {
	uint8_t result = 0;
	if (A == 0x6000) {
		switch (m517.adc_state) {
		case 0:
			m517.adc_state = 1;
			result = 0;
			break;
		case 1:
			m517.adc_state = 2;
			result = 1;
			break;
		case 2:
			if (m517.adc_low > 0) {
				m517.adc_low--;
				result = 1;
			} else {
				m517.adc_state = 0;
				result = 0;
			}
			break;
		}
	} else {
		result = m517.adc_high-- > 0 ? 0 : 1;
	}
	return result;
}

static DECLFW(Write8000) {
	/* TODO: implement mic input from frontend */
	/* m517.adc_data = MIC * 63.0; */
	m517.adc_data = 0.0 * 63.0;
	m517.adc_high = m517.adc_data >> 2;
	m517.adc_low = 0x40 - m517.adc_high - ((m517.adc_data & 0x03) << 2);
	m517.adc_state = 0;
	Latch_Write(A, V);
}

static void Reset(void) {
	m517.adc_data = 0;
	m517.adc_state = 0;
	Sync();
}

static void Power(void) {
	memset(&m517, 0, sizeof(m517));
	Latch_Power();
	SetReadHandler(0x6000, 0x6FFF, Read6000);
	SetWriteHandler(0x8000, 0x8FFF, Write8000);
}

void Mapper517_Init(CartInfo *info) {
	Latch_Init(info, Sync, NULL, FALSE, FALSE);
	info->Power = Power;
	info->Reset = Reset;
	AddExState(StateRegs, ~0, 0, NULL);
}
