#ifndef MLT_NIMBLE_H
#define MLT_NIMBLE_H

#include "mlt_defs.h"


// Define UUIDs for different hardware variants
#define SM_UUID                   "42574065-18ed-41e9-a723-b85ac1d74000" // SwitchMaster UUID 
#define SM_MAXI_UUID                        "42574065-18ed-41e9-a723-b85ac1d74001" // MAXI UUID
#define SM_TEST_VENDOR_UUID                        "42574065-18ed-41e9-a723-b85ac1d74002" // Test vendor SwitchMaster

// Define UUIDs for characteristics
#define METRICS_CHARACTERISTIC_UUID         "2ff2caa4-7932-44c6-b73a-a7cfb897400a"  // Open Tx for switches, voltage and currents and current averages
#define DEV_CONFIG_CHARACTERISTIC_UUID          "2ff2caa4-7932-44c6-b73a-a7cfb897400c"  // Device configuration items Tx and Rx
#define SW_CONFIG_CHARACTERISTIC_UUID          "2ff2caa4-7932-44c6-b73a-a7cfb897400d"  // Device configuration items Tx and Rx

#define SET_SWITCHES_CHARACTERISTIC_UUID          "2ff2caa4-7932-44c6-b73a-a7cfb897401a"  // Secure channel for switching outputs
#define TIMERS_CHARACTERISTIC_UUID            "2ff2caa4-7932-44c6-b73a-a7cfb897401d"  // Secure channel for channel timers
#define LOCKS_CHARACTERISTIC_UUID           "2ff2caa4-7932-44c6-b73a-a7cfb897401e"  // Secure channel for channel lockouts
#define TEMPS_CHARACTERISTIC_UUID            "2ff2caa4-7932-44c6-b73a-a7cfb897401f"  // Secure channel for channel timeouts
#define SMARTWATCH_CHARACTERISTIC_UUID            "2ff2caa4-7932-44c6-b73a-a7cfb897400e"  // Open channel for switching via Smartwatch/Garmin/Non-authenticated devices

//*****************************************************************************
// NIMBLE message structures
//


// status structure
#pragma pack(2)   // pack on 2-byte boundary, so there is no "holes"
typedef struct nimble_metrics_t {
  uint16_t  outputs;            // output bitmask, LSB = Output1
  uint8_t   usb;                // usb on or off. 0 = off, 1 = on
  uint16_t  voltage;            // voltage in 1mV steps
  uint16_t  current;            // current in 1mA steps
  uint16_t  last_hour;          // average current the last hour in 10mA steps
  uint16_t  last_12;            // average current the last 12 hours in 10mA steps
  uint8_t override_active;      // is there an override active? 0 = No, 1 = Yes
  uint16_t  ch_temps[3];    // Individual temperature value in Kelvin * 100 (-273K is 0°C)
}nimble_metrics_t;

// setting structure with output status
typedef struct nimble_set_t {
    uint16_t  setting;            // bitmask with settings
    uint8_t   usb;                // usb on or off. 0 = off, 1 = on
}nimble_set_t;

// Device config structure for read and write
typedef struct nimble_dev_cfg_t {
  uint8_t   ble_adr[6];             // device mac
  uint8_t   ssid[MAX_SSID_LEN];     // WiFi and BLE SSID
  uint8_t   pass[MAX_PASS_LEN];     // WiFi Password
  uint32_t  pin;                    // BLE pin
  uint16_t  lowcut;                 // global battery cutout value
  uint16_t  gbltempcutout;          // global temperature cutout in °C * 100
}nimble_dev_cfg_t;

// Switch config structure for ble comms
typedef struct nimble_sw_cfg_t {
  uint8_t   mode;                   // Tells the characteristic what to do. 0 = read the received switch # from NVS. 1 = store received switch info to NVS.
  uint8_t   switch_number;          // Switch Number for configuration
  uint8_t   switch_name[14];        // Switch name
  uint16_t  voltage_cutin;          // Voltage in mV for when this switch should cut in. (mV: 12.5v = 12500). 0 = disabled.
  uint16_t  voltage_cutout;         // Voltage in mV for when this switch should cut out. (mV: 12.5v = 12500). 0 = disabled.
}nimble_sw_cfg_t;

// Switch config structure to send to the NVS
typedef struct nimble_sw_data_t {
  uint8_t   switch_number;          // Switch Number for configuration
  uint8_t   switch_name[14];        // Switch name
  uint16_t  voltage_cutin;          // Voltage in mV for when this switch should cut in. (mV: 12.5v = 12500). 0 = disabled.
  uint16_t  voltage_cutout;         // Voltage in mV for when this switch should cut out. (mV: 12.5v = 12500). 0 = disabled.
}nimble_sw_data_t;

// Config structure for lockouts
typedef struct nimble_lockout_t {
  uint16_t  ch_lockout[MAX_CHANNELS];    // individual lockout value - 0 off. 1 locked
}nimble_cfg_lockout_t;

// new config structure for channel timeouts
typedef struct nimble_timeouts_t {
  uint16_t  ch_timeouts[MAX_CHANNELS];    // individual timer cutout value
 }nimble_timeouts_t;

// Temp config structure for ble comms
typedef struct nimble_temp_cfg_t {
  uint8_t  mode;                 // Tells the characteristic what to do. 0 = read the received temp info from NVS. 1 = store received temp info to NVS.
  uint8_t  ch_temp_switch;       // temperature channel to switch from
  uint8_t  switching_condition;  // the switching condition. 0 = off, 1=(if greater than switching value do x), 2=(if lower than switching value do x)
  uint16_t  switching_value;     // Individual temperature value in Kelvin * 100 (-273K is 0°C)
  uint8_t  switch_on;            // single number with which switch to turn on or off. 0 based. (Allows only one)
 }nimble_temp_cfg_t;

// Temp config structure to send to the NVS
typedef struct nimble_temp_data_t {
  uint8_t  ch_temp_switch;        // temperature channel to switch from
  uint8_t  switching_condition;   // the switching condition. 0 = off, 1=(if greater than switching value do x), 2=(if lower than switching value do x)
  uint16_t  switching_value;      // Individual temperature value to switch at in Kelvin
  uint8_t  switch_on;             // single number with which switch to turn on or off. 0 based. (Allows only one)
 }nimble_temp_data_t;

// setting structure for smartwatch control
typedef struct nimble_smartwatch_t {
    uint32_t  rxpin;              // PIN number to compare
    uint16_t  setting;            // bitmask with settings
}nimble_smartwatch_t;

typedef struct espnow_control {
  uint8_t msgType;          // 0 for keep alive, 1 for output control, 2 for lockout control
  uint8_t apmac[6];         // RC device mac
  uint8_t output_num;       // output number
  uint8_t lockout_num;      // 0 for off, 1 for on
} espnow_control;

typedef struct espnow_data {
  uint16_t output_status;     // output bitmask, LSB = Output1
  uint16_t lockout_status [MAX_CHANNELS];    // individual lockout value - 0 off. 1 locked // output number
} espnow_data;


#pragma pack()


//*****************************************************************************

bool is_mac_valid(uint8_t *pmac);


void nimble_init(const char * ssid, uint32_t pin);

void nimble_build_output(float lasthour, float last12hours, uint8_t override_active);

void nimble_build_lockouts(void);
void nimble_build_timers(void);

void nimble_set_globallv_read(void);
void nimble_set_cfg_read(void);

#endif //MLT_NIMBLE_H