#ifndef _EEPROM_X24C0X_H
#define _EEPROM_X24C0X_H

typedef enum {
	EEPROM_NONE = 0,
	EEPROM_X24C01,
	EEPROM_X24C02
} EEPROM_TYPE;

extern uint8 x24c0x_data[512];

void x24c01_init(void);
void x24c01_write(uint8 data);
uint8 x24c01_read(void);
extern SFORMAT x24c01_StateRegs[9];

void x24c02_init(void);
void x24c02_write(uint8 data);
uint8 x24c02_read(void);
extern SFORMAT x24c02_StateRegs[9];

#endif /* _EEPROM_X24C0X_H */
