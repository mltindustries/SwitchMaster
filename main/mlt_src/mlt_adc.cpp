
#include <Arduino.h>
#include <Wire.h>

#include "mlt_defs.h"
#include "mlt_config.h"
#include "mlt_calibration.h"

#include "mlt_adc.h"

/******************************************************************************/

//
// I2C functions for talking to the NAU7802
//
TwoWire *_wire = new TwoWire(0);

static bool nau_write(uint8_t reg, uint8_t value) {
  _wire->beginTransmission(NAU7802_I2CADDR);
  _wire->write((uint8_t)reg);
  _wire->write((uint8_t)value);
  return (_wire->endTransmission() == 0);
}

static uint8_t nau_read(uint8_t reg) {
  _wire->beginTransmission(NAU7802_I2CADDR);
  _wire->write((uint8_t)reg);
  _wire->endTransmission();

  _wire->requestFrom((int) NAU7802_I2CADDR, (int) 1);
  return _wire->read();
}

static uint32_t nau_read24(uint8_t reg) {
  uint32_t val;
  _wire->beginTransmission(NAU7802_I2CADDR);
  _wire->write((uint8_t)reg);
  _wire->endTransmission();

  _wire->requestFrom((int) NAU7802_I2CADDR, (int) 3);
  val = _wire->read();    // receive high byte
  val <<= 8;              // shift byte to make room for new byte
  val |= _wire->read();   // receive mid byte
  val <<= 8;              // shift both bytes
  val |= _wire->read();   // receive low byte
  return val;
}

static void nau_writebit(uint8_t reg, uint8_t bit) {
  uint8_t val = nau_read(reg) | (1<<bit);
  nau_write(reg, val);
}

void nau_clearbit(uint8_t reg, uint8_t bit) {
  uint8_t val = nau_read(reg) & ~(1<<bit);
  nau_write(reg, val);
}

static bool nau_readbit(uint8_t reg, uint8_t bit) {
  //create bitmask
  uint8_t bitmask = 1<<bit;
  if(nau_read(reg) & bitmask){
    return true;
  }
  return false;
}

static void nau_readuntiltruedelay(uint8_t reg, uint8_t bit, uint16_t delaytime) {
  //create bitmask
  uint8_t bitmask = 1<<bit;
  //Just keep reading until bit requested is false
  while (1) {
    if (nau_read(reg) & bitmask)
        break;
    if (delaytime)
        delay(delaytime);
  }
}

static void nau_readuntiltrue(uint8_t reg, uint8_t bit) {
    nau_readuntiltruedelay(reg, bit, 0);
}

static void nau_readuntilfalsedelay(uint8_t reg, uint8_t bit, uint16_t delaytime) {
  //create bitmask
  uint8_t bitmask = 1<<bit;
  //Just keep reading until bit requested is false
  while (1) {
    if (~nau_read(reg) & bitmask)
        break;
    if (delaytime)
        delay(delaytime);
  }
}

static void nau_readuntilfalse(uint8_t reg, uint8_t bit) {
    nau_readuntilfalsedelay(reg, bit, 0);
}

/******************************************************************************/



static void nau7802_set_channel(uint8_t channel);




//
// a calibration takes about 50 ms
//
static void nau_calibrate() {
    nau_writebit(NAU7802_CTRL2, NAU7802_CALS);                  // Begin calibration
    nau_readuntilfalsedelay(NAU7802_CTRL2, NAU7802_CALS, 10);   // Wait for calibration to finish
}

