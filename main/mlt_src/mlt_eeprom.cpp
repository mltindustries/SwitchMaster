

#include <Arduino.h>
#include <EEPROM.h>

#include "mlt_defs.h"

#include "mlt_config.h"
#include "mlt_calibration.h"

#include "mlt_eeprom.h"


//
// Initialize EEPROM routines
//
void eeprom_init(void) {
    EEPROM.begin(EEPROM_SIZE); //Initialasing EEPROM
    delay(10);
}

//
// Check if the EEPROM status
// return true if blank
bool eeprom_isblank(void) {
    return (EEPROM.read(EEPROM_CONFIG_ADDRESS) == 0xFF);
}


void eeprom_display_values(void) {
    Serial.println("EEPROM values");

    config_load();
    calibration_load();

    Serial.print("SSID: ");
    Serial.println((char*)config_new.ssid);
    Serial.print("PASS: ");
    Serial.println((char*)config_new.pass);
    Serial.print("PIN: ");
    Serial.println(config_new.pin);
    Serial.print("LOWCUT: ");
    Serial.println(config_new.lowcut);
    Serial.printf("\n");
    Serial.print("Demo activated? 1. True 0.False : ");
    Serial.println(demo_mode_activated);
    Serial.print("ADC Deactivated? 1. True 0.False : ");
    Serial.println(adc_activated);

    Serial.printf("Calibration gain  : %3.6f\r\n", calibration_data.calibration_gain);
    Serial.printf("Calibration offset: %3.6f\r\n\r\n", calibration_data.calibration_offset);
}
