

// total size reserved for EEPROM
#define EEPROM_SIZE   1024

// EEPROM addresses
#define EEPROM_CALIBRATION_ADDRESS      0
#define EEPROM_CONFIG_ADDRESS           32


//
// Initialize EEPROM routines
//
void eeprom_init(void);

//
// Check if the EEPROM status
// return true if blank
bool eeprom_isblank(void);



void eeprom_display_values(void);
