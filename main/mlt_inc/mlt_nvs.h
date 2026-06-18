#ifndef MLT_NVS_H
#define MLT_NVS_H

#include "mlt_nimble.h"
#include "mlt_config.h"
#include "mlt_calibration.h"

/**
 * @brief API copies configuration data from ESP32 NVS into RAM buffer to
 * process in application during runtime
 * @param None
 * @return None
*/
void mlt_copy_nvs_to_ram( void );

/**
 * @brief API writes switch configuration to ESP32 NVS storage after opening the namespace
 * and it will also close the the opened namespace before return
 * @param[in] pxSwitchConfigWr pointer to the write switch config data structure
 * @return true on successfull write, false on write fail
*/
bool mlt_nvs_write_switch_config( nimble_sw_data_t *pxSwitchConfigWr );

/**
 * @brief API reads switch config structure from ESP32 NVS storage after opening the namespace
 * and it will also close the the opened namespace before return
 * @param[in] u8SwitchNum Switch Number (0 to 9) to read
 * @param[out] pxSwitchConfigRd pointer to raed switch config data structure
 * @return true on successfull read, false on read fail
*/
bool mlt_nvs_read_switch_config( uint8_t u8SwitchNum, nimble_sw_data_t *pxSwitchConfigRd );

/**
 * @brief API erases switch configure data from ESP32 NVS storage. It will open the respective
 * NVS namespace and close it after erase is done
 * @param[in] u8SwitchNum Switch number to erase (0 to 9) config data
 * @return true on successfull erase, false on erase fail
*/
bool mlt_nvs_erase_switch_config( uint8_t u8SwitchNum );

/**
 * @brief API writes temperature switching data to ESP32 NVS storage. It will open the respective
 * NVS namespace and close it after write operation is done
 * @param[in] pxTempSwWr poniter to the write temp switching data structure
 * @return true on successfull write, false on write fail
*/
bool mlt_nvs_write_temp_sw_data( nimble_temp_data_t *pxTempSwWr );

/**
 * @brief API reads temperature switching data from ESP32 NVS storage. It will open the respective
 * NVS namespace and close it after read operation is done
 * @param[in] u8TempSwCh Temp switch channel number (0 to 2)
 * @param[in] pxTempSwRd poniter to the raed temp switching data structure
 * @return true on successfull read, false on read fail
*/
bool mlt_nvs_read_temp_sw_data( uint8_t u8TempSwCh, nimble_temp_data_t *pxTempSwRd );

/**
 * @brief API erases temp switch channel data from ESP32 NVS storage. It will open the respective
 * NVS namespace and close it after erase is done
 * @param[in] u8TempSwCh Temp switch channel number (0 to 2) to erase data for
 * @return true on successfull erase, false on erase fail
*/
bool mlt_nvs_erase_temp_sw_data( uint8_t u8TempSwCh );

/**
 * @brief API writes vendor ID (read from serial console) to ESP32 NVS storage.
 * It will open the respective NVS namespace and close it after write operation is done
 * @param[in] u8HwVersionWr Hardware version in uint8_t format
 * @return true on successfull write, false on write fail
*/
bool mlt_nvs_write_vendor_id( uint16_t u16VendorVersionWr );

/**
 * @brief API reads vendor ID from ESP32 NVS storage. It will open the respective
 * NVS namespace and close it after read operation is done
 * @param[in] pu8HwVersionRd poniter to the varaible (uint8_t) to store hardware version in runtime
 * @return true on successfull read, false on read fail
*/
bool mlt_nvs_read_vendor_id( uint16_t *u16VendorVersionWr );

/**
 * @brief API writes hardware version (read from serial console) to ESP32 NVS storage.
 * It will open the respective NVS namespace and close it after write operation is done
 * @param[in] u8HwVersionWr Hardware version in uint8_t format
 * @return true on successfull write, false on write fail
*/
bool mlt_nvs_write_hardware_version( uint8_t u8HwVersionWr );

/**
 * @brief API reads hardware version from ESP32 NVS storage. It will open the respective
 * NVS namespace and close it after read operation is done
 * @param[in] pu8HwVersionRd poniter to the varaible (uint8_t) to store hardware version in runtime
 * @return true on successfull read, false on read fail
*/
bool mlt_nvs_read_hardware_version( uint8_t *pu8HwVersionRd );

