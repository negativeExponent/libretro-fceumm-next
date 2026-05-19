/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2026 negativeExponent
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 */

#ifndef _CARTRAM_H
#define _CARTRAM_H

extern uint32_t CHRRAMSRAM;
extern uint32_t WRAMSIZE;
void CartRAM_Init(CartInfo *info,
                  uint8_t min_wram_kb,
                  uint8_t min_chrram_kb);
void CartRAM_Close(void);
void CHRRAM_Init(CartInfo *info, uint8_t min_chrram_kb);
void WRAM_Init(CartInfo *info, uint8_t min_wram_kb);

#endif
