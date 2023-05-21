#ifndef _EEPROM_93C66_H
#define _EEPROM_93C66_H

#include "mapinc.h"

void eeprom_93C66_init   (uint8 *data, uint32 capacity, uint8 wordsize);
uint8 eeprom_93C66_read  (void);
void  eeprom_93C66_write (uint8 CS, uint8 CLK, uint8 DAT);
#endif
