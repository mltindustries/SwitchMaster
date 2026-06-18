
// Platform includes
#include <Arduino.h>
#include <Preferences.h>

// Application includes
#include "mlt_defs.h"
#include "mlt_nvs.h"

#define TEMP_SENSOR_ID_LEN          ( 8 )

// list of undervoltage cutout level, in mV for a channel. 0 means ignore
extern uint16_t  ch_cutout[MAX_CHANNELS];

// list of cut-in level per switch, in mV, to auto-turn on a channel. 0 means ignore.
extern uint16_t  ch_cutin[MAX_CHANNELS];

static Preferences PreferenceNvsObj;
const char *pcNamespace_SwConfig = "switch_config";
const char *pcSwKeyPrefix = "sw";

const char *pcNamespace_mlt = "mlt_ns";
const char *pcTempSwKeyPrefix = "tsw";
const char *pcHwdVersionKey = "hw_ver";
const char *pcVendorKey = "vendor_no";
const char *pcCalibKey = "calib";
const char *pcConfigKey = "config";
const char *pcDemoKey = "demo";
const char *pcAdcKey = "adc";
const char *pcTempSensorIdKey = "TempSenId";
const char *pcIsIdWrittenKey = "IdWritten";
const char *pcOnboardTempCutoutKey = "obTempCutout";

void mlt_copy_nvs_to_ram( void ) {
    // Copy switch cutin and cutout threshold values into RAM buffer
    nimble_sw_data_t xSwConfigData;
    for ( uint8_t u8SwIdx = 0; u8SwIdx < MAX_CHANNELS; u8SwIdx++ ) {
        (void) memset( (void *)&xSwConfigData, 0x00, sizeof( nimble_sw_data_t ) );
        if ( true == mlt_nvs_read_switch_config( u8SwIdx, &xSwConfigData ) ) {
            ch_cutin[u8SwIdx] = xSwConfigData.voltage_cutin;
            ch_cutout[u8SwIdx] = xSwConfigData.voltage_cutout;
        }
    }
}

bool mlt_nvs_write_switch_config( nimble_sw_data_t *pxSwitchConfigWr ) {
    bool xRet = true;
    bool xIsOpened = false;

    if ( pxSwitchConfigWr->switch_number >= MAX_CHANNELS ) {
        // Serial.printf( "Undefined switch number: %d\n", pxSwitchConfigWr->switch_number );
        xRet = false;
    }

    // Open NVS namespace for switch config data
    if ( xRet == true ) {
        xRet = PreferenceNvsObj.begin( pcNamespace_SwConfig );
        if ( true == xRet ) {
            xIsOpened = true;
        }
        else {
            // Serial.println( "Failed to open swtich config NVS" );
        }
    }

    if ( true == xRet ) {
        char nvs_key[6] = { 0 };
        // for each switch we will have NVS key like sw_0, sw_1, ... sw_9
        snprintf( nvs_key, 6, "%s_%d", pcSwKeyPrefix, pxSwitchConfigWr->switch_number );
        size_t xWriteLen = sizeof( nimble_sw_data_t );
        size_t xWrittenBytes = 0;
        // Write swich config data in NVS
        xWrittenBytes = PreferenceNvsObj.putBytes( nvs_key, (const void *)pxSwitchConfigWr, xWriteLen );
        if ( xWrittenBytes != xWriteLen ) {
            // Serial.printf( "Failed to write switch (%d) config data in NVS\n", pxSwitchConfigWr->switch_number );
            xRet = false;
        }
    }

    // Close NVS namespace for switch config data
    if ( true == xIsOpened ) {
        PreferenceNvsObj.end();
    }

    return (xRet);
}

