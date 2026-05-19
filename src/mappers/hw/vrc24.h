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

#ifndef _VRC24_H
#define _VRC24_H

enum {
	VRC2a = 1, /* Mapper 22 */
	VRC2b,     /* Mapper 23 */
	VRC2c,     /* Mapper 25 */
	VRC4a,     /* Mapper 21 */
	VRC4b,     /* Mapper 25 */
	VRC4c,     /* Mapper 21 */
	VRC4d,     /* Mapper 25 */
	VRC4e,     /* Mapper 23 */
	VRC4f,     /* Mapper 23 */
	VRC4_544,
	VRC4_559
};

typedef enum __VRC24TYPE {
	VRC24_VRC2 = 0,
	VRC24_VRC4 = 1
} VRC24TYPE;

typedef struct __VRC24 {
	uint8_t prg[2];
	uint16_t chr[8];
	uint8_t cmd;
	uint8_t mirr;
	uint8_t wire; /* VRC2 $6000-$6FFF microwire interface */

	/* not normally added to state */
	uint8_t type;
	uint16_t A0;
	uint16_t A1;
} VRC24;

extern VRC24 vrc24;

uint16_t VRC24_GetPRGBank(int bank);
uint16_t VRC24_GetCHRBank(int bank);

void VRC24_SyncPRG_default(void);
void VRC24_SyncCHR_default(void);
void VRC24_SyncMirror_default(void);

DECLFR(VRC24_ReadWRAM);
DECLFW(VRC24_WriteWRAM);
DECLFW(VRC24_Write);

void VRC24_IRQCPUHook(int a);
void VRC24_Reset(void);
void VRC24_Power(void);
void VRC24_Close(void);

void VRC24_Init(CartInfo *info, VRC24TYPE vrc4, uint32_t A0, uint32_t A1, int wram, int irqRepeated);
void VRC2_SetConfig(uint8_t clear, uint32_t _A0, uint32_t _A1);
void VRC4_SetConfig(uint8_t clear, uint32_t _A0, uint32_t _A1, int irqRepeated);

extern void (*VRC24_SyncPRG)(void);
extern void (*VRC24_SyncCHR)(void);
extern void (*VRC24_SyncMirror)(void);

/* VRC2 mircrowire interface when wram is not present $6000-$7FFF*/
/* callback function on writes */
extern void (*VRC24_SyncWires)(void);

extern void (*VRC24_pwrap)(uint16_t A, uint16_t V);
extern void (*VRC24_cwrap)(uint16_t A, uint16_t V);

/* VRC4 External Select, e.g. $9000 port 3 */
extern DECLFW((*VRC24_WriteExtSelect));

#endif /* _VRC24_H */
