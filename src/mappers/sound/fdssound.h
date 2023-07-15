#ifndef _FDS_APU_H
#define _FDS_APU_H

#include "fceu.h"

DECLFR(FDSSound_Read);  /* $4040-$407F, $4090-$4092 */
DECLFW(FDSSound_Write); /* $4040-$407F, $4080-$408A */

void FDSSound_Power(void); 
void FDSSound_Reset(void);
void FDSSound_AddSaveState(void);

#endif /* _FDS_APU_H */
