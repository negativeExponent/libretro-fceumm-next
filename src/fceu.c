/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2003 Xodnizel
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

#include  <stdio.h>
#include  <string.h>
#include  <stdlib.h>
#include  <stdarg.h>
#include  <time.h>

#include  "fceu.h"
#include  "fceu-types.h"
#include  "x6502.h"
#include  "ppu.h"
#include  "sound.h"
#include  "general.h"
#include  "fceu-endian.h"
#include  "fceu-memory.h"

#include  "cart.h"
#include  "nsf.h"
#include  "fds.h"
#include  "ines.h"
#include  "unif.h"
#include  "cheat.h"
#include  "palette.h"
#include  "state.h"
#include  "video.h"
#include  "input.h"
#include  "file.h"
#include  "crc32.h"
#include  "vsuni.h"

uint64 timestampbase;

FCEUGI *GameInfo = NULL;
void (*GameInterface)(int h);

void (*GameStateRestore)(int version);

readfunc ARead[0x10000 + 0x200];
writefunc BWrite[0x10000 + 0x200];
static readfunc *AReadG = NULL;
static writefunc *BWriteG = NULL;
static int RWWrap = 0;

static DECLFW(BNull)
{
}

static DECLFR(ANull)
{
	return(cpu.openbus);
}

int AllocGenieRW(void)
{
   if (!AReadG)
   {
      if (!(AReadG = (readfunc*)FCEU_malloc(0x8000 * sizeof(readfunc))))
         return 0;
   }
   else
      memset(AReadG, 0, 0x8000 * sizeof(readfunc));

   if (!BWriteG)
   {
      if (!(BWriteG = (writefunc*)FCEU_malloc(0x8000 * sizeof(writefunc))))
         return 0;
   }
   else
      memset(BWriteG, 0, 0x8000 * sizeof(writefunc));

   RWWrap = 1;
   return 1;
}

void FlushGenieRW(void)
{
   int32 x;

   if (RWWrap)
   {
      for (x = 0; x < 0x8000; x++)
      {
         ARead[x + 0x8000] = AReadG[x];
         BWrite[x + 0x8000] = BWriteG[x];
      }
      free(AReadG);
      free(BWriteG);
      AReadG = NULL;
      BWriteG = NULL;
   }
   RWWrap = 0;
}

readfunc GetReadHandler(int32 a)
{
	if (a >= 0x8000 && RWWrap)
		return AReadG[a - 0x8000];
	else
		return ARead[a];
}

void SetReadHandler(int32 start, int32 end, readfunc func)
{
	int32 x;

	if (!func)
		func = ANull;

	if (RWWrap)
		for (x = end; x >= start; x--)
      {
         if (x >= 0x8000)
            AReadG[x - 0x8000] = func;
         else
            ARead[x] = func;
      }
	else
		for (x = end; x >= start; x--)
			ARead[x] = func;
}

writefunc GetWriteHandler(int32 a)
{
	if (RWWrap && a >= 0x8000)
		return BWriteG[a - 0x8000];
	else
		return BWrite[a];
}

void SetWriteHandler(int32 start, int32 end, writefunc func)
{
	int32 x;

	if (!func)
		func = BNull;

	if (RWWrap)
		for (x = end; x >= start; x--)
      {
			if (x >= 0x8000)
				BWriteG[x - 0x8000] = func;
			else
				BWrite[x] = func;
		}
	else
		for (x = end; x >= start; x--)
			BWrite[x] = func;
}

uint8 RAM[RAM_SIZE];

uint8 isPAL = FALSE;
uint8 isDendy = FALSE;

static int AllocBuffers(void) {
	return 1;
}

static void FreeBuffers(void) {
}

static DECLFW(BRAML)
{
	RAM[A] = V;
}

static DECLFR(ARAML)
{
	return RAM[A];
}

static DECLFW(BRAMH)
{
	RAM[A & RAM_MASK] = V;
}

static DECLFR(ARAMH)
{
	return RAM[A & RAM_MASK];
}

static DECLFW(BOverflow)
{
	A &= 0xFFFF;
	BWrite[A](A, V);
}

static DECLFR(AOverflow)
{
	A &= 0xFFFF;
	cpu.PC &= 0xFFFF;
	return ARead[A](A);
}