bool mlt_nvs_read_switch_config( uint8_t u8SwitchNum, nimble_sw_data_t *pxSwitchConfigRd ) {
    bool xRet = true;
    bool xIsOpened = false;

    // We should have a max 10 switches ( 0 to 9 )
    if ( u8SwitchNum >= MAX_CHANNELS ) {
        // Serial.printf( "Undefined switch number: %d\n", u8SwitchNum );
        xRet = false;
    }

    // Open NVS namespace for switch config data
    if ( xRet == true ) {
        xRet = PreferenceNvsObj.begin( pcNamespace_SwConfig );
        if ( true == xRet ) {
            xIsOpened = true;
        }
        else {
            // Serial.println( "Failed to open swtich config NVS" );
        }
    }

    if ( true == xRet ) {
        char nvs_key[6] = { 0 };
        // for each switch we will have NVS key like sw_0, sw_1, ... sw_9
        snprintf( nvs_key, 6, "%s_%d", pcSwKeyPrefix, u8SwitchNum );
        // Check if there is configuration already written with the key
        if ( 0 != PreferenceNvsObj.getBytesLength( nvs_key ) ) {
            size_t xReadLen = sizeof( nimble_sw_data_t );
            size_t xReadBytes = 0;
            // Read the switch config data from NVS
            xReadBytes = PreferenceNvsObj.getBytes( nvs_key, (void *)pxSwitchConfigRd, xReadLen );
            if ( xReadBytes != xReadLen ) {
                //Serial.printf( "Failed to read switch (%d) config data from NVS", u8SwitchNum );
                xRet = false;
            } else {
                //Serial.printf( "Read switch # (%d) config data from NVS", u8SwitchNum );
            }
        }
        else {
            //Serial.printf( "No switch (%d) config data found in NVS\n", u8SwitchNum );

            // Close NVS namespace for switch config here since, it will again open by write API
            PreferenceNvsObj.end();
            xIsOpened = false;

            // Write default switch config data
            nimble_sw_data_t xWriteDefault = { 0 };
            int switch_num = u8SwitchNum + 1; //0 based, add 1 for the switches name
            snprintf( (char *)&xWriteDefault.switch_name, 14, "Switch %d", switch_num);
            xWriteDefault.switch_number = u8SwitchNum;
            xWriteDefault.voltage_cutin = 0;
            xWriteDefault.voltage_cutout = 0;
            if ( true == mlt_nvs_write_switch_config( &xWriteDefault ) ) {
                //Serial.printf( "Success writing default switch (%d) config data in NVS\n", u8SwitchNum );
            }
            xRet = false;
        }
    }

    // Close NVS namespace for switch config data
    if ( true == xIsOpened ) {
        PreferenceNvsObj.end();
    }

    return (xRet);
}

bool mlt_nvs_erase_switch_config( uint8_t u8SwitchNum ) {
    bool xRet = true;
    bool xIsOpened = false;

    // We should have a max 10 switches ( 0 to 9 )
    if ( u8SwitchNum >= MAX_CHANNELS ) {
        // Serial.printf( "Undefined switch number: %d\n", u8SwitchNum );
        xRet = false;
    }

    // Open NVS namespace for switch config data
    if ( xRet == true ) {
        xRet = PreferenceNvsObj.begin( pcNamespace_SwConfig );
        if ( true == xRet ) {
            xIsOpened = true;
        }
        else {
            // Serial.println( "Failed to open switch config NVS" );
        }
    }

    if ( true == xRet ) {
        char nvs_key[6] = { 0 };
        // for each switch we will have NVS key like sw_0, sw_1, ... sw_9
        snprintf( nvs_key, 6, "%s_%d", pcSwKeyPrefix, u8SwitchNum );
        // Erase configuration data
        if ( true != PreferenceNvsObj.remove( nvs_key ) ) {
            // Serial.printf( "Failed to erase switch (%d) config data from NVS", u8SwitchNum );
            xRet = false;
        } else {
            // Serial.printf("Succesfully erased switch position %d config data from NVS", u8SwitchNum );
        }
    }

    // Close NVS namespace for switch config data
    if ( true == xIsOpened ) {
        PreferenceNvsObj.end();
    }

    return (xRet);
}

