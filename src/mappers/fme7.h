
typedef struct __FME7 {
	uint8 prg[4];
	uint8 chr[8];
	uint8 cmd;
	uint8 mirr;
} FME7;

extern FME7 fme7;

DECLFW(FME7_WriteIndex);
DECLFW(FME7_WriteReg);

void FME7_Init(CartInfo *info, int wram, int battery);
void FME7_Power(void);
void FME7_Reset(void);

extern void (*FME7_FixPRG)(void);
extern void (*FME7_FixCHR)(void);
extern void (*FME7_FixMIR)(void);
extern void (*FME7_FixWRAM)(void);

extern void (*FME7_pwrap)(uint32 A, uint8 V);
extern void (*FME7_cwrap)(uint32 A, uint8 V);