void FCEUI_CloseGame(void)
{
	if (!GameInfo)
      return;

   if (GameInfo->name)
      free(GameInfo->name);
   GameInfo->name = 0;
   if (GameInfo->type != GIT_NSF)
      FCEU_FlushGameCheats();
   GameInterface(GI_CLOSE);
   ResetExState(0, 0);
   FCEU_CloseGenie();
   free(GameInfo);
   GameInfo = 0;
}

void ResetGameLoaded(void)
{
	if (GameInfo)
      FCEUI_CloseGame();

	GameStateRestore = NULL;
	PPU_hook = NULL;
	GameHBIRQHook = NULL;

	if (GameExpSound.Kill)
		GameExpSound.Kill();
	memset(&GameExpSound, 0, sizeof(GameExpSound));

	MapIRQHook = NULL;
	MMC5Hack = 0;
	PEC586Hack = 0;
	isPAL &= 1;
	palette_nes_selected = PAL_NES_DEFAULT;
}

int UNIFLoad(const char *name, FCEUFILE *fp);
int iNESLoad(const char *name, FCEUFILE *fp);
int FDSLoad(const char *name, FCEUFILE *fp);
int NSFLoad(FCEUFILE *fp);

FCEUGI *FCEUI_LoadGame(const char *name, const uint8_t *databuf, size_t databufsize,
      frontend_post_load_init_cb_t frontend_post_load_init_cb)
{
   FCEUFILE *fp;

   ResetGameLoaded();

   GameInfo = malloc(sizeof(FCEUGI));
   memset(GameInfo, 0, sizeof(FCEUGI));

   GameInfo->soundchan = 0;
   GameInfo->soundrate = 0;
   GameInfo->name = 0;
   GameInfo->type = GIT_CART;
   GameInfo->vidsys = GIV_USER;
   GameInfo->input[0] = GameInfo->input[1] = -1;
   GameInfo->inputfc = -1;
   GameInfo->cspecial = 0;

   fp = FCEU_fopen(name, databuf, databufsize);

   if (!fp) {
      FCEU_PrintError("Error opening \"%s\"!", name);

      free(GameInfo);
      GameInfo = NULL;

      return NULL;
   }

   if (iNESLoad(name, fp))
      goto endlseq;
   if (NSFLoad(fp))
      goto endlseq;
   if (UNIFLoad(NULL, fp))
      goto endlseq;
   if (FDSLoad(NULL, fp))
      goto endlseq;

   FCEU_PrintError("An error occurred while loading the file.\n");
   FCEU_fclose(fp);

   if (GameInfo->name)
      free(GameInfo->name);
   GameInfo->name = NULL;
   free(GameInfo);
   GameInfo = NULL;

   return NULL;

endlseq:
   FCEU_fclose(fp);

   if (frontend_post_load_init_cb)
      (*frontend_post_load_init_cb)();

   FCEU_ResetVidSys();
   if (GameInfo->type != GIT_NSF)
      if (FSettings.GameGenie)
         FCEU_OpenGenie();

   PowerNES();

   if (GameInfo->type != GIT_NSF) {
      FCEU_LoadGamePalette();
      FCEU_LoadGameCheats();
   }

   FCEU_ResetPalette();

   return(GameInfo);
}

int FCEUI_Initialize(void) {
	srand(time(0));

	if (!AllocBuffers())
		return 0;

	if (!FCEU_InitVirtualVideo())
		return 0;

	memset(&FSettings, 0, sizeof(FSettings));

	FSettings.UsrFirstSLine[0] = 8;
	FSettings.UsrFirstSLine[1] = 0;
	FSettings.UsrLastSLine[0] = 231;
	FSettings.UsrLastSLine[1] = 239;

	FSettings.volume[APU_MASTER] = 256;
	FSettings.volume[APU_SQUARE1] = 256;
	FSettings.volume[APU_SQUARE2] = 256;
	FSettings.volume[APU_TRIANGLE] = 256;
	FSettings.volume[APU_NOISE] = 256;
	FSettings.volume[APU_DPCM] = 256;
	FSettings.volume[APU_FDS] = 256;
	FSettings.volume[APU_MMC5] = 256;
	FSettings.volume[APU_N163] = 256;
	FSettings.volume[APU_S5B] = 256;
	FSettings.volume[APU_VRC6] = 256;
	FSettings.volume[APU_VRC7] = 256;

	FCEUPPU_Init();
	X6502_Init();

	return 1;
}