bool mlt_nvs_write_temp_sw_data( nimble_temp_data_t *pxTempSwWr ) {
    bool xRet = true;
    bool xIsOpened = false;

    // We should have a max 3 temp switches ( 0 to 2 )
    if ( pxTempSwWr->ch_temp_switch >= 3 ) {
        Serial.printf( "Undefined temp switch channel: %d\n", pxTempSwWr->ch_temp_switch );
        xRet = false;
    }

    // Open NVS namespace for temp switching data
    if ( true == xRet ) {
        xRet = PreferenceNvsObj.begin( pcNamespace_mlt );
        if ( true == xRet ) {
            xIsOpened = true;
        }
        else {
            // Serial.println( "Failed to open temp switching data NVS" );
        }
    }

    if ( true == xRet ) {
        char nvs_key[9] = { 0 };
        // for each temp switch channel we will have NVS key like tsw_0, tsw_1, and tsw_2
        snprintf( nvs_key, 9, "%s_%d", pcTempSwKeyPrefix, pxTempSwWr->ch_temp_switch );

        size_t xWriteLen = sizeof( nimble_temp_data_t );
        size_t xWrittenBytes = 0;
        // Write temperature switching data in NVS
        xWrittenBytes = PreferenceNvsObj.putBytes( nvs_key, (const void *)pxTempSwWr, xWriteLen );
        if ( xWrittenBytes != xWriteLen ) {
            // Serial.printf( "Failed to write ch %d temp switch data in NVS", pxTempSwWr->ch_temp_switch );
            xRet = false;
        }
    }

    // Close NVS namespace for temp switching data
    if ( true == xIsOpened ) {
        PreferenceNvsObj.end();
    }

    return (xRet);
}

bool mlt_nvs_read_temp_sw_data( uint8_t u8TempSwCh, nimble_temp_data_t *pxTempSwRd ) {
    bool xRet = true;
    bool xIsOpened = false;

    // We should have a max 3 temp switches ( 0 to 2 )
    if ( u8TempSwCh >= 3 ) {
        Serial.printf( "Undefined temp switch channel: %d\n", u8TempSwCh );
        xRet = false;
    }

    // Open NVS namespace for temp switching data
    if ( true == xRet ) {
        xRet = PreferenceNvsObj.begin( pcNamespace_mlt );
        if ( true == xRet ) {
            xIsOpened = true;
        }
        else {
            // Serial.println( "Failed to open temp switching data NVS" );
        }
    }

    if ( true == xRet ) {
        char nvs_key[9] = { 0 };
        // for each temp switch channel we will have NVS key like tsw_0, tsw_1, and tsw_2
        snprintf( nvs_key, 9, "%s_%d", pcTempSwKeyPrefix, u8TempSwCh );
        // Check if there is configuration already written with the key
        if ( 0 != PreferenceNvsObj.getBytesLength( nvs_key ) ) {
            size_t xReadLen = sizeof( nimble_temp_data_t );
            size_t xReadBytes = 0;
            // Read the switch config data from NVS
            xReadBytes = PreferenceNvsObj.getBytes( nvs_key, (void *)pxTempSwRd, xReadLen );
            if ( xReadBytes != xReadLen ) {
                // Serial.printf( "Failed to read temp switch channel (%d) config data from NVS", u8TempSwCh );
                xRet = false;
            }
        }
        else {
            // Serial.printf( "No switch (%d) config data found in NVS\n", u8TempSwCh );

            // Close NVS namespace for switch config here since, it will again open by write API
            PreferenceNvsObj.end();
            xIsOpened = false;

            // Write defualt switch config data
            nimble_temp_data_t xWriteDefault = {
                .switching_condition = 0,
                .switching_value = 300,
                .switch_on = 0
            };
            xWriteDefault.ch_temp_switch = u8TempSwCh;
            if ( true == mlt_nvs_write_temp_sw_data( &xWriteDefault ) ) {
                // Serial.printf( "Success writing default switch (%d) config data in NVS\n", u8TempSwCh );
            }
            xRet = false;
        }
    }

    // Close NVS namespace for temp switcing data
    if ( true == xIsOpened ) {
        PreferenceNvsObj.end();
    }

    return (xRet);
}

