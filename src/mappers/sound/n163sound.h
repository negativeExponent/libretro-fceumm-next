#ifndef _N163_SOUND_H
#define _N163_SOUND_H

#include "mapinc.h"

uint8 *GetIRAM_ptr(void); /* pointer to internal RAM */
uint32 GetIRAM_size(void);

DECLFR(N163Sound_Read);
DECLFW(N163Sound_Write);

void N163Sound_Reset(void);
void N163Sound_FixCache(void);

void N163Sound_ESI(void);

#endif /*  _N163_SOUND_H */
