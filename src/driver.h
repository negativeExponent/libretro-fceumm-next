#ifndef _FCEU_DRIVER_H
#define _FCEU_DRIVER_H

#include <libretro.h>

#ifdef  __cplusplus
extern "C" {
#endif

#include "fceu-types.h"
#include "git.h"
#include "debug.h"
#include "palette.h"

#define FCEUNPCMD_RESET       0x01
#define FCEUNPCMD_POWER       0x02

#define FCEUNPCMD_VSUNICOIN   0x07
#define FCEUNPCMD_VSUNICOIN2   0x20
#define FCEUNPCMD_VSUNISERVICE 0x21
#define FCEUNPCMD_VSUNIDIP0   0x08
#define FCEUNPCMD_FDSINSERTx  0x10
#define FCEUNPCMD_FDSINSERT   0x18
#define FCEUNPCMD_FDSEJECT    0x19
#define FCEUNPCMD_FDSSELECT   0x1A

#define FCEUNPCMD_LOADSTATE   0x80

#define FCEUNPCMD_SAVESTATE   0x81     /* Sent from server to client. */
#define FCEUNPCMD_LOADCHEATS  0x82
#define FCEUNPCMD_TEXT        0x90

/* This makes me feel dirty for some reason. */
#define FCEUI_printf FCEU_printf

/* Video interface */
void FCEUD_SetPalette(int index, uint8_t r, uint8_t g, uint8_t b);

/* Displays an error.  Can block or not. */
void FCEUD_PrintError(char *s);
void FCEUD_PrintDebug(char *s);
void FCEUD_Message(char *s);

void FCEUD_DispMessage(enum retro_log_level level, unsigned duration, const char *str);
void FCEU_DispMessage(enum retro_log_level level, unsigned duration, const char *format, ...);

void FCEUI_ResetNES(void);
void FCEUI_PowerNES(void);

void FCEUI_SetInput(int port, int type, void *ptr, int attrib);
void FCEUI_SetInputFC(int type, void *ptr, int attrib);
void FCEUI_SetInputFourScore(int useFourScore);

/* New interface functions */

/* 0 to keep 8-sprites limitation, 1 to remove it */
void FCEUI_DisableSpriteLimitation(int a);

/* 1 = show, 0 = hide */
void FCEUI_SetRenderPlanes(int sprites, int bg);
void FCEUI_GetRenderPlanes(int *sprites, int *bg);

/* Loads and identifies a ROM image.
 * Returns a valid FCEUGI* on success, or NULL on failure.
 */
FCEUGI *FCEUI_LoadROM(const char *name, const uint8_t *databuf, size_t databufsize);

/* Post load and prepares the emulator after a successful ROM load.
 * Resets video system, applies Game Genie (if enabled),
 * initializes cheat hooks, sets palettes, and powers on the system.
 * Returns nonzero on success, 0 on failure.
 */
int FCEUI_PostLoad(void);

/* allocates memory.  0 on failure, 1 on success. */
int FCEUI_Initialize(void);

/* Emulates a frame. */
void FCEUI_Emulate(uint8_t **, uint8_t **, int32_t **, int32_t *, int);

/* Closes currently loaded game */
void FCEUI_CloseGame(void);

/* Deallocates all allocated memory.  Call after FCEUI_Emulate() returns. */
void FCEUI_Kill(void);

/* Enable/Disable game genie. a=0 disable, a=1 enable */
void FCEUI_SetGameGenie(int a);

/* Set video system a=0 NTSC, a=1 PAL */
void FCEUI_SetVidSystem(int a);

/* Convenience function; returns currently emulated video system(0=NTSC, 1=PAL).  */
int FCEUI_GetCurrentVidSystem(int *slstart, int *slend);

#ifdef FRAMESKIP
/* Should be called from FCEUD_BlitScreen().  Specifies how many frames
   to skip until FCEUD_BlitScreen() is called.  FCEUD_BlitScreenDummy()
   will be called instead of FCEUD_BlitScreen() when when a frame is skipped.
*/
void FCEUI_FrameSkip(int x);
#endif

/* First and last scanlines to render, for ntsc and pal emulation. */
void FCEUI_SetRenderedLines(int ntscf, int ntscl, int palf, int pall);

/* Sets the base directory (bios and palette files are saved
   in this directory. */
void FCEUI_SetBaseDirectory(const char *dir);

/* Tells FCE Ultra to copy the palette data pointed to by pal and use it.
   Data pointed to by pal needs to be 64*3 or 512x3 bytes in length.
*/
void FCEU_SetPaletteUser(const pal *pal, const unsigned nEntries);

/* Sets default palette */
void FCEU_SetPalette(void);

/* Get VS Unisystem palette number */
int  FCEU_VSUniGetPaletteNum(void);

/* Sets up sound code to render sound at the specified rate, in samples
   per second.  Only sample rates of 44100, 48000, and 96000 are currently
   supported.
   If "Rate" equals 0, sound is disabled. */
void FCEUI_Sound(int Rate);
void FCEUI_SetSoundQuality(int quality);

/* Sets up volume on each sound channel.
   If "volume" equals 0, then sound for that channel is muted. Max volume is 256. */
void FCEUI_SetSoundVolume(int channel, int volume);

/* Gets the current volume of the specific audio channel. */
int FCEUI_GetSoundVolume(int channel);

int32_t FCEUI_GetDesiredFPS(void);

int FCEUI_DecodePAR(const char *code, uint16_t *a, uint8_t *v, int *c, int *type);
int FCEUI_DecodeGG(const char *str, uint16_t *a, uint8_t *v, int *c);
int FCEUI_AddCheat(const char *name, uint32_t addr, uint8_t val, int compare, int type);
int FCEUI_DelCheat(uint32_t which);
int FCEUI_ToggleCheat(uint32_t which);

int32_t FCEUI_CheatSearchGetCount(void);
void FCEUI_CheatSearchGetRange(uint32_t first, uint32_t last, int (*callb)(uint32_t a, uint8_t last, uint8_t current));
void FCEUI_CheatSearchGet(int (*callb)(uint32_t a, uint8_t last, uint8_t current, void *data), void *data);
void FCEUI_CheatSearchBegin(void);
void FCEUI_CheatSearchEnd(int type, uint8_t v1, uint8_t v2);
void FCEUI_ListCheats(int (*callb)(char *name, uint32_t a, uint8_t v, int compare, int s, int type, void *data), void *data);

int FCEUI_GetCheat(uint32_t which, char **name, uint32_t *a, uint8_t *v, int *compare, int *s, int *type);
int FCEUI_SetCheat(uint32_t which, const char *name, int32_t a, int32_t v, int compare, int s, int type);

void FCEUI_CheatSearchShowExcluded(void);
void FCEUI_CheatSearchSetCurrentAsOriginal(void);

#ifdef FCEUDEF_DEBUGGER
void FCEUI_MemDump(uint16_t a, int32_t len, void (*callb)(uint16_t a, uint8_t v));
uint8_t FCEUI_MemSafePeek(uint16_t A);
void FCEUI_MemPoke(uint16_t a, uint8_t v, int hl);
void FCEUI_NMI(void);
void FCEUI_IRQ(void);
uint16_t FCEUI_Disassemble(void *XA, uint16_t a, char *stringo);
void FCEUI_GetIVectors(uint16_t *reset, uint16_t *irq, uint16_t *nmi);
#endif

void FCEUI_SetLowPass(int q);

void FCEUI_RemoveTringleNoise(int d);
void FCEUI_ReduceDmcPopping(int d);
void FCEUI_ReverseDMCBitOrder(int d);

void FCEUI_NSFSetVis(int mode);
int FCEUI_NSFChange(int amount);
int FCEUI_NSFGetInfo(uint8_t *name, uint8_t *artist, uint8_t *copyright, int maxlen);

void FCEUI_VSUniToggleDIPView(void);
void FCEUI_VSUniToggleDIP(int w);
uint8_t FCEUI_VSUniGetDIPs(void);
void FCEUI_VSUniSetDIP(int w, int state);
void FCEUI_VSUniCoin(void);
void FCEUI_VSUniCoin2(void);
void FCEUI_VSUniService(void);

void FCEUI_FDSInsert(void);
void FCEUI_FDSEject(void);
void FCEUI_FDSSelect(void);

int FCEUI_DatachSet(uint8_t *rcode);

#ifdef  __cplusplus
}
#endif


#endif