void FCEUI_Kill(void) {
	FCEU_KillVirtualVideo();
	FCEU_KillGenie();
	FreeBuffers();
}

void FCEUI_Emulate(uint8 **pXBuf, uint8 **pXDBuf, int32 **SoundBuf, int32 *SoundBufSize, int skip) {
	int ssize;

	FCEU_UpdateInput();
	if (geniestage != 1) FCEU_ApplyPeriodicCheats();
	FCEUPPU_Loop(skip);

	ssize = FlushEmulateSound();

	timestampbase += timestamp;

	timestamp = 0;
	sound_timestamp = 0;

	*pXBuf = skip ? 0 : XBuf;
	*pXDBuf = skip ? 0 : XDBuf;

	*SoundBuf = WaveFinal;
	*SoundBufSize = ssize;
}


void ResetNES(void)
{
	if (!GameInfo)
      return;
	GameInterface(GI_RESETM2);
	FCEUSND_Reset();
	FCEUPPU_Reset();
	X6502_Reset();
}

int ram_init_seed = 0;
int option_ramstate = 0;

uint64 splitmix64(uint32 input) {
	uint64 z = (input + 0x9e3779b97f4a7c15);
	z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9;
	z = (z ^ (z >> 27)) * 0x94d049bb133111eb;
	return z ^ (z >> 31);
}

static INLINE uint64 xoroshiro128plus_rotl(const uint64 x, int k) {
	return (x << k) | (x >> (64 - k));
}

uint64 xoroshiro128plus_s[2];
void xoroshiro128plus_seed(uint32 input)
{
/* http://xoroshiro.di.unimi.it/splitmix64.c */
	uint64 x = input;

	uint64 z = (x += 0x9e3779b97f4a7c15);
	z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9;
	z = (z ^ (z >> 27)) * 0x94d049bb133111eb;
	xoroshiro128plus_s[0] = z ^ (z >> 31);
	
	z = (x += 0x9e3779b97f4a7c15);
	z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9;
	z = (z ^ (z >> 27)) * 0x94d049bb133111eb;
	xoroshiro128plus_s[1] = z ^ (z >> 31);
}

/* http://vigna.di.unimi.it/xorshift/xoroshiro128plus.c */
uint64 xoroshiro128plus_next(void) {
	const uint64 s0 = xoroshiro128plus_s[0];
	uint64 s1 = xoroshiro128plus_s[1];
	const uint64 result = s0 + s1;

	s1 ^= s0;
	xoroshiro128plus_s[0] = xoroshiro128plus_rotl(s0, 55) ^ s1 ^ (s1 << 14); /* a, b */
	xoroshiro128plus_s[1] = xoroshiro128plus_rotl(s1, 36); /* c */

	return result;
}

void FCEU_MemoryRand(uint8 *ptr, uint32 size)
{
	int x = 0;

	if (!ptr || !size)
		return;

	while (size) {
		uint8 v = 0;
		switch (option_ramstate)
		{
		case 0: v = 0xff; break;
		case 1: v = 0x00; break;
		case 2: v = (uint8)(xoroshiro128plus_next()); break;
		}
		*ptr = v;
		x++;
		size--;
		ptr++;
	}
}

void hand(X6502 *X, int type, uint32 A)
{
}