//
// NAU7802
//
static bool nau7802_init(void) {

    // EDIT THIS HERE FOR SM/MAXI TYPE
    uint16_t timeout = 0;
    _wire->begin(I2C_SDA_IO, I2C_SCL_IO, (uint32_t) 100000);

    nau_read(0);                                      // dummy, to clear I2C

    nau_writebit(NAU7802_PU_CTRL, NAU7802_RR);        // Reset Registers
    nau_clearbit(NAU7802_PU_CTRL, NAU7802_RR);        // Clear Reset Registers

    nau_writebit(NAU7802_PU_CTRL, NAU7802_PUD);       // Power up digital
    nau_readuntiltrue(NAU7802_PU_CTRL, NAU7802_PUR);  // Wait until power up
    nau_writebit(NAU7802_PU_CTRL, NAU7802_PUA);       // Power up analog

    nau_writebit(NAU7802_ADC_REG, 4);                 // Disable chopper function
    nau_writebit(NAU7802_ADC_REG, 5);                 // Disable chopper function
    nau_writebit(NAU7802_ADC_REG, 3);                 // Set VCM bit 1

    nau_writebit(NAU7802_PGA_REG, 0);                 // Disable chopper function
    nau_writebit(NAU7802_PGA_REG, 4);                 // Bypass PGA

    nau_writebit(NAU7802_CTRL2, 4);                   // configure for 320 SPS
    nau_writebit(NAU7802_CTRL2, 5);                   // configure for 320 SPS
    //nau_writebit(NAU7802_CTRL2, 6);                   // configure for 320 SPS

    nau_calibrate();                                  // Calibrate

    nau_writebit(NAU7802_PU_CTRL,   NAU7802_CS);      // Start New Conversion

    return true;
}


static void nau7802_set_channel(uint8_t channel) {
    if (channel == ADC_CHANNEL_V)
        nau_clearbit(NAU7802_CTRL2, 7);
    else
        nau_writebit(NAU7802_CTRL2, 7);
      delay(15); // we get unstable data without this delay
    nau_calibrate();
}

//
// takes about 40-50 ms
//
static uint32_t nau7802_read_data(void) {
  delay(15); // we get unstable data without this delay
  nau_readuntiltruedelay(NAU7802_PU_CTRL,NAU7802_CR,10);  // Wait for Conversion
  delay(15); // we get unstable data without this delay
  uint32_t adcVal = nau_read24(NAU7802_ADC_B2);           // Get data
  delay(15); // we get unstable data without this delay
  nau_writebit(NAU7802_PU_CTRL,   NAU7802_CS);            // Start New Conversion
  delay(15); // we get unstable data without this delay
    return adcVal;
}

uint32_t nau7802_get_data(int channel) {
    nau7802_set_channel(channel);
    delay(15); // we get unstable data without this delay
    return nau7802_read_data();
}


/******************************************************************************/

//
// initialize ADC
//
void adc_init(void) {
    nau7802_init();
    Serial.printf("Calibration gain  : %3.6f\r\n", calibration_data.calibration_gain);
    Serial.printf("Calibration offset: %3.6f\r\n\r\n", calibration_data.calibration_offset);
}


//
// calculate and return voltage
//
float adc_get_voltage(void) {
    float v;
    if (adc_activated) {
    uint32_t raw = nau7802_get_data(ADC_CHANNEL_V);
    
    if (hdwr_ver == 1 ){ // SwitchMaster Maxi
      v = (VREF_MAXI+diodeoffset) * (float) raw / ADC_STEPS;
    } else { // it's not a maxi,
      v = (VREF_SM+diodeoffset) * (float) raw / ADC_STEPS;
    }
    // resistor divider on main board 33k/7.55k
    v = v * 40.5 / 7.5;  // 33k / 7.5k
    // compensate with calibration data
    v = (v * calibration_data.calibration_gain) - calibration_data.calibration_offset;

    if (enablepowerdebug == 1) {
        Serial.printf("Raw ADC Voltage : %8d (0x%08x) --> ", raw, raw);
        Serial.printf("Voltage: %2.2f V\r\n", v);
    }
    } else {
      v = 13.1;
    }
    return v;
}

//
// calculate and return current
//
float adc_get_current(void) {
    uint32_t raw;
    float a;
  if (adc_activated){ 
    raw = nau7802_get_data(ADC_CHANNEL_A);
    a = raw * 133.3 / ADC_STEPS;  // 37.5 mV/A
    if (enablepowerdebug == 1) {
        Serial.printf("Raw ADC Current : %8d (0x%08x) --> ", raw, raw);
        Serial.printf("Current: %2.1f A\r\n\r\n", a);
    }
  } else {
    a = 5;
  }
    return a;
}