bool mlt_nvs_erase_temp_sw_data( uint8_t u8TempSwCh ) {
    bool xRet = true;
    bool xIsOpened = false;

    // We should have a max 3 temp switches ( 0 to 2 )
    if ( u8TempSwCh >= 3 ) {
        // Serial.printf( "Undefined temp switch channel: %d\n", u8TempSwCh );
        xRet = false;
    }

    // Open NVS namespace for temp switching data
    if ( xRet == true ) {
        xRet = PreferenceNvsObj.begin( pcNamespace_mlt );
        if ( true == xRet ) {
            xIsOpened = true;
        }
        else {
            // Serial.println( "Failed to open temp switching data NVS" );
        }
    }

    if ( true == xRet ) {
        char nvs_key[9] = { 0 };
        // for each temp switch channel we will have NVS key like tsw_0, tsw_1, and tsw_2
        snprintf( nvs_key, 9, "%s_%d", pcTempSwKeyPrefix, u8TempSwCh );
        // Erase temp switching data
        if ( true != PreferenceNvsObj.remove( nvs_key ) ) {
            // Serial.printf( "Failed to erase temp switch (ch - %d) data from NVS", u8TempSwCh );
            xRet = false;
        } else {
            // Serial.printf("Succesfully erased temp switch (ch - %d) data from NVS", u8TempSwCh );
        }
    }

    // Close NVS namespace for temp switching data
    if ( true == xIsOpened ) {
        PreferenceNvsObj.end();
    }

    return (xRet);
}

bool mlt_nvs_write_vendor_id( uint16_t u16VendorVersionWr ) {
    bool xRet = true;
    bool xIsOpened = false;

    // Open NVS namespace for hardware version
    xRet = PreferenceNvsObj.begin( pcNamespace_mlt );
    if ( true == xRet ) {
        xIsOpened = true;
    }
    else {
        // Serial.println( "Failed to open hardware version NVS" );
    }

    if ( true == xRet ) {
        // Write hardware version in NVS
        if ( 1 != PreferenceNvsObj.putUChar( pcVendorKey, u16VendorVersionWr ) ) {
            // Serial.println( "Failed to write hardware version in NVS" );
            xRet = false;
        }
    }

    // Close NVS namespace for hardware version
    if ( true == xIsOpened ) {
        PreferenceNvsObj.end();
    }

    return (xRet);
}

bool mlt_nvs_read_vendor_id(uint16_t *u16VendorVersionWr) {
    bool xRet = true;
    bool xIsOpened = false;

    // Open NVS namespace for hardware version
    xRet = PreferenceNvsObj.begin( pcNamespace_mlt );
    if ( true == xRet ) {
        xIsOpened = true;
    }
    else {
        // // Serial.println( "Failed to open hardware version NVS" );
    }

    if ( true == xRet ) {
        // Read hardware version from NVS. 0 is the default/base hardware version
        *u16VendorVersionWr = PreferenceNvsObj.getUChar( pcVendorKey );
    }

    // Close NVS namespace for hardware version
    if ( true == xIsOpened ) {
        PreferenceNvsObj.end();
    }

    return (xRet);
}

bool mlt_nvs_write_hardware_version( uint8_t u8HwVersionWr ) {
    bool xRet = true;
    bool xIsOpened = false;

    // Open NVS namespace for hardware version
    xRet = PreferenceNvsObj.begin( pcNamespace_mlt );
    if ( true == xRet ) {
        xIsOpened = true;
    }
    else {
        // Serial.println( "Failed to open hardware version NVS" );
    }

    if ( true == xRet ) {
        // Write hardware version in NVS
        if ( 1 != PreferenceNvsObj.putUChar( pcHwdVersionKey, u8HwVersionWr ) ) {
            // Serial.println( "Failed to write hardware version in NVS" );
            xRet = false;
        }
    }

    // Close NVS namespace for hardware version
    if ( true == xIsOpened ) {
        PreferenceNvsObj.end();
    }

    return (xRet);
}

bool mlt_nvs_read_hardware_version( uint8_t *pu8HwVersionRd ) {
    bool xRet = true;
    bool xIsOpened = false;

    // Open NVS namespace for hardware version
    xRet = PreferenceNvsObj.begin( pcNamespace_mlt );
    if ( true == xRet ) {
        xIsOpened = true;
    }
    else {
        // // Serial.println( "Failed to open hardware version NVS" );
    }

    if ( true == xRet ) {
        // Read hardware version from NVS. 0 is the default/base hardware version
        *pu8HwVersionRd = PreferenceNvsObj.getUChar( pcHwdVersionKey );
    }

    // Close NVS namespace for hardware version
    if ( true == xIsOpened ) {
        PreferenceNvsObj.end();
    }

    return (xRet);
}