/**
 * @brief API writes calibration data to ESP32 NVS storage. It will open the respective NVS namespace
 * and close it after the write operation is done
 * @param[in] pxCalibDataWr Pointer to the write calibration data structure
 * @return true on successfull write, false on write fail
*/
bool mlt_nvs_write_calibration_data( calibration_t *pxCalibDataWr );

/**
 * @brief API reads calibration data from ESP32 NVS storage. It will open the respective NVS namespace
 * and close it after the read operation is done
 * @param[out] pxCalibDataRd Pointer to the read calibration data structure
 * @return true on successfull read, false on read fail
*/
bool mlt_nvs_read_calibration_data( calibration_t *pxCalibDataRd );

/**
 * @brief API writes config data to ESP32 NVS storage. It will open the respective NVS namespace
 * and close it after the write operation is done
 * @param[in] pxConfigDataWr Pointer to the write config data structure
 * @return true on successfull write, false on write fail
*/
bool mlt_nvs_write_config_data( config_new_t *pxConfigDataWr );

/**
 * @brief API reads config data from ESP32 NVS storage. It will open the respective NVS namespace
 * and close it after the read operation is done
 * @param[out] pxConfigDataRd Pointer to the read config data structure
 * @return true on successfull read, false on read fail
*/
bool mlt_nvs_read_config_data( config_new_t *pxConfigDataRd );

/**
 * @brief API writes demo on/off mode in the ESP32 NVS storage. It will open the respective NVS namespace
 * and close it after the read operation is done
 * @param[in] xDemoMode true for demo on, false for demo off
 * @return true on successfull write, false on write fail
*/
bool mlt_nvs_write_demo_mode( bool xDemoMode );

/**
 * @brief API reads demo on/off mode from the ESP32 NVS storage. It will open the respective NVS namespace
 * and close it after the read operation is done
 * @param[out] pxDemoMode Pointer to variable (bool type) to store read demo mode
 * @return true on successfull read, false on read fail
*/
bool mlt_nvs_read_demo_mode( bool *pxDemoMode );

/**
 * @brief API writes ADC on/off mode in the ESP32 NVS storage. It will open the respective NVS namespace
 * and close it after the read operation is done
 * @param[in] xAdcMode true for ADC activated, false for ADC deactivated
 * @return true on successfull write, false on write fail
*/
bool mlt_nvs_write_adc_mode( bool xAdcMode );

/**
 * @brief API reads ADC activated/deactivated status from the ESP32 NVS storage. It will open the
 * respective NVS namespace and close it after the read operation is done
 * @param[out] pxAdcMode Pointer to variable (bool type) to store read ADC mode
 * @return true on successfull read, false on read fail
*/
bool mlt_nvs_read_adc_mode( bool *pxAdcMode );

/**
 * @brief API writes internal temperature sensor ID in the ESP32 NVS storage. It wil open the respective
 * NVS namespace and close it after the read operation is done.
 * @param[in] pu8TempSensorIdWr Pointer to sensor id string (const uint8_t *)
 * @return true on successfull write, false on write fail
*/
bool mlt_nvs_write_temp_sensor_id( const uint8_t *pu8TempSensorIdWr );

/**
 * @brief API reads internal temperature sensor ID from the ESP32 NVS storage. It wil open the respective
 * NVS namespace and close it after the read operation is done.
 * @param[out] pu8TempSensroIdRd Pointer to variable (uint8_t *) to copy read sensor id string
 * @return true on successfull write, false on write fail
*/
bool mlt_nvs_read_temp_sensor_id( uint8_t *pu8TempSensroIdRd );

/**
 * @brief API writes onboard temperature cutout threshold value to ESP32 NVS storage. It wil open the respective
 * NVS namespace and close it after the read operation is done.
 * @param[in] i32CutoutTempWr onboard temp cutout value (int32_t type)
 * @return true on successfull write, false on write fail
*/
bool mlt_nvs_write_onboard_cutout_temp( uint16_t pui16CutoutTempWr );

/**
 * @brief API reads onboard temperature cutout threshold value from the ESP32 NVS storage. It wil open the respective
 * NVS namespace and close it after the read operation is done.
 * @param[out] pi32CutoutTempRd Pointer to variable (int32_t *) to copy onboard cutout temp
 * @return true on successfull write, false on write fail
*/
bool mlt_nvs_read_onboard_cutout_temp( uint16_t *pui16CutoutTempRd );

#endif //MLT_NVS_H

/******************************* END OF FILE *******************************/
