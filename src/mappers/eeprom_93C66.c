#include "eeprom_93C66.h"

static uint8* eeprom_93C66_storage;
static uint8  eeprom_93C66_opcode;
static uint16 eeprom_93C66_data;
static uint16 eeprom_93C66_address;
static uint8  eeprom_93C66_state;
static uint8  eeprom_93C66_lastCLK;
static uint8  eeprom_93C66_writeEnabled;
static uint8  eeprom_93C66_output;
static uint8  eeprom_savesize;
static uint8  eeprom_wordlength;
static uint8  state_address;
static uint8  state_data;

#define OPCODE_MISC         0
#define OPCODE_WRITE        1
#define OPCODE_READ         2
#define OPCODE_ERASE        3
#define OPCODE_WRITEDISABLE 10
#define OPCODE_WRITEALL     11
#define OPCODE_ERASEALL     12
#define OPCODE_WRITEENABLE  13

#define STATE_STANDBY   0
#define STATE_STARTBIT  1
#define STATE_OPCODE    3
#define STATE_ADDRESS8  12
#define STATE_DATA8     20
#define STATE_ADDRESS16 11
#define STATE_DATA16    27
#define STATE_FINISHED  99

void eeprom_93C66_init(uint8 *data, uint32 savesize, uint8 wordlength)
{
   eeprom_93C66_storage      =data;
   eeprom_93C66_address      =0;
   eeprom_93C66_state        =STATE_STANDBY;
   eeprom_93C66_lastCLK      =0;
   eeprom_93C66_writeEnabled =0;
   eeprom_savesize           =savesize;
   eeprom_wordlength         =wordlength;

   state_address = (wordlength == 16) ? STATE_ADDRESS16 : STATE_ADDRESS8;
   state_data    = (wordlength == 16) ? STATE_DATA16 : STATE_DATA8;
}

uint8 eeprom_93C66_read (void)
{
   return eeprom_93C66_output;
}

