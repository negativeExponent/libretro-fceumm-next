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

#ifndef _FCEU_NSF_H
#define _FCEU_NSF_H

#define	NSFSOUND_VRC6	0x01
#define	NSFSOUND_VRC7	0x02
#define	NSFSOUND_FDS	0x04
#define	NSFSOUND_MMC5	0x08
#define	NSFSOUND_N163	0x10
#define	NSFSOUND_S5B	0x20

typedef struct NSF_HEADER {
	char ID[5];				/* NESM^Z */
	uint8_t Version;
	uint8_t TotalSongs;
	uint8_t StartingSong;
	uint8_t LoadAddressLow;
	uint8_t LoadAddressHigh;
	uint8_t InitAddressLow;
	uint8_t InitAddressHigh;
	uint8_t PlayAddressLow;
	uint8_t PlayAddressHigh;
	uint8_t GameName[32];
	uint8_t Artist[32];
	uint8_t Copyright[32];
	uint8_t NTSCspeed[2];		/* Unused */
	uint8_t BankSwitch[8];
	uint8_t PALspeed[2];		/* Unused */
	uint8_t VideoSystem;
	uint8_t SoundChip;
	uint8_t Expansion[4];
	uint8_t reserve[8];
} NSF_HEADER;

typedef struct NSFINFO {
	char SongName[256];
	char Artist[256];
	char Copyright[256];
	char Dumper[256];
	char SongNames[100][256];

	uint8_t TotalSongs;
	uint8_t StartingSong;
	uint8_t CurrentSong;
	uint8_t VideoSystem;

	uint16_t PlayAddr, InitAddr, LoadAddr;
	uint8_t BankSwitch[8];
	uint8_t SoundChip;

	uint8_t *NSFDATA;
	size_t NSFMaxBank;
	size_t NSFSize;
} NSFINFO;

extern NSFINFO *NSFInfo;

void NSF_init(void);
void DrawNSF(uint8_t *target);
void DoNSFFrame(void);

/* NSF Expansion Chip Set Write Handler for mappers */
void NFSSetWriteHandler(int chip, int32_t start, int32_t end, writefunc func);

#endif
