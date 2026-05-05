#ifndef _FCEUH
#define _FCEUH

#include "fceu-types.h"
#include "file.h"

#define RAM_SIZE 0x800
#define RAM_MASK (RAM_SIZE - 1)

#define NES_WIDTH  256
#define NES_HEIGHT 240
#define NTSC_WIDTH 602

extern int fceuindbg;
extern int newppu;
void ResetGameLoaded(void);

#define DECLFR(x) uint8_t x(uint16_t A)
#define DECLFW(x) void x(uint16_t A, uint8_t V)

void FCEU_MemoryRand(uint8_t *ptr, uint32_t size);
void SetReadHandler(uint16_t start, uint16_t end, readfunc func);
void SetWriteHandler(uint16_t start, uint16_t end, writefunc func);
writefunc GetWriteHandler(uint16_t a);
readfunc GetReadHandler(uint16_t a);

void FCEU_ResetVidSys(void);

void ResetMapping(void);
void ResetNES(void);
void PowerNES(void);

extern uint64_t timestampbase;
extern uint32_t MMC5HackVROMMask;
extern uint8_t *MMC5HackExNTARAMPtr;
extern uint8_t MMC5Hack;
extern uint8_t *MMC5HackVROMPTR;
extern uint8_t MMC5HackCHRMode;
extern uint8_t MMC5HackSPMode;
extern uint8_t MMC50x5130;
extern uint8_t MMC5HackSPScroll;
extern uint8_t MMC5HackSPPage;

extern uint8_t PEC586Hack;

extern uint8_t QTAIHack;
extern uint8_t qtaintramreg;
extern uint8_t QTAINTRAM[0x800];

extern uint8_t *RAM;

extern readfunc ARead[0x10000];
extern writefunc BWrite[0x10000];

extern void (*GameInterface)(int h);
extern void (*GameStateRestore)(int version);

#define GI_RESETM2 1
#define GI_POWER   2
#define GI_CLOSE   3

#include "git.h"
extern FCEUGI *GameInfo;

extern uint8_t isPAL;
extern uint8_t isDendy;

#include "driver.h"

enum __VRC7_TONE {
	TONE_AUTO,
	TONE_2413,
	TONE_VRC7,
	TONE_281B
};

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

	int SwapDutyCycles;
	int RamInitState;
	int ShowCrosshair;
	int ReplaceP2StartWithMicrophone;
	int PPUOverclockEnabled;
	int SkipDMC7BitOverclock;
	int RemoveTriangleNoise;
	int ReduceDMCPopping;
	int ReverseDMCBitOrder;
	int VRC7ToneType; /*0: mapper dependenr, 1: 2413 2: vrcc7 3: 281B */
	int DisableEmphasis;
} FCEUS;

extern FCEUS FSettings;

void FCEU_PrintError(char *format, ...); /* warning level messages */
void FCEU_PrintDebug(char *format, ...); /* debug level messages */
void FCEU_printf(char *format, ...);     /* normal messages */
void FCEU_TogglePPU(void);

void SetNESDeemph_OldHacky(uint8_t d, int force);
void DrawTextTrans(uint8_t *dest, uint32_t width, uint8_t *textmsg, uint8_t fgcolor);
void FCEU_PutImage(void);
#ifdef FRAMESKIP
void FCEU_PutImageDummy(void);
#endif

#define JOY_A      0x01
#define JOY_B      0x02
#define JOY_SELECT 0x04
#define JOY_START  0x08
#define JOY_UP     0x10
#define JOY_DOWN   0x20
#define JOY_LEFT   0x40
#define JOY_RIGHT  0x80

int UNIFLoad(const char *name, FCEUFILE *fp);
int iNESLoad(const char *name, FCEUFILE *fp);
int FDSLoad(const char *name, FCEUFILE *fp);
int NSFLoad(FCEUFILE *fp);

#endif
