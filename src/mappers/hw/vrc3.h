/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2025-2026 negativeExponent
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

#ifndef _VRC3_H
#define _VRC3_H

 typedef struct __VRC3 {
	uint8_t prg;
	uint8_t IRQx; /* autoenable */
	uint8_t IRQm; /* mode */
	uint8_t IRQa;
	uint16_t IRQLatch, IRQCount;
} VRC3;

void VRC3_SetPRG_default(uint16_t A, uint16_t V);
void VRC3_SetCHR_default(uint16_t V);
void VRC3_SyncPRG_default(void);
void VRC3_SyncCHR_default(void);

DECLFW(VRC3_WriteReg);

void VRC3_CPUIRQHook(int a);
void VRC3_Reset(void);
void VRC3_Power(void);
void VRC3_StateRestore(int version);
void VRC3_Init(CartInfo *info);

void VRC3_SetConfig(uint8_t clear);

extern VRC3 vrc3;

extern void (*VRC3_SyncPRG)(void);
extern void (*VRC3_SyncCHR)(void);

extern void (*VRC3_pwrap)(uint16_t A, uint16_t V);
extern void (*VRC3_cwrap)(uint16_t V);

#endif /* _VRC3_H */
