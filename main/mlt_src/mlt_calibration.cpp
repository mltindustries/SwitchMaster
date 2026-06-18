

#include <Arduino.h>
#include <EEPROM.h>
#include "mlt_eeprom.h"
#include <Preferences.h>

#include "mlt_defs.h"

#include "mlt_adc.h"

#include "mlt_calibration.h"

#include "mlt_led.h"
#include "mlt_nvs.h"

calibration_t calibration_data;  // configuration data

#define CALIBRATION_MAGIC_NUMBER      ( 0xDEAFABE )

/**
 * @brief USE_ESP32_NVS defines where to store configuration files
 * value of 1 will store in ESP32 NVS memory
 * value of 0 will store in external EEPROM
*/
#define USE_ESP32_NVS           ( 1 )

void calibration_init(void) {
#if (USE_ESP32_NVS == 1)
  // Try to load (read) calibration data from ESP32 NVS storage.
  if ( true == mlt_nvs_read_calibration_data( &calibration_data ) ) {
    // Calibration data found, so check if its valid or corrupted
    if ( calibration_data.magic != CALIBRATION_MAGIC_NUMBER ) {
      Serial.println( "Calibration data is corrupted" );
      calibration_restore_defaults();
    }
  }
  else {
    // If no data found, start with default value
    Serial.println( "No calibration data found" );
    calibration_restore_defaults();
  }
#else
    eeprom_init();

    if (eeprom_isblank()) {
        Serial.println("EEPROM is blank.");
        calibration_restore_defaults();
    } else {
        calibration_load();
    }
#endif
}


void calibration_restore_defaults(void) {
    Serial.println("Restoring Default Calibration Settings.");
    calibration_data.calibration_gain = 1;                // TODO: this is board dependent
    calibration_data.calibration_offset = 0;              // TODO: this is board dependent
    calibration_data.magic = CALIBRATION_MAGIC_NUMBER;    // magic marker
    // write structure to EEPROM
    calibration_save();
}

//
// load stored config
//
void calibration_load(void) {
#if (USE_ESP32_NVS == 1)
    mlt_nvs_read_calibration_data( &calibration_data );
#else
    // read structure from EEPROM
    EEPROM.get(EEPROM_CALIBRATION_ADDRESS, calibration_data);
#endif
    if ( calibration_data.magic != CALIBRATION_MAGIC_NUMBER ) {
        Serial.println("\r\n\r\n **** WARNING! Calibration data is lost. Defaults applied. ***\r\n\r\n");
        calibration_restore_defaults();
    }
}

//
// save config
//
void calibration_save(void) {
#if (USE_ESP32_NVS == 1)
  mlt_nvs_write_calibration_data( &calibration_data );
#else
    // write structure to EEPROM
    EEPROM.put(EEPROM_CALIBRATION_ADDRESS, calibration_data);
    EEPROM.commit();
#endif
}


void calibration_run(void) {
    led_set();
    calibration_restore_defaults();
    Serial.printf("Calibration gain  : %3.6f\r\n", calibration_data.calibration_gain);
    Serial.printf("Calibration offset: %3.6f\r\n", calibration_data.calibration_offset);
    int calibration_mode = 1;
    float xl = 6.0;      // known reference voltage low
    float xh = 16.0;     // known reference voltage high
    float yl, yh;        // ADC measurement low, high

      // need to get two calibration points
      // 6.0V and 16.0V

      while (1) {
          switch (calibration_mode) {
            case 1:
              Serial.println("Adjust the input voltage to 6.0V, then press the BOOT button.");
              calibration_mode = 2;
              break;
            case 2:
              if (digitalRead(0) == 0) {                        // button is down
                yl = adc_get_voltage() ;                 
                while (digitalRead(0) == 0);                    // wait for button release
                Serial.printf("Measured %3.2f V\r\n",yl);
                delay(50);
                calibration_mode = 3;                          // next state
              }
              break;
            case 3:
              Serial.println("Adjust the input voltage to 16.0V, then press the BOOT button.");
              calibration_mode = 4;
              break;
            case 4:
              if (digitalRead(0) == 0) {                        // button is down
                yh = adc_get_voltage();                 
                while (digitalRead(0) == 0);                    // wait for button release
                Serial.printf("Measured %3.2f V\r\n",yh);
                delay(50);
                calibration_mode = 5;                          // next state
              }
              break;
            case 5:
              Serial.println("Calibration complete.");

              calibration_data.calibration_gain = (xh-xl) / (yh-yl);
              calibration_data.calibration_offset = (yl * calibration_data.calibration_gain) - xl;

              Serial.printf("Calibration gain  : %3.6f\r\n", calibration_data.calibration_gain);
              Serial.printf("Calibration offset: %3.6f\r\n", calibration_data.calibration_offset);
              calibration_mode = 0;
              calibration_save();
              Serial.print("Voltage: ");
              Serial.println(adc_get_voltage());
              Serial.print("Current: ");
              Serial.println(adc_get_current());
              led_clear();
              return;
          }
      }
}