void PowerNES(void)
{
	if (!GameInfo)
      return;
	
	/* reseed random, unless we're in a movie */
	ram_init_seed = rand() ^ (uint32)xoroshiro128plus_next();
	/* always reseed the PRNG with the current seed, for deterministic results (for that seed) */
	xoroshiro128plus_seed(ram_init_seed);

	FCEU_CheatResetRAM();
	FCEU_CheatAddRAM(2, 0, RAM);

	FCEU_GeniePower();

	FCEU_MemoryRand(RAM, RAM_SIZE);

	SetReadHandler(0x0000, 0xFFFF, ANull);
	SetWriteHandler(0x0000, 0xFFFF, BNull);

	SetReadHandler(0x10000, 0x10000 + 0x200, AOverflow);
	SetWriteHandler(0x10000, 0x10000 + 0x200, BOverflow);

	SetReadHandler(0, 0x7FF, ARAML);
	SetWriteHandler(0, 0x7FF, BRAML);

	SetReadHandler(0x800, 0x1FFF, ARAMH);	/* Part of a little */
	SetWriteHandler(0x800, 0x1FFF, BRAMH);	/* hack for a small speed boost. */

	InitializeInput();
	FCEUSND_Power();
	FCEUPPU_Power();

	/* Have the external game hardware "powered" after the internal NES stuff.
		Needed for the NSF code and VS System code.
	*/
	GameInterface(GI_POWER);
	if (GameInfo->type == GIT_VSUNI)
		FCEU_VSUniPower();

	timestampbase = 0;
	X6502_Power();
	FCEU_PowerCheats();
}

void FCEU_ResetVidSys(void)
{
	int w;

	if (GameInfo->vidsys == GIV_NTSC)
		w = 0;
	else if (GameInfo->vidsys == GIV_PAL)
   {
      w = 1;
      isDendy = FALSE;
   }
	else
		w = FSettings.PAL;

	isPAL = w ? TRUE : FALSE;

   if (isPAL)
      isDendy = FALSE;

   ppu.normal_scanlines = isDendy ? 290 : 240;
   ppu.totalscanlines   = ppu.normal_scanlines;
   if (ppu.overclock_enabled)
      ppu.totalscanlines += ppu.extrascanlines;

	FCEUPPU_SetVideoSystem(w || isDendy);
	SetSoundVariables();
}

FCEUS FSettings;

void FCEU_DispMessage(enum retro_log_level level, unsigned duration, const char *format, ...)
{
   static char msg[512] = {0};
   va_list ap;

   if (!format || (*format == '\0'))
      return;

   va_start(ap, format);
   vsprintf(msg, format, ap);
   va_end(ap);

   FCEUD_DispMessage(level, duration, msg);
}

void FCEU_printf(char *format, ...)
{
	char temp[2048];

	va_list ap;

	va_start(ap, format);
	vsprintf(temp, format, ap);
	FCEUD_Message(temp);

	va_end(ap);
}

void FCEU_PrintDebug(char *format, ...)
{
	char temp[2048];

	va_list ap;

	va_start(ap, format);
	vsprintf(temp, format, ap);
	FCEUD_PrintDebug(temp);

	va_end(ap);
}

void FCEU_PrintError(char *format, ...)
{
	char temp[2048];

	va_list ap;

	va_start(ap, format);
	vsprintf(temp, format, ap);
	FCEUD_PrintError(temp);

	va_end(ap);
}

void FCEUI_SetRenderedLines(int ntscf, int ntscl, int palf, int pall)
{
	FSettings.UsrFirstSLine[0] = ntscf;
	FSettings.UsrLastSLine[0] = ntscl;
	FSettings.UsrFirstSLine[1] = palf;
	FSettings.UsrLastSLine[1] = pall;
	if (isPAL || isDendy)
   {
		FSettings.FirstSLine = FSettings.UsrFirstSLine[1];
		FSettings.LastSLine = FSettings.UsrLastSLine[1];
	}
   else
   {
		FSettings.FirstSLine = FSettings.UsrFirstSLine[0];
		FSettings.LastSLine = FSettings.UsrLastSLine[0];
	}
}

void FCEUI_SetVidSystem(int a)
{
	FSettings.PAL = a ? 1 : 0;

	if (!GameInfo)
      return;

   FCEU_ResetVidSys();
   FCEU_ResetPalette();
}

int FCEUI_GetCurrentVidSystem(int *slstart, int *slend)
{
	if (slstart)
		*slstart = FSettings.FirstSLine;
	if (slend)
		*slend = FSettings.LastSLine;
	return(isPAL);
}

void FCEUI_SetGameGenie(int a)
{
	FSettings.GameGenie = a ? 1 : 0;
}

int32 FCEUI_GetDesiredFPS(void)
{
	if (isPAL || isDendy)
		return (838977920); /* ~50.007 */
	return (1008307711);    /* ~60.1 */
}