bool mlt_nvs_write_calibration_data( calibration_t *pxCalibDataWr ) {
    bool xRet = true;
    bool xIsOpened = false;

    // Open NVS namespace for calibration data
    xRet = PreferenceNvsObj.begin( pcNamespace_mlt );
    if ( true == xRet ) {
        xIsOpened = true;
    }
    else {
        // Serial.println( "Failed to open calibration data NVS" );
    }

    if ( true == xRet ) {
        size_t xWriteLen = sizeof( calibration_t );
        size_t xWrittenBytes = 0;
        // Write calibration data in NVS
        xWrittenBytes = PreferenceNvsObj.putBytes( pcCalibKey, (const void *)pxCalibDataWr, xWriteLen );
        if ( xWrittenBytes != xWriteLen ) {
            // Serial.println( "Failed to write calibration data in NVS" );
            xRet = false;
        }
    }

    // Close NVS namespace for calibration data
    if ( true == xIsOpened ) {
        PreferenceNvsObj.end();
    }

    return (xRet);
}

bool mlt_nvs_read_calibration_data( calibration_t *pxCalibDataRd ) {
    bool xRet = true;
    bool xIsOpened = false;

    // Open NVS namespace for calibration data
    xRet = PreferenceNvsObj.begin( pcNamespace_mlt );
    if ( true == xRet ) {
        xIsOpened = true;
    }
    else {
        // Serial.println( "Failed to open calibration data NVS" );
    }

    if ( true == xRet ) {
        // Check if there is data already written with the key
        if ( 0 != PreferenceNvsObj.getBytesLength( pcCalibKey ) ) {
            size_t xReadLen = sizeof( calibration_t );
            size_t xReadBytes = 0;
            // Read the calibration data from NVS
            xReadBytes = PreferenceNvsObj.getBytes( pcCalibKey, (void *)pxCalibDataRd, xReadLen );
            if ( xReadBytes != xReadLen ) {
                // Serial.println( "Failed to read calibration data from NVS" );
                xRet = false;
            }
        }
        else {
            // Serial.println( "No calibration data found in NVS" );
            xRet = false;
        }
    }

    // Close NVS namespace for calibration data
    if ( true == xIsOpened ) {
        PreferenceNvsObj.end();
    }

    return (xRet);
}

bool mlt_nvs_write_config_data( config_new_t *pxConfigDataWr ) {
    bool xRet = true;
    bool xIsOpened = false;

    // Open NVS namespace for config data
    xRet = PreferenceNvsObj.begin( pcNamespace_mlt );
    if ( true == xRet ) {
        xIsOpened = true;
    }
    else {
        // Serial.println( "Failed to open config data NVS" );
    }

    if ( true == xRet ) {
        size_t xWriteLen = sizeof( config_new_t );
        size_t xWrittenBytes = 0;
        // Write config data in NVS
        xWrittenBytes = PreferenceNvsObj.putBytes( pcConfigKey, (const void *)pxConfigDataWr, xWriteLen );
        if ( xWrittenBytes != xWriteLen ) {
            // Serial.println( "Failed to write config data in NVS" );
            xRet = false;
        }
    }

    // Close NVS namespace for config data
    if ( true == xIsOpened ) {
        PreferenceNvsObj.end();
    }

    return (xRet);
}

bool mlt_nvs_read_config_data( config_new_t *pxConfigDataRd ) {
    bool xRet = true;
    bool xIsOpened = false;

    // Open NVS namespace for config data
    xRet = PreferenceNvsObj.begin( pcNamespace_mlt );
    if ( true == xRet ) {
        xIsOpened = true;
    }
    else {
        Serial.println( "Failed to open config data NVS" );
    }

    if ( true == xRet ) {
        // Check if there is data already written with the key
        if ( 0 != PreferenceNvsObj.getBytesLength( pcConfigKey ) ) {
            size_t xReadLen = sizeof( config_new_t );
            size_t xReadBytes = 0;
            // Read the config data from NVS
            xReadBytes = PreferenceNvsObj.getBytes( pcConfigKey, (void *)pxConfigDataRd, xReadLen );
            if ( xReadBytes != xReadLen ) {
                // Serial.println( "Failed to read config data from NVS" );
                xRet = false;
            }
        }
        else {
            Serial.println( "No config data found in NVS" );
            xRet = false;
        }
    }

    // Close NVS namespace for config data
    if ( true == xIsOpened ) {
        PreferenceNvsObj.end();
    }

    return (xRet);
}

