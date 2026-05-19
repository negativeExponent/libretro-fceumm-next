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
 */

#ifndef _ONEBUS_H
#define _ONEBUS_H

typedef struct __ONEBUS {
	/* General Purpose Registers */
	uint8_t cpu41xx[0x0100], ppu20xx[0x100], apu40xx[0x40];
	uint32_t relative_8k;

	/* IRQ Registers */
	uint8_t IRQCount, IRQa, IRQReload;

	/* APU Registers */
	uint8_t pcm_enable, pcm_irq;
	int16_t pcm_addr, pcm_size, pcm_latch, pcm_clock;

	struct {
		uint32_t size;
		uint8_t *data;
		uint8_t *low;
		uint8_t *high;
		uint8_t *low16;
		uint8_t *high16;
	} chr;
} ONEBUS;

extern ONEBUS onebus;

DECLFR(OneBus_ReadAPU40XX); /* APU Read  $4000 - $403F */
DECLFR(OneBus_ReadCPU41XX); /* CPU Read $4100 - $4FFF */

DECLFW(OneBus_WritePPU20XX); /* PPU Write $2010 - $20FF */
DECLFW(OneBus_WriteAPU40XX); /* APU Write $4000 - $403F */
DECLFW(OneBus_WriteCPU41XX); /* CPU Write $4100 - $41FF */
DECLFW(OneBus_WriteMMC3);

void OneBus_Power(void);
void OneBus_Reset(void);
void OneBus_Init(CartInfo *info, void (*proc)(void), int wram, int battery);

void OneBus_SyncPRG(uint16_t mmask, uint16_t mblock);
void OneBus_SyncCHR(uint16_t mmask, uint16_t mblock);
void OneBus_SyncMirror(void);
void OneBus_SyncPRG16(uint16_t bank0, uint16_t bank1, uint16_t mmask, uint16_t mblock);

void OneBus_SetCHR(uint8_t **banks, uint8_t *base, uint8_t bit4pp, uint8_t extended, uint16_t EVA, uint16_t mmask, uint16_t mblock);

#endif /* _ONEBUS_H */
