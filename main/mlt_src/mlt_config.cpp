

#include <Arduino.h>
#include <EEPROM.h>
#include "mlt_eeprom.h"
#include <Preferences.h> //NH

#include "mlt_defs.h"
#include "mlt_config.h"
#include "mlt_nvs.h"

config_new_t config_new;  // configuration data

#define CONFIG_MAGIC_NUMBER         ( 0xDEAFABE )

/**
 * @brief USE_ESP32_NVS defines where to store configuration files
 * value of 1 will store in ESP32 NVS memory
 * value of 0 will store in external EEPROM
*/
#define USE_ESP32_NVS               ( 1 )

void config_init(void) {
#if (USE_ESP32_NVS == 1)
     // Try to load (read) config data from ESP32 NVS storage.
  if ( true == mlt_nvs_read_config_data( &config_new ) ) {
    // Config data found, so check if its valid or corrupted
    if ( config_new.magic != CONFIG_MAGIC_NUMBER ) {
      Serial.println( "Config data is corrupted" );
      config_restore_defaults();
    }
  }
  else {    
    // If no data found, start with default value
    Serial.println( "No config data found" );
    config_restore_defaults();
  }

  mlt_nvs_read_demo_mode( &demo_mode_activated );
  mlt_nvs_read_adc_mode( &adc_activated );

  
#else
    eeprom_init();

    if (eeprom_isblank()) {
        Serial.println("EEPROM is blank.");
        config_restore_defaults();
    } else {
        config_load();
    }
#endif
}

void config_restore_defaults(void) {
    Serial.println("Restoring Default Configuration Settings.");
    memset((void *) &config_new, 0, sizeof(config_new));  // clear structure
    // as the struct was zeroed, lowcut, ch_cutout and ch_timeout is all zero
    // setup the remaining members
    strcpy((char *) config_new.ssid, DEFAULT_SSID);
    strcpy((char *) config_new.pass, DEFAULT_PASS);
    config_new.pin = DEFAULT_PIN;
    config_new.lowcut = 11500;
    config_new.calibration_gain = 1;            // board dependent
    config_new.calibration_offset = 0;          // board dependent
    config_new.magic = CONFIG_MAGIC_NUMBER;     // magic marker
#if (USE_ESP32_NVS == 1)
    mlt_nvs_write_demo_mode( false );
#else
    EEPROM.put(80,0); // Set demo to no
#endif
    // write structure to EEPROM
    config_save();
}

//
// load stored config
//
void config_load(void) {
#if (USE_ESP32_NVS == 1)
    mlt_nvs_read_config_data( &config_new );
    mlt_nvs_read_demo_mode( &demo_mode_activated );
    mlt_nvs_read_adc_mode( &adc_activated );
#else
    // read structure from EEPROM
    EEPROM.get(EEPROM_CONFIG_ADDRESS, config_new);
    EEPROM.get(80, demo_mode_activated);
    EEPROM.get(81, adc_activated);
#endif
    if (config_new.magic != 0xDEAFABE) {
        Serial.println("Configuration corrupted.");
        config_restore_defaults();
    }
}

//
// save config
//
void config_save(void) {
#if (USE_ESP32_NVS == 1)
    mlt_nvs_write_config_data( &config_new );
#else
    // write structure to EEPROM
    EEPROM.put(EEPROM_CONFIG_ADDRESS, config_new);
    EEPROM.commit();
#endif
}

//
// Start Demo Mode
//
void config_demo_on(void) {
    // write structure to EEPROM
    Serial.println("Demo mode ON");
#if (USE_ESP32_NVS == 1)
    mlt_nvs_write_demo_mode( true );
#else
    EEPROM.put(80,1);
    EEPROM.commit();
#endif
}

// Stop Demo Mode
//
void config_demo_off(void) {
    // write structure to EEPROM
    Serial.println("Demo mode off");
#if (USE_ESP32_NVS == 1)
    mlt_nvs_write_demo_mode( false );
#else
    EEPROM.put(80,0);
    EEPROM.commit();
#endif
}

//
// Stop ADC
//
void config_adc_deactivate(void) {
    // write structure to EEPROM
    Serial.println("Setting ADC to off. RESTART TO ACTION.");
#if (USE_ESP32_NVS == 1)
    mlt_nvs_write_adc_mode( false );
#else
    EEPROM.put(81,0);
    EEPROM.commit();
#endif
}

//
// Start ADC
//
void config_adc_activate(void) {
    // write structure to EEPROM
    Serial.println("Setting ADC to on. RESTART TO ACTION.");
#if (USE_ESP32_NVS == 1)
    mlt_nvs_write_adc_mode( true );
#else
    EEPROM.put(81,1);
    EEPROM.commit();
#endif
}
