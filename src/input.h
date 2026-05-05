#ifndef _FCEU_INPUT_H
#define _FCEU_INPUT_H

typedef struct {
	uint8_t (*Read)(int w);
	void (*Write)(uint8_t v);
	void (*Strobe)(int w);
	void (*Update)(int w, void *data, int arg);
	void (*SLHook)(int w, uint8_t *bg, uint8_t *spr, uint32_t linets, int final);
	void (*Draw)(int w, uint8_t *buf, int arg);
} INPUTC;

typedef struct {
	uint8_t (*Read)(int w, uint8_t ret);
	void (*Write)(uint8_t v);
	void (*Strobe)(void);
	void (*Update)(void *data, int arg);
	void (*SLHook)(uint8_t *bg, uint8_t *spr, uint32_t linets, int final);
	void (*Draw)(uint8_t *buf, int arg);
} INPUTCFC;

uint8_t FCEU_GetJoyJoy(void);

void FCEU_DrawInput(uint8_t *buf);
void FCEU_UpdateInput(void);
void FCEUINPUT_Power(void);

void InputScanlineHook(uint8_t *bg, uint8_t *spr, uint32_t linets, int final);

void FCEU_DoSimpleCommand(int cmd);

void FCEU_ZapperSetTolerance(int t);
void FCEU_ZapperSetSTMode(int mode);
void FCEU_ZapperInvertTrigger(int invert);
void FCEU_ZapperInvertSensor(int invert);

INPUTC *FCEU_InitZapper(int w);
INPUTC *FCEU_InitMouse(int w);
INPUTC *FCEU_InitPowerpadA(int w);
INPUTC *FCEU_InitPowerpadB(int w);
INPUTC *FCEU_InitArkanoid(int w);
INPUTC *FCEU_InitLCDCompZapper(int w);
INPUTC *FCEU_InitSNESMouse(int w);
INPUTC *FCEU_InitSNESGamepad(int w);
INPUTC *FCEU_InitVirtualBoy(int w);

INPUTCFC *FCEU_InitArkanoidFC(void);
INPUTCFC *FCEU_InitSpaceShadow(void);
INPUTCFC *FCEU_InitFKB(void);
INPUTCFC *FCEU_InitSuborKB(void);
INPUTCFC *FCEU_InitPEC586KB(void);
INPUTCFC *FCEU_InitHS(void);
INPUTCFC *FCEU_InitMahjong(void);
INPUTCFC *FCEU_InitPartyTap(void);
INPUTCFC *FCEU_InitFamilyTrainerA(void);
INPUTCFC *FCEU_InitFamilyTrainerB(void);
INPUTCFC *FCEU_InitOekaKids(void);
INPUTCFC *FCEU_InitTopRider(void);
INPUTCFC *FCEU_InitFamiNetSys(void);
INPUTCFC *FCEU_InitBarcodeWorld(void);
INPUTCFC *FCEU_InitExcitingBoxing(void);

#endif
