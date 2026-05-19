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

#ifndef _VRC7_H
#define _VRC7_H

typedef struct __VRC7 {
	uint8_t prg[4];
	uint8_t chr[8];
	uint8_t mirr;
} VRC7;

extern VRC7 vrc7;

DECLFW(VRC7_Write);

void VRC7_Power(void);
void VRC7_Close(void);

void VRC7_Init(CartInfo *info, uint32_t A0);

void VRC7_SetConfig(uint8_t clear, int A0);

void VRC7_SyncPRG(void);
void VRC7_SyncCHR(void);

extern void (*VRC7_pwrap)(uint16_t A, uint16_t V);
extern void (*VRC7_cwrap)(uint16_t A, uint16_t V);
extern void (*VRC7_mwrap)(uint8_t V);

#endif /* _VRC7_H */