void eeprom_93C66_write (uint8 CS, uint8 CLK, uint8 DAT)
{
   if (!CS && eeprom_93C66_state <= state_address)
      eeprom_93C66_state =STATE_STANDBY;
   else
   if (eeprom_93C66_state ==STATE_STANDBY && CS  && CLK)
   {
      eeprom_93C66_state =STATE_STARTBIT;
      eeprom_93C66_opcode  =0;
      eeprom_93C66_address =0;
      eeprom_93C66_output  =1;
   }
   else
   if (CLK && !eeprom_93C66_lastCLK)
   {
      if (eeprom_93C66_state >=STATE_STARTBIT && eeprom_93C66_state <STATE_OPCODE) 
         eeprom_93C66_opcode  =(eeprom_93C66_opcode  <<1) | (DAT? 1: 0);
      else
      if (eeprom_93C66_state >=STATE_OPCODE   && eeprom_93C66_state <state_address)
         eeprom_93C66_address =(eeprom_93C66_address <<1) | (DAT? 1: 0);
      else
      if (eeprom_93C66_state >=state_address  && eeprom_93C66_state <state_data)
      {
         if (eeprom_93C66_opcode ==OPCODE_WRITE || eeprom_93C66_opcode ==OPCODE_WRITEALL)
            eeprom_93C66_data =(eeprom_93C66_data    <<1) | (DAT? 1: 0);
         else
         if (eeprom_93C66_opcode ==OPCODE_READ)
         {
            if (eeprom_wordlength == 16)
               eeprom_93C66_output =!!(eeprom_93C66_data &0x8000);
            else
               eeprom_93C66_output =!!(eeprom_93C66_data &0x80);
            eeprom_93C66_data   =   eeprom_93C66_data <<1;
         }
      }
      eeprom_93C66_state++;
      if (eeprom_93C66_state ==state_address) {
         switch (eeprom_93C66_opcode)
         {
            case OPCODE_MISC:
               if (eeprom_wordlength == 16)
                  eeprom_93C66_opcode =(eeprom_93C66_address >>6) + 10;
               else
                  eeprom_93C66_opcode =(eeprom_93C66_address >>7) + 10;
               switch (eeprom_93C66_opcode)
               {
                  case OPCODE_WRITEDISABLE:
                     eeprom_93C66_writeEnabled = 0;
                     eeprom_93C66_state = STATE_FINISHED;
                     break;
                  case OPCODE_WRITEENABLE: 
                     eeprom_93C66_writeEnabled = 1;
                     eeprom_93C66_state = STATE_FINISHED;
                     break;
                  case OPCODE_ERASEALL:
                     if (eeprom_93C66_writeEnabled)
                     {
                        int i;
                        for (i =0; i <eeprom_savesize; i++)
                           eeprom_93C66_storage[i] = 0xFF;
                     }
                     eeprom_93C66_state = STATE_FINISHED;
                     break;
                  case OPCODE_WRITEALL:
                     eeprom_93C66_address = 0;
                     break;
               }
               break;
            case OPCODE_ERASE:
               if (eeprom_93C66_writeEnabled)
               {
                  if (eeprom_wordlength == 16)
                  {
                     eeprom_93C66_storage[eeprom_93C66_address << 1 | 0] = 0xFF;
                     eeprom_93C66_storage[eeprom_93C66_address << 1 | 1] = 0xFF;
                  }
                  else
                     eeprom_93C66_storage[eeprom_93C66_address] = 0xFF;
               }
               eeprom_93C66_state = STATE_FINISHED;
               break;
            case OPCODE_READ:
               if (eeprom_wordlength == 16)
               {
                  eeprom_93C66_data  = eeprom_93C66_storage[eeprom_93C66_address << 1 | 0];
                  eeprom_93C66_data |= (eeprom_93C66_storage[eeprom_93C66_address << 1 | 1] << 8);
                  eeprom_93C66_address++;
               }
               else
                  eeprom_93C66_data = eeprom_93C66_storage[eeprom_93C66_address++];
               break;
         }
      }
      else
      if (eeprom_93C66_state ==state_data)
      {
         if (eeprom_93C66_opcode ==OPCODE_WRITE)
	 {
            if (eeprom_wordlength == 16)
            {
               eeprom_93C66_storage[eeprom_93C66_address << 1 | 0] = eeprom_93C66_data & 0xFF;
               eeprom_93C66_storage[eeprom_93C66_address << 1 | 1] = eeprom_93C66_data >> 8;
               eeprom_93C66_address++;
            }
            else
               eeprom_93C66_storage[eeprom_93C66_address++] = eeprom_93C66_data;
            eeprom_93C66_state = STATE_FINISHED;
         }
	 else
         if (eeprom_93C66_opcode ==OPCODE_WRITEALL)
	 {
            if (eeprom_wordlength == 16)
            {
               eeprom_93C66_storage[eeprom_93C66_address << 1 | 0] = eeprom_93C66_data & 0xFF;
               eeprom_93C66_storage[eeprom_93C66_address << 1 | 1] = eeprom_93C66_data >> 8;
               eeprom_93C66_address++;
            }
            else
               eeprom_93C66_storage[eeprom_93C66_address++] = eeprom_93C66_data;
            eeprom_93C66_state = CS && eeprom_93C66_address <eeprom_savesize? state_address: STATE_FINISHED;
         }
	 else
         if (eeprom_93C66_opcode ==OPCODE_READ)
	 {
            if (eeprom_93C66_address <eeprom_savesize)
            {
               if (eeprom_wordlength == 16)
                  eeprom_93C66_data = eeprom_93C66_storage[eeprom_93C66_address << 1 | 0] | (eeprom_93C66_storage[eeprom_93C66_address << 1 | 1] << 8);
               else
                  eeprom_93C66_data = eeprom_93C66_storage[eeprom_93C66_address];
            }
            eeprom_93C66_state = CS && ++eeprom_93C66_address <=eeprom_savesize? state_address: STATE_FINISHED;
         }
      }
      if (eeprom_93C66_state == STATE_FINISHED)
      {
         eeprom_93C66_output = 0;
         eeprom_93C66_state = STATE_STANDBY;
      }
   }
   eeprom_93C66_lastCLK = CLK;
}
