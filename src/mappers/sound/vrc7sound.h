#ifndef _VRC7_SOUND_H
#define _VRC7_SOUND_H

#include "mapinc.h"
#include "emu2413.h"

DECLFW(VRC7Sound_Write);

void VRC7Sound_StateRestore(void);
void VRC7Sound_ESI(void);

#endif /* _VRC7_SOUND_H */