bool mlt_nvs_write_demo_mode( bool xDemoMode ) {
    bool xRet = true;
    bool xIsOpened = false;

    // Open NVS namespace for demo mode
    xRet = PreferenceNvsObj.begin( pcNamespace_mlt );
    if ( true == xRet ) {
        xIsOpened = true;
    }
    else {
        // Serial.println( "Failed to open demo mode NVS" );
    }

    if ( true == xRet ) {
        // Write demo mode in NVS
        if ( true != PreferenceNvsObj.putBool( pcDemoKey, xDemoMode ) ) {
            // Serial.println( "Failed to write demo mode in NVS" );
            xRet = false;
        }
    }

    // Close NVS namespace for demo mode
    if ( true == xIsOpened ) {
        PreferenceNvsObj.end();
    }

    return (xRet);
}

bool mlt_nvs_read_demo_mode( bool *pxDemoMode ) {
    bool xRet = true;
    bool xIsOpened = false;

    // Open NVS namespace for demo mode
    xRet = PreferenceNvsObj.begin( pcNamespace_mlt );
    if ( true == xRet ) {
        xIsOpened = true;
    }
    else {
        // Serial.println( "Failed to open demo mode NVS" );
    }

    if ( true == xRet ) {
        // Read demo mode. Demo off is default value
        *pxDemoMode = PreferenceNvsObj.getBool( pcDemoKey );
    }

    // Close NVS namespace for demo mode
    if ( true == xIsOpened ) {
        PreferenceNvsObj.end();
    }

    return (xRet);
}

bool mlt_nvs_write_adc_mode( bool xAdcMode ) {
    bool xRet = true;
    bool xIsOpened = false;

    // Open NVS namespace for ADC mode
    xRet = PreferenceNvsObj.begin( pcNamespace_mlt );
    if ( true == xRet ) {
        xIsOpened = true;
    }
    else {
        // Serial.println( "Failed to open ADC mode NVS" );
    }

    if ( true == xRet ) {
        // Write ADC mode in NVS
        if ( true != PreferenceNvsObj.putBool( pcAdcKey, xAdcMode ) ) {
            // Serial.println( "Failed to write ADC mode in NVS" );
            xRet = false;
        }
    }

    // Close NVS namespace for ADC mode
    if ( true == xIsOpened ) {
        PreferenceNvsObj.end();
    }

    return (xRet);
}

bool mlt_nvs_read_adc_mode( bool *pxAdcMode ) {
    bool xRet = true;
    bool xIsOpened = false;

    // Open NVS namespace for ADC mode
    xRet = PreferenceNvsObj.begin( pcNamespace_mlt );
    if ( true == xRet ) {
        xIsOpened = true;
    }
    else {
        // Serial.println( "Failed to open ADC mode NVS" );
    }

    if ( true == xRet ) {
        // Read ADC mode. ADC off is default value
        *pxAdcMode = PreferenceNvsObj.getBool( pcAdcKey );
    }

    // Close NVS namespace for ADC mode
    if ( true == xIsOpened ) {
        PreferenceNvsObj.end();
    }

    return (xRet);
}

bool mlt_nvs_write_temp_sensor_id( const uint8_t *pu8TempSensorIdWr ) {
    bool xRet = true;
    bool xIsOpened = false;
    bool xIsIdWritten = false;

    // Open NVS namespace for internal temp sensor ID
    if ( true == xRet ) {
        xRet = PreferenceNvsObj.begin( pcNamespace_mlt );
        if ( true == xRet ) {
            xIsOpened = true;
        }
        else {
            // Serial.println( "Failed to open internal temp sensor ID NVS" );
        }
    }

    if ( true == xRet ) {
        // check if ID is already written
        xIsIdWritten = PreferenceNvsObj.getBool( pcIsIdWrittenKey );

        if ( true == xIsIdWritten ) {
            // Serial.println( "Internal temp sensor ID already written! Can't overwrite!!" );
            xRet = false;
        } 
        else {
            size_t xWriteLen = TEMP_SENSOR_ID_LEN;
            size_t xWrittenBytes = 0;
            xWrittenBytes = PreferenceNvsObj.putBytes( pcTempSensorIdKey, (const void*)pu8TempSensorIdWr, xWriteLen );
            if ( xWrittenBytes == xWriteLen ) {
                // Serial.println( "Success writing internal temp sensor ID" );
                if ( true != PreferenceNvsObj.putBool( pcIsIdWrittenKey, true ) ) {
                    // Serial.println( "Failed to write lock internal temp sensor ID" );
                }
            }
            else {
                // Serial.println( "Failed writing internal temp sensor ID" );
                xRet = false;
            }
        }
    }

    // Close NVS namespace for internal temp sensor ID
    if ( true == xIsOpened ) {
        PreferenceNvsObj.end();
    }

    return (xRet);
}

