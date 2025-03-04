/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2002 Xodnizel
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

/* Various macros for faster memory stuff
	(at least that's the idea)
*/

#ifndef _FCEU_MEMORY_H_
#define _FCEU_MEMORY_H_

#include "fceu-types.h"

/**
 * @brief Sets memory in blocks of 4 bytes (32-bit) to a specified 32-bit value.
 *
 * This macro fills a memory block, `d`, with the value `c`, in chunks of 4 bytes at a time.
 * It assumes that the memory block size `n` is a multiple of 4 for optimal performance.
 *
 * The macro performs a loop that starts from the end of the memory block and sets every
 * 4-byte block to the specified value `c`.
 *
 * @param d Pointer to the start of the memory block to fill.
 * @param c The 32-bit value to fill the memory with.
 * @param n The total number of bytes to fill. The memory is filled from the end to the start,
 *          and this should generally be a multiple of 4 for efficiency.
 *
 * @note This macro assumes that the memory block is aligned to 4-byte boundaries.
 *       It does not handle cases where `n` is not a multiple of 4 or where alignment is
 *       not guaranteed.
 */
#define FCEU_dwmemset32(d, c, n) { int _x; for (_x = n - 4; _x >= 0; _x -= 4) *(uint32*)& (d)[_x] = c; }

/* returns an aligned buffer  */
void *FCEU_amalloc(uint32 size);

/* returns a buffer initialized to 0 */
void *FCEU_malloc(uint32 size);

/* returns a buffer with initialization based on FCEU_MemoryRand() */
/* Used by mappers for wram, chr ram, etc */
void *FCEU_gmalloc(uint32 size);

/* free memory allocated by FCEU_amalloc */
void FCEU_afree(void *ptr);

/* free memory allocated by FCEU_gmalloc */
void FCEU_gfree(void *ptr);

/* free memory allocated by FCEU_malloc */
void FCEU_free(void *ptr);

#endif
