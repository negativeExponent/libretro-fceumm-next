#ifndef _FCEUH
#define _FCEUH

#include "fceu-types.h"

#define RAM_SIZE 0x800
#define RAM_MASK (RAM_SIZE - 1)

#define NES_WIDTH 256
#define NES_HEIGHT 240

extern int fceuindbg;
extern unsigned DMC_7bit;
/* Region selection */

/* Audio mods*/
extern unsigned swapDuty; /* Swap bits 6 & 7 of $4000/$4004 to mimic bug
                           * found on some famiclones/Dendy models.
                           */

void ResetGameLoaded(void);

#define DECLFR(x) uint8 x(uint32 A)
#define DECLFW(x) void x(uint32 A, uint8 V)

void FCEU_MemoryRand(uint8 *ptr, uint32 size);
void SetReadHandler(int32 start, int32 end, readfunc func);
void SetWriteHandler(int32 start, int32 end, writefunc func);
writefunc GetWriteHandler(int32 a);
readfunc GetReadHandler(int32 a);

int AllocGenieRW(void);
void FlushGenieRW(void);

void FCEU_ResetVidSys(void);

void ResetMapping(void);
void ResetNES(void);
void PowerNES(void);


extern uint64 timestampbase;
extern uint32 MMC5HackVROMMask;
extern uint8 *MMC5HackExNTARAMPtr;
extern int MMC5Hack;
extern uint8 *MMC5HackVROMPTR;
extern uint8 MMC5HackCHRMode;
extern uint8 MMC5HackSPMode;
extern uint8 MMC50x5130;
extern uint8 MMC5HackSPScroll;
extern uint8 MMC5HackSPPage;

extern int PEC586Hack;

extern int QTAIHack;
extern uint8 qtaintramreg;
extern uint8 QTAINTRAM[0x800];

extern uint8 RAM[RAM_SIZE];

extern readfunc ARead[0x10000 + 0x200];
extern writefunc BWrite[0x10000 + 0x200];

extern void (*GameInterface)(int h);
extern void (*GameStateRestore)(int version);

#define GI_RESETM2  1
#define GI_POWER  2
#define GI_CLOSE  3

#include "git.h"
extern FCEUGI *GameInfo;

extern uint8 isPAL;
extern uint8 isDendy;

#include "driver.h"

typedef struct {
	int PAL;
	
	int volume[12]; /* master, nes apu and expansion audio */
	
	int GameGenie;

	/* Current first and last rendered scanlines. */
	int FirstSLine;
	int LastSLine;

	/* Driver code(user)-specified first and last rendered scanlines.
	 * Usr*SLine[0] is for NTSC, Usr*SLine[1] is for PAL.
	 */
	int UsrFirstSLine[2];
	int UsrLastSLine[2];

	int SndRate;
	int soundq;
	int lowpass;
} FCEUS;

extern FCEUS FSettings;

void FCEU_PrintError(char *format, ...); /* warning level messages */
void FCEU_PrintDebug(char *format, ...); /* debug level messages */
void FCEU_printf(char *format, ...); /* normal messages */

void SetNESDeemph(uint8 d, int force);
void DrawTextTrans(uint8 *dest, uint32 width, uint8 *textmsg, uint8 fgcolor);
void FCEU_PutImage(void);
#ifdef FRAMESKIP
void FCEU_PutImageDummy(void);
#endif

#define JOY_A        0x01
#define JOY_B        0x02
#define JOY_SELECT   0x04
#define JOY_START    0x08
#define JOY_UP       0x10
#define JOY_DOWN     0x20
#define JOY_LEFT     0x40
#define JOY_RIGHT    0x80

#endif