bool mlt_nvs_read_temp_sensor_id( uint8_t *pu8TempSensroIdRd ) {
    bool xRet = true;
    bool xIsOpened = false;

    // Open NVS namespace for internal temp sensor ID
    xRet = PreferenceNvsObj.begin( pcNamespace_mlt );
    if ( true == xRet ) {
        xIsOpened = true;
    }
    else {
        // Serial.println( "Failed to open internal temp sensor ID NVS" );
    }

    if ( true == xRet ) {
        // Read internal temp sensor ID
        if ( 0 != PreferenceNvsObj.getBytesLength( pcTempSensorIdKey ) ) {
            size_t xReadLen = TEMP_SENSOR_ID_LEN;
            size_t xReadBytes = 0;
            // Read the switch config data from NVS
            xReadBytes = PreferenceNvsObj.getBytes( pcTempSensorIdKey, (void *)pu8TempSensroIdRd, xReadLen );
            if ( xReadBytes != xReadLen ) {
                // Serial.printf( "Failed to read temp sensor ID from NVS %d\n", xReadBytes );
                xRet = false;
            }
        }
        else {
            // Serial.println( "No temp sensor ID found in NVS" );
            xRet = false;
        }
    }

    // Close NVS namespace for internal temp sensor ID
    if ( true == xIsOpened ) {
        PreferenceNvsObj.end();
    }

    return (xRet);
}

bool mlt_nvs_write_onboard_cutout_temp( uint16_t i32CutoutTempWr ) {
    bool xRet = true;
    bool xIsOpened = false;

    // Open NVS namespace for onboard temp cutout threshold
    xRet = PreferenceNvsObj.begin( pcNamespace_mlt );
    if ( true == xRet ) {
        xIsOpened = true;
    }
    else {
        // Serial.println( "Failed to open onboard temp cutout NVS" );
    }

    if ( true == xRet ) {
        // Write onboard cutout temp threshold in NVS
        if ( 4 != PreferenceNvsObj.putInt( pcOnboardTempCutoutKey, i32CutoutTempWr ) ) {
            // Serial.println( "Failed to write onboard temp cutout threshold in NVS" );
            xRet = false;
        }
    }

    // Close NVS namespace for onboard temp cutout threshold
    if ( true == xIsOpened ) {
        PreferenceNvsObj.end();
    }

    return (xRet);
}

bool mlt_nvs_read_onboard_cutout_temp( uint16_t *pi32CutoutTempRd ) {
    bool xRet = true;
    bool xIsOpened = false;

    // Open NVS namespace for onboard temp cutout threshold
    xRet = PreferenceNvsObj.begin( pcNamespace_mlt );
    if ( true == xRet ) {
        xIsOpened = true;
    }
    else {
        // Serial.println( "Failed to open onboard temp cutout NVS" );
    }

    if ( true == xRet ) {
        // Read onboard temp cutout threshold from NVS.
        // If no value is found, 0 will be considered as default value
        *pi32CutoutTempRd = PreferenceNvsObj.getInt( pcOnboardTempCutoutKey );
    }

    // Close NVS namespace for onboard temp cutout threshold
    if ( true == xIsOpened ) {
        PreferenceNvsObj.end();
    }

    return (xRet);
}


// Code to show hardware version in main - CURRENTLY NOT REQUIRED
// uint8_t u8HardwareVersion = 0; // 0 is default/base version
//   if ( true == mlt_nvs_read_hardware_version( &u8HardwareVersion ) ) {
//     Serial.printf( "Hardware version: %d\n", u8HardwareVersion );
//   }
//   else {
//     Serial.println( "Failed to read hardware version from NVS." );
//   }

/******************************* END OF FILE *******************************/

