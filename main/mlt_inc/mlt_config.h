#ifndef MLT_CONFIG_H
#define MLT_CONFIG_H

// New config structure
typedef struct config_new_t {
    uint8_t   ssid[MAX_SSID_LEN];       // WiFi and BLE SSID
    uint8_t   pass[MAX_PASS_LEN];       // WiFi Password
    uint32_t  pin;                      // BLE pin
    uint16_t  lowcut;                   // global undervoltage cutout level in mV
    float     calibration_gain;         // calibration data
    float     calibration_offset;       // calibration data
    uint32_t  magic;                    // magic pattern
}config_new_t;

extern config_new_t config_new;
extern bool demo_mode_activated;
extern bool adc_activated;


void config_init(void);

void config_restore_defaults(void);

void config_restore_demo(void);

void config_load(void);

void config_save(void);

void config_demo_on(void);

void config_demo_off(void);

void config_adc_activate(void);

void config_adc_deactivate(void);

#endif //MLT_CONFIG_H
