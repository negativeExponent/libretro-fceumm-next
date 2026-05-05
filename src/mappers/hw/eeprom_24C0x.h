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
        uint8_t scl;
        uint8_t sda;
    } line;

    Mode mode;
    Mode next;

    struct {
        uint8_t bit;
        uint8_t address;
        uint8_t data;
    } latch;

    uint8_t rw;
    uint8_t output;

    uint8_t *mem;
} X24C0X;

void eeprom_24C01_init(X24C0X *e, uint8_t *_rom);
void eeprom_24C02_init(X24C0X *e, uint8_t *_rom);

void eeprom_init(X24C0X *e, uint8_t model, uint8_t *_data);
void eeprom_AddStateInfo(X24C0X *e);
void eeprom_i2c_step(X24C0X *e, uint8_t scl, uint8_t sda_in);
uint8_t eeprom_read(X24C0X *e);

#endif
