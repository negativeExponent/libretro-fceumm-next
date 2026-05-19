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

#ifndef _VRC6_H
#define _VRC6_H

typedef struct __VRC6 {
	uint8_t prg[2];
	uint8_t chr[8];
	uint8_t mirr;
	uint16_t A0;
	uint16_t A1;
} VRC6;

extern VRC6 vrc6;

DECLFW(VRC6_Write);

void VRC6_Power(void);
void VRC6_Reset(void);
void VRC6_Close(void);
void VRC6_Restore(int version);
void VRC6_IRQCPUHook(int a);

void VRC6_Init(CartInfo *info, uint32_t A0, uint32_t A1, int wram);

void VRC6_SetConfig(uint8_t clear, int A0, int A1);

void VRC6_SyncPRG(void);
void VRC6_SyncCHR(void);
void VRC6_SyncMirror(void);

extern void (*VRC6_pwrap)(uint16_t A, uint16_t V);
extern void (*VRC6_cwrap)(uint16_t A, uint16_t V);

#endif /* _VRC6_H */
