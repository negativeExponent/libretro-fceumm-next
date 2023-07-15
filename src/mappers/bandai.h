#ifndef _BANDAI_H
#define _BANDAI_H

#include "eeprom_x24c0x.h"

DECLFW(BANDAI_Write);

void BANDAI_Power(void);
void BANDAI_IRQHook(int a);
void BANDAI_Reset(void);

void BANDAI_Init(CartInfo *info, EEPROM_TYPE _eeprom_type, int _isFCG);

void BANDAI_FixPRG(void);
void BANDAI_FixCHR(void);
void BANDAI_FixMIR(void);

extern void (*BANDAI_pwrap)(uint32 A, uint8 V);
extern void (*BANDAI_cwrap)(uint32 A, uint8 V);

#endif /* _BANDAI_H */
