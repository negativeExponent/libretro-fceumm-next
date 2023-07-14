#ifndef _FDS_APU_H
#define _FDS_APU_H

#include "fceu.h"

void FDSSoundPower(void); 
void FDSSoundReset(void);
void FDSSoundStateAdd(void);

DECLFR(FDSSoundRead);  /* $4040-$407F, $4090-$4092 */
DECLFW(FDSSoundWrite); /* $4040-$407F, $4080-$408A */

#endif /* _FDS_APU_H */
