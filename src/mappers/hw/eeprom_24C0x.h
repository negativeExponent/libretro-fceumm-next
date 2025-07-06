#ifndef _EEPROM24C0X_H
#define _EEPROM24C0X_H

typedef enum Model {
    EEPROM_24C01 = 1,
    EEPROM_24C02 = 2
} Model;

typedef enum Mode {
    MODE_IDLE,
    MODE_DATA,
    MODE_ADDRESS,
    MODE_READ,
    MODE_WRITE,
    MODE_ACK,
    MODE_NOT_ACK,
    MODE_ACK_WAIT,
    MODE_MAX
} Mode;

typedef struct X24C0X {
    Model model;

    struct {
        uint8 scl;
        uint8 sda;
    } line;

    Mode mode;
    Mode next;

    struct {
        uint8 bit;
        uint8 address;
        uint8 data;
    } latch;

    uint8 rw;
    uint8 output;

    uint8 *mem;
} X24C0X;

void eeprom_24C01_init(X24C0X *e, uint8 *_rom);
void eeprom_24C02_init(X24C0X *e, uint8 *_rom);

void eeprom_init(X24C0X *e, uint8 model, uint8 *_data);
void eeprom_AddStateInfo(X24C0X *e);
void eeprom_i2c_step(X24C0X *e, uint8 scl, uint8 sda_in);
uint8 eeprom_read(X24C0X *e);

#endif
