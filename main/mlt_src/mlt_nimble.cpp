#include <Arduino.h>

#include <NimBLEDevice.h>

#include "mlt_eeprom.h"
#include "mlt_output.h"
#include "mlt_adc.h"

#include "mlt_temp.h"
#include "mlt_defs.h"
#include "mlt_config.h"
#include "mlt_nimble.h"
#include "mlt_nvs.h"

static uint8_t my_mac[6];

static const char * ble_ssid;
static uint32_t ble_pin;
static uint32_t rxpin;

extern bool enableusb;
extern uint16_t  ch_cutout[MAX_CHANNELS];  // list of undervoltage cutout level, in mV for a channel. 0 means ignore
extern uint16_t  ch_timeout[MAX_CHANNELS]; // list of timeout counts, ticked every second. 0 = inactive
extern int ch_temps[3]; // list of temperatures. -127degC is a inserted for a null read. Multiplied by 100.

extern bool usbchangeflag;
extern bool tswchangeflag;
extern bool swvoltchangeflag;
extern uint16_t v_adc; // measured voltage in mV
extern uint16_t a_adc; // measured voltage in mA

bool is_mac_valid(uint8_t *pmac) {
  bool valid = true;
  for (int i=0;i<6;i++) {
    if (pmac[i] != my_mac[i]) {
      valid = false;
      if (enablemacdebug == 1) {
      Serial.printf("Mac comparison failure at %d, my_mac[%d] = %02x, pmac[%d] = %02x\r\n", i, i, my_mac[i], i,  pmac[i]);
      }
    }
  }
    if (enablemacdebug == 1) {
      Serial.println("MAC Comparison is good");
      }
    return true;
}

//*****************************************************************************


// DECLARE NIMBLE SERVER INFORMATION
NimBLEServer* pServer = NULL;
NimBLEService *pService = NULL;

// Tx Characteristics
NimBLECharacteristic *pMetricsCharacteristic = NULL;
NimBLECharacteristic *pChInfoCharacteristic = NULL;

// Tx and RX
NimBLECharacteristic *pDevConfigCharacteristic = NULL;
NimBLECharacteristic *pSwConfigCharacteristic = NULL;
NimBLECharacteristic *pTempsCharacteristic = NULL;

// Rx Characteristics
NimBLECharacteristic *pSetSwitchCharacteristic = NULL;
NimBLECharacteristic *pTimersCharacteristic = NULL;
NimBLECharacteristic *pLocksCharacteristic = NULL;

NimBLECharacteristic *pSmartWatchCharacteristic = NULL;

// Onward through BLE Stack Requirements
NimBLEAdvertising *pAdvertising = NULL;





//*****************************************************************************




//*****************************************************************************

// NIMBLE SERVER CALLBACKS

/**  None of these are required as they will be handled by the library with defaults. **
 **                       Remove as you see fit for your needs                        */
class ServerCallbacks: public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* pServer) {
      if (enabledebug == 1) {
      Serial.println("Client connected");
      Serial.println("Multi-connect support: start advertising");
      }
      NimBLEDevice::startAdvertising();
    };
    /** Alternative onConnect() method to extract details of the connection.
        See: src/ble_gap.h for the details of the ble_gap_conn_desc struct.
    */
    void onConnect(NimBLEServer* pServer, ble_gap_conn_desc* desc) {
       if (enabledebug == 1) {
      Serial.print("Client connected, address: ");
      Serial.println(NimBLEAddress(desc->peer_ota_addr).toString().c_str());
       }
      
      /** We can use the connection handle here to ask for different connection parameters.
          Args: connection handle, min connection interval, max connection interval
          latency, supervision timeout.
          Units; Min/Max Intervals: 1.25 millisecond increments.
          Latency: number of intervals allowed to skip.
          Timeout: 10 millisecond increments, try for 5x interval time for best results.
      */
      pServer->updateConnParams(desc->conn_handle, 24, 48, 0, 60);
    };

    void onDisconnect(NimBLEServer* pServer) {
      if (enabledebug == 1) {
      Serial.println("Client disconnected - start advertising");
      }
      NimBLEDevice::startAdvertising();
    };

    /********************* Security handled here **********************
    ****** Note: these are the same return values as defaults ********/
    uint32_t onPassKeyRequest() {
      if (enabledebug == 1) {
      Serial.println("Server Passkey Request");
      }
      /** This should return a random 6 digit number for security
          or make your own static passkey as done here.
      */
      return ble_pin;
    };

    void onAuthenticationComplete(ble_gap_conn_desc* desc) {
      /** Check that encryption was successful, if not we disconnect the client */
      if (!desc->sec_state.encrypted) {
        NimBLEDevice::getServer()->disconnect(desc->conn_handle);
        if (enabledebug == 1) {
        Serial.println("Encrypt connection failed - disconnecting client");
        }
        return;
      }
      if (enabledebug == 1) {
      Serial.println("Secure Connection Started");
      }
    };
};


//*****************************************************************************

// NIMBLE CHARACTERISTIC CALLBACKS

// This is OLD data callback

class MetricCharacteristicCallbacks: public NimBLECharacteristicCallbacks {
    /** Called before notification or indication is sent,
        the value can be changed here before sending if desired.
    */
    void onNotify(NimBLECharacteristic* pCharacteristic) {
      if (enabledebug == 1) {
          Serial.println("Sending notification to clients");
        }
    };

    /** The status returned in status is defined in NimBLECharacteristic.h.
        The value returned in code is the NimBLE host return code.
    */
    void onStatus(NimBLECharacteristic* pCharacteristic, Status status, int code) {
      String str = ("Notification/Indication status code: ");
      str += status;
      str += ", return code: ";
      str += code;
      str += ", ";
      str += NimBLEUtils::returnCodeToString(code);
      if (enabledebug == 1) {
      Serial.println(str);
      }
    };

    void onSubscribe(NimBLECharacteristic* pCharacteristic, ble_gap_conn_desc* desc, uint16_t subValue) {
      String str = "Client ID: ";
      str += desc->conn_handle;
      str += " Address: ";
      str += std::string(NimBLEAddress(desc->peer_ota_addr)).c_str();
      if (subValue == 0) {
        str += " Unsubscribed to ";
      } else if (subValue == 1) {
        str += " Subscribed to notifications for ";
      } else if (subValue == 2) {
        str += " Subscribed to indications for ";
      } else if (subValue == 3) {
        str += " Subscribed to notifications and indications for ";
      }
      str += std::string(pCharacteristic->getUUID()).c_str();
      if (enabledebug == 1) {
        Serial.println(str);
      }
    };


    void onWrite(NimBLECharacteristic *pCharacteristic) {
      nimble_set_t set = pSetSwitchCharacteristic->getValue <nimble_set_t> ();
      if (enabledebug == 1) {
      Serial.printf("VALUE AS RECEIVED: 0x%04x\r\n", set.setting);
      }
        uint16_t value = set.setting;
        if (enabledebug == 1) {
        Serial.printf("VALUE AS RECEIVED: 0x%04x\r\n", value);
        }
        for (int i=0;i<NUM_OUTPUTS;i++) {
          int state = (value & (1<<i)) != 0;
          if (enabledebug == 1) {
          Serial.printf("NimBLE: Turning %s Output %d\r\n", state ? "on " : "off", i+1);
          }
          if (state)
            output_set(i);
          else
            output_clear(i);
      }

      if (set.usb != enableusb) {
          if (set.usb == 1) {
            enableusb = true;
          } else {
            enableusb = false;
          }
      usbchangeflag = true;
      }
      
    }
};


// THIS IS THE CHANNEL INFORMATION CALLBACKS
class ChInfoCharacteristicCallbacks: public NimBLECharacteristicCallbacks {

    void onRead(NimBLECharacteristic *pCharacteristic) {
      if (enabledebug == 1) {
        Serial.println("Switch Channel info sent successfully");
      }
    }

    /** The status returned in status is defined in NimBLECharacteristic.h.
        The value returned in code is the NimBLE host return code.
    */
    void onStatus(NimBLECharacteristic* pCharacteristic, Status status, int code) {
      String str = ("Notification/Indication status code: ");
      str += status;
      str += ", return code: ";
      str += code;
      str += ", ";
      str += NimBLEUtils::returnCodeToString(code);
      if (enabledebug == 1) {
      Serial.println(str);
      }
    };

    void onSubscribe(NimBLECharacteristic* pCharacteristic, ble_gap_conn_desc* desc, uint16_t subValue) {
      String str = "Client ID: ";
      str += desc->conn_handle;
      str += " Address: ";
      str += std::string(NimBLEAddress(desc->peer_ota_addr)).c_str();
      if (subValue == 0) {
        str += " Unsubscribed to ";
      } else if (subValue == 1) {
        str += " Subscribed to notifications for ";
      } else if (subValue == 2) {
        str += " Subscribed to indications for ";
      } else if (subValue == 3) {
        str += " Subscribed to notifications and indications for ";
      }
      str += std::string(pCharacteristic->getUUID()).c_str();
      Serial.println(str);
    };

};
//*****************************************************************************

// Configuration Characteristic CALLBACKS
//

//  GLOBAL DEVICE CONFIGURATION
class DevConfigCharacteristicCallbacks: public NimBLECharacteristicCallbacks {

    void onWrite(NimBLECharacteristic *pCharacteristic) {
      nimble_dev_cfg_t cfg = pDevConfigCharacteristic->getValue <nimble_dev_cfg_t> ();
            // Serial.println("\nReceived value for NVS CONFIG");
            // Serial.print("SSID: ");
            // Serial.println((char*)cfg.ssid);
            // Serial.print("PASS: ");
            // Serial.println((char*)cfg.pass);
            // Serial.print("PIN: ");
            // Serial.println(cfg.pin);
            // Serial.print("Lowcut: ");
            // Serial.println(cfg.lowcut);
            // Serial.printf("Global Lowtemp cutout: ");
            // Serial.println(cfg.gbltempcutout);
            // Serial.printf("\n");
            

        //Serial.println("Writing values to NVS CONFIG");
        strncpy((char *)config_new.ssid, (char*) cfg.ssid, 20); // can use 19 bytes of 20 (last byte is null or 0 termination)
        config_new.ssid[20] = 0;   // zero terminate
        strncpy((char *)config_new.pass, (char*) cfg.pass, 32); // can use 31 bytes of 32 (last byte is null or 0 termination)
        config_new.pass[32] = 0;   // zero terminate
        config_new.pin = cfg.pin;
        config_new.lowcut = cfg.lowcut;

        if (enabledebug == 1) {
            Serial.print("SSID: ");
            Serial.println((char*)config_new.ssid);
            Serial.print("PASS: ");
            Serial.println((char*)config_new.pass);
            Serial.print("PIN: ");
            Serial.println(config_new.pin);
            Serial.printf("\n");
            Serial.print("Lowcut: ");
            Serial.println(config_new.lowcut);
            Serial.printf("\n");
            Serial.print("Global Temp Cutout (°C * 100): ");
        }
        config_save();
        
        // Writing global temp cutout in NVS
        mlt_nvs_write_onboard_cutout_temp(cfg.gbltempcutout); 
        
        if (enabledebug == 1) {
            Serial.println("-- BLE Global Device Config Written--\r\n");
        }
    }

      void onRead(NimBLECharacteristic *pCharacteristic) {
          nimble_dev_cfg_t nimble_cfg_read = {0};                      // clear all to zero
          config_new_t nimble_read_gbl_data  = {0};                    // clear all to zero

          mlt_nvs_read_config_data(&nimble_read_gbl_data);

          // Read global temp cutout from NVS
          uint16_t gbltempcutoutread;
          mlt_nvs_read_onboard_cutout_temp(&gbltempcutoutread );
          const uint8_t *padr = NimBLEDevice::getAddress().getNative(); // get device address as bytes
          for (int i=0;i<6;i++) {                                   // loop through all 6
          nimble_cfg_read.ble_adr[i] = padr[i];                            // and stick data into structure
          }

          strncpy((char *)nimble_cfg_read.ssid, (char*) nimble_read_gbl_data.ssid, 20);
          strncpy((char *)nimble_cfg_read.pass, (char*) nimble_read_gbl_data.pass, 32);
          nimble_cfg_read.pin = nimble_read_gbl_data.pin;
          nimble_cfg_read.lowcut = nimble_read_gbl_data.lowcut;
          
          nimble_cfg_read.gbltempcutout = gbltempcutoutread;
          
            // Serial.println("Global Device Config Read");
            // Serial.print("SSID: ");+
            // Serial.println((char*)nimble_read_gbl_data.ssid);
            // Serial.print("PASS: ");
            // Serial.println((char*)nimble_read_gbl_data.pass);
            // Serial.print("PIN: ");
            // Serial.println(nimble_read_gbl_data.pin);
            // Serial.print("Lowcut: ");
            // Serial.println(nimble_read_gbl_data.lowcut);
            // Serial.print("Global Temp Cutout (°C * 100): ");
            // Serial.println(nimble_cfg_read.gbltempcutout);
            // Serial.printf("\n");        

          pDevConfigCharacteristic->setValue <nimble_dev_cfg_t> (nimble_cfg_read);  // update value
          // Serial.println("-- NIMBLE Read Global Device Config--");      
    }
};

//  SWITCH CONFIGURATIONS DETAILS
class SwConfigCharacteristicCallbacks: public NimBLECharacteristicCallbacks {

//   // Switch config structure for read and write
// typedef struct nimble_sw_cfg_t {
//   uint8_t   mode;                   // Tells the characteristic what to do. 0 = read the received switch # from NVS. 1 = store received switch info to NVS.
//   uint8_t   switch_number;          // Switch Number for configuration
//   uint8_t   switch_name[20];        // Switch name
//   uint16_t  voltage_cutin;          // Voltage in mV for when this switch should cut in. (mV: 12.5v = 12500). 0 = disabled.
//   uint16_t  voltage_cutout;         // Voltage in mV for when this switch should cut out. (mV: 12.5v = 12500). 0 = disabled.
// }nimble_sw_cfg_t;
    void onRead(NimBLECharacteristic *pCharacteristic) {
       // No read as it is wrapped up in the onWrite (using mode)
    }

    void onWrite(NimBLECharacteristic *pCharacteristic) {
      nimble_sw_cfg_t rxswdata = pSwConfigCharacteristic->getValue <nimble_sw_cfg_t> ();
      nimble_sw_data_t switch_read = {0};

      if (rxswdata.mode == 0) { 
        mlt_nvs_read_switch_config(rxswdata.switch_number, &switch_read);
          // Serial.printf( "Received read request\n");
          // Serial.printf( "RX Mode : %d\n", rxswdata.mode);
          // Serial.printf( "Switch number : %d\n", rxswdata.switch_number);

          // Serial.printf( "Reading from NVS for switch\n");
          // Serial.printf( "Array number : %d\n", rxswdata.switch_number);
          // Serial.printf( "Switch number : %d\n", switch_read.switch_number );
          // Serial.printf( "Switch name   : %s\n", switch_read.switch_name );
          // Serial.printf( "Switch cutin  : %d\n", switch_read.voltage_cutin );
          // Serial.printf( "Switch cutout : %d\n", switch_read.voltage_cutout );

          pSwConfigCharacteristic->setValue <nimble_sw_data_t> (switch_read); // update value
          pSwConfigCharacteristic->notify();  // and tell other end
        // } else {
        // Serial.printf( "Failed reading switch (%d) config\n", switch_read);
        // }

      } else {
        nimble_sw_data_t switch_write = {0};
        
        // Store the name
        snprintf( (char *)&switch_write.switch_name, 14, "%s", rxswdata.switch_name );
        
        // Store other values
        switch_write.switch_number = rxswdata.switch_number;
        switch_write.voltage_cutin = rxswdata.voltage_cutin;
        switch_write.voltage_cutout = rxswdata.voltage_cutout;

        // Show what we're saving
          // Serial.printf( "About to write NVS for switch\n");
          // Serial.printf( "Switch number : %d\n", switch_write.switch_number );
          // Serial.printf( "Switch name   : %s\n", switch_write.switch_name );
          // Serial.printf( "Switch cutin  : %d\n", switch_write.voltage_cutin );
          // Serial.printf( "Switch cutout : %d\n", switch_write.voltage_cutout );
        

        // Send it through to get saved
        if (true == mlt_nvs_write_switch_config(&switch_write)) {
          //Serial.printf( "Succeed writing switch (%d) config\n", switch_write.switch_number );
          nvschangeflag = true;
        } else {
          // Serial.printf( "Failed writing switch (%d) config\n", switch_write.switch_number );
        }      
      }
    }
};

// THIS IS THE SWITCH TIMER CHARACTERISTIC
class TimersCharacteristicCallbacks: public NimBLECharacteristicCallbacks {

    void onWrite(NimBLECharacteristic *pCharacteristic) {
      nimble_timeouts_t cfg = pTimersCharacteristic->getValue <nimble_timeouts_t> ();
     
          

          for (int i=0;i<MAX_CHANNELS;i++) {
              ch_timeout[i] = cfg.ch_timeouts[i];
              if (enabletimerdebug == 1) {
                Serial.println("Writing switch timers");
                Serial.printf("Channel: %2d -> Timeout = %d\r\n", i, ch_timeout[i]);
              }
          }
    }

    void onRead(NimBLECharacteristic *pCharacteristic) {
          nimble_timeouts_t nimble_timeouts = {0};  // clear all to zero

          for (int i=0;i<MAX_CHANNELS;i++) {
            nimble_timeouts.ch_timeouts[i] = ch_timeout[i];
          }
          pTimersCharacteristic->setValue <nimble_timeouts_t> (nimble_timeouts);  // update value
          // Serial.println("Reading switch timers");
    }
    };


// THIS IS THE CHANNEL LOCKOUT CHARACTERISTIC
class LockoutCharacteristicCallbacks: public NimBLECharacteristicCallbacks {

    void onWrite(NimBLECharacteristic *pCharacteristic) {
      nimble_lockout_t cfg = pLocksCharacteristic->getValue <nimble_lockout_t> ();
      
          

          for (int i=0;i<MAX_CHANNELS;i++) {
              ch_lockout[i] = cfg.ch_lockout[i];
              if (enablelockoutdebug == 1) {
                Serial.println("Writing channel lockout configs");
                Serial.printf("Channel: %2d -> Lockout = %d\r\n", i, ch_lockout[i]);
              }
          }
    }

        void onRead(NimBLECharacteristic *pCharacteristic) {
          nimble_lockout_t nimble_lockout = {0};                      // clear all to zero

          for (int i=0;i<MAX_CHANNELS;i++) {
            nimble_lockout.ch_lockout[i] = ch_lockout[i];
          }
          pLocksCharacteristic->setValue <nimble_lockout_t> (nimble_lockout);  // update value
          pLocksCharacteristic->notify();

          if (enablelockoutdebug == 1) {
            Serial.println("-- NIMBLE NOTIFY Global Lockout Status --");
          }
         
    }

};

// THIS IS THE TEMPERATURE CHARACTERISTIC
class TempCharacteristicCallbacks: public NimBLECharacteristicCallbacks {

    void onRead(NimBLECharacteristic *pCharacteristic) {
      // No read as it is wrapped up in the onWrite (using mode)
    }

    void onWrite(NimBLECharacteristic *pCharacteristic) {  
      // nimble_temp_cfg_t rxtempdata = pTempsCharacteristic->getValue <nimble_temp_cfg_t> ();
      nimble_temp_cfg_t rxdata = pTempsCharacteristic->getValue <nimble_temp_cfg_t> ();
      
      nimble_temp_data_t temp_read = {0};


      // Serial.printf("TEMP CHARACTERISTICS AS RECEIVED 0x%04x\r\n", rxdata);
      
      if (rxdata.mode == 0) {
        if ( true == mlt_nvs_read_temp_sw_data(rxdata.ch_temp_switch, &temp_read) ) {
          // Serial.printf( "\nReceived Temp Read Request\n");
          // Serial.printf( "Temp Switch: %d\n", temp_read.ch_temp_switch );
          // Serial.printf( "Switch condition : %d\n", temp_read.switching_condition);
          // Serial.printf( "Switching value  : %d\n", temp_read.switching_value );
          // Serial.printf( "Output Number  : %d\n", temp_read.switch_on);

          pTempsCharacteristic->setValue <nimble_temp_data_t> (temp_read);    // update value
          pTempsCharacteristic->notify();                                // and tell other end (will always be OPEN service/characteristic
        } else {
          // Serial.printf( "Failed reading switch (%d) config\n", temp_read.ch_temp_switch);
        }
      } else {
        tswchangeflag = true;
        nimble_temp_data_t temp_write = {0};
          // Serial.printf( "Received Write Request: %d\n", temp_read.ch_temp_switch );
        
          // Serial.printf( "\nWrote Temp Read Request\n");
          // Serial.printf( "Temp Switch: %d\n", rxdata.ch_temp_switch );
          // Serial.printf( "Switch condition : %d\n", rxdata.switching_condition);
          // Serial.printf( "Switching value  : %d\n", rxdata.switching_value );
          // Serial.printf( "Output Number  : %d\n", rxdata.switch_on);

        uint16_t temp_rxd; // Temp received in K * 100
        uint16_t temp_celsius;
        temp_rxd = rxdata.switching_value;
        temp_celsius = (temp_rxd - 27300) / 100;
        temp_write.ch_temp_switch = rxdata.ch_temp_switch;
        temp_write.switching_condition = rxdata.switching_condition;
        temp_write.switching_value = rxdata.switching_value;
        temp_write.switch_on = rxdata.switch_on;

        
        
        // Send it through to the NVS subsystem
        if (true == mlt_nvs_write_temp_sw_data(&temp_write)) {
          // Serial.printf( "Succeed writing switch (%d) config\n", temp_write.ch_temp_switch );
          nvschangeflag = true;

          // Notify others it's successful
          pTempsCharacteristic->setValue <nimble_temp_data_t> (temp_write);    // update value
          pTempsCharacteristic->notify();                                // and tell other end (will always be OPEN service/characteristic
        } else {
          // Serial.printf( "Failed writing switch (%d) config\n", temp_write.ch_temp_switch );
        }        
      }
    }
};

// THIS IS THE SmartWatch/NON-PAIRING CHARACTERISTIC
class SmartWatchCharacteristicCallbacks: public NimBLECharacteristicCallbacks {
    
    void onWrite(NimBLECharacteristic *pCharacteristic) {

      nimble_smartwatch_t set = pSmartWatchCharacteristic->getValue <nimble_smartwatch_t> ();
      // if (enabledebug == 1) {
      Serial.printf("SMARTWATCH SETTINGS AS RECEIVED: 0x%04x\r\n", set.setting);
      // }
      uint32_t testpin = set.rxpin;
      Serial.println("Testing Manual PIN");
      Serial.print("EEPROM PIN: ");
      Serial.println(ble_pin);
      Serial.print("RX PIN: ");
      Serial.println(set.rxpin);
      
      Serial.println("\nStart manual PIN test");
      if (set.rxpin == ble_pin) {
        uint16_t value = set.setting;
        // if (enabledebug == 1) {
        Serial.println("CORRECT PIN. Proceeding...");
        Serial.printf("SMARTWATCH VALUE AS RECEIVED: 0x%04x\r\n", value);
        
        for (int i=0;i<NUM_OUTPUTS;i++) {
          int state = (value & (1<<i)) != 0;
          // if (enabledebug == 1) {
          Serial.printf("NimBLE: Turning %s Output %d\r\n", state ? "on " : "off", i+1);
          // }
          if (state)
            output_set(i);
          else
            output_clear(i);
        }
      } else {
        Serial.println("PIN is INCORRECT");
     }
      //nimble_buildoutput();
    }
};

//*****************************************************************************




void nimble_init(const char * ssid, uint32_t pin) {

    ble_ssid = ssid;
    ble_pin = pin;

  // Initialize NimBLE device instance and set a default power level
  Serial.println("Starting NimBLE Server");
  NimBLEDevice::init(ble_ssid);
  NimBLEDevice::setPower(ESP_PWR_LVL_P9);

  // Display NimBLE MAC
  std::string bleAddress = NimBLEDevice::toString();
  Serial.print("BT MAC Address: ");
  Serial.println(bleAddress.c_str());

  // save mac for use in validation later
  const uint8_t *pmac = NimBLEDevice::getAddress().getNative();
  for (int i=0;i<6;i++) {
    my_mac[i] = pmac[i];
  }


  // Setup security options on this NimBLE device
  NimBLEDevice::setSecurityIOCap(BLE_HS_IO_DISPLAY_ONLY);
  NimBLEDevice::setSecurityAuth(true, true, true);
  NimBLEDevice::setSecurityPasskey(ble_pin);

  // Create NimBLE server and setup callbacks
  pServer = NimBLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks);

  // Create our service
  if (hdwr_ver == 0 ) {
    Serial.print("Detected SwitchMaster - UUID Set: ");
    Serial.println(SM_UUID);
    pService = pServer->createService(SM_UUID);
  } else if ( hdwr_ver == 1) {
    Serial.print("Detected SwitchMaster MAXI - UUID Set: ");
    Serial.println(SM_MAXI_UUID);
    pService = pServer->createService(SM_MAXI_UUID);
  } else if ( hdwr_ver  == 2) {
    Serial.print("Detected Test Hardware - UUID Set: ");
    Serial.println(SM_TEST_VENDOR_UUID);
    pService = pServer->createService(SM_TEST_VENDOR_UUID);
  } else {
    Serial.println("Incorrect hardware ID found - resetting to default SwitchMaster - ID # 0. Please reset the board now.");
    mlt_nvs_write_hardware_version(0);
    pService = pServer->createService(SM_UUID);
  }

  // Create the OPEN NimBLE characteristic and setup callbacks for switch status, voltage and current and current stats
  pMetricsCharacteristic = pService->createCharacteristic(METRICS_CHARACTERISTIC_UUID, NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::READ);
  pMetricsCharacteristic->setValue(0);
  pMetricsCharacteristic->setCallbacks(new MetricCharacteristicCallbacks);

  // Create SECURE NimBLE characteristic and setup callbacks for device details config
  pDevConfigCharacteristic = pService->createCharacteristic(DEV_CONFIG_CHARACTERISTIC_UUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::READ_ENC | NIMBLE_PROPERTY::READ_AUTHEN | NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_ENC | NIMBLE_PROPERTY::WRITE_AUTHOR);
  //  pDevConfigCharacteristic->setValue(0); // we need this, even though this characteristic cannot be read, otherwise the stack with throw up
  pDevConfigCharacteristic->setCallbacks(new DevConfigCharacteristicCallbacks);

    // Create SECURE NimBLE characteristic and setup callbacks for switch details config
  pSwConfigCharacteristic = pService->createCharacteristic(SW_CONFIG_CHARACTERISTIC_UUID, NIMBLE_PROPERTY::NOTIFY |NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_ENC | NIMBLE_PROPERTY::WRITE_AUTHOR);
  //  pSwConfigCharacteristic->setValue(0); // we need this, even though this characteristic cannot be read, otherwise the stack with throw up
  pSwConfigCharacteristic->setCallbacks(new SwConfigCharacteristicCallbacks);

  // SECURE - Switch on/off on Metric Characteristic
  pSetSwitchCharacteristic = pService->createCharacteristic(SET_SWITCHES_CHARACTERISTIC_UUID, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_ENC | NIMBLE_PROPERTY::WRITE_AUTHOR);
  pSetSwitchCharacteristic->setCallbacks(new MetricCharacteristicCallbacks);

  // SECURE - Switch timers characteristic
  pTimersCharacteristic = pService->createCharacteristic(TIMERS_CHARACTERISTIC_UUID, NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_ENC | NIMBLE_PROPERTY::WRITE_AUTHOR);
  pTimersCharacteristic->setCallbacks(new TimersCharacteristicCallbacks);

  // SECURE - Switch lockout characteristic
  pLocksCharacteristic = pService->createCharacteristic(LOCKS_CHARACTERISTIC_UUID, NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_ENC | NIMBLE_PROPERTY::WRITE_AUTHOR);
  pLocksCharacteristic->setCallbacks(new LockoutCharacteristicCallbacks);

  // SECURE - Temperature characteristics
  pTempsCharacteristic = pService->createCharacteristic(TEMPS_CHARACTERISTIC_UUID, NIMBLE_PROPERTY::NOTIFY |NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_ENC | NIMBLE_PROPERTY::WRITE_AUTHOR);
  //   pTempsCharacteristic->setValue(0); // we need this, even though this characteristic cannot be read, otherwise the stack with throw up
  pTempsCharacteristic->setCallbacks(new TempCharacteristicCallbacks);

  // Create the OPEN NimBLE characteristic to allow writing from smartwatches
  pSmartWatchCharacteristic = pService->createCharacteristic(SMARTWATCH_CHARACTERISTIC_UUID, NIMBLE_PROPERTY::WRITE);
  pSmartWatchCharacteristic->setCallbacks(new SmartWatchCharacteristicCallbacks);
if (pSmartWatchCharacteristic) {
    Serial.println("SmartWatch characteristic created OK");
    Serial.println(SMARTWATCH_CHARACTERISTIC_UUID);
} else {
    Serial.println("SmartWatch characteristic creation FAILED");
}

  
  // Start the service
  pService->start();

  // Setup and start advertising
  pAdvertising = NimBLEDevice::getAdvertising();

  // Find which service to advertise
  if (vendor_id == 0 ) {
    Serial.print("SM Detected - Advertising: ");
    Serial.println(SM_UUID);
    pAdvertising->addServiceUUID(SM_UUID);
  } else if ( vendor_id == 1) {
    Serial.print("SM MAXI Detected - Advertising: ");
    Serial.println(SM_MAXI_UUID);
    pAdvertising->addServiceUUID(SM_MAXI_UUID);
  } else if ( vendor_id == 2) {
    Serial.print("Test Vendor Detected - Advertising: ");
    Serial.println(SM_TEST_VENDOR_UUID);
    pAdvertising->addServiceUUID(SM_TEST_VENDOR_UUID);
  } else {
    Serial.println("Incorrect hardware ID found. Please reset the board now.");
  }

  pAdvertising->setScanResponse(true);
  pAdvertising->start();

  // Make sure there will be valid data to read right from the start
  nimble_build_output(0,0,0);
  
}


//*****************************************************************************

//
// fill structure with data and indicate to other party
//
void nimble_build_output(float lasthour, float last12hours, uint8_t override_active) {
  nimble_metrics_t nimble_metrics_out = {0};                                // clear all to zero

  for (int i=0;i<NUM_OUTPUTS;i++)  {// loop through outputs
      nimble_metrics_out.outputs |= (output_status(i) << i);   // set bits for HIGH outputs
  }                        
    
    nimble_metrics_out.usb = enableusb;                      // usb on or off. 0 = off, 1 = on 
    nimble_metrics_out.voltage = round(v_adc);               // add voltage data
    nimble_metrics_out.current = round(a_adc);               // and current data

    nimble_metrics_out.last_hour = round(lasthour * 100);    // Add average current for last hour
    nimble_metrics_out.last_12 = round(last12hours  * 100);  // Add average currents for last 12 hours
    nimble_metrics_out.override_active = override_active;

    // Loop through the temperature sensors
      for (int i=0;i<3;i++) {
        nimble_metrics_out.ch_temps[i] = (ch_temps[i] + 27300); // We need to add 273 (to convert to Kelvin scale, so we dont have a negative number)
        
        if(enabletempdebug) {
        // Loop through outputs and display temps
        for(int i=0;i<3; i++){
          Serial.print("ch_temp variable ");
          Serial.print(i,DEC);
          Serial.print(" raw reading is ");
          Serial.println(ch_temps[i]);
          float tempC = ch_temps[i] / 100.0;
          Serial.print("After conversion ");
          Serial.println(tempC);
          Serial.println();
          Serial.print("Onboard sensor temp: ");
          Serial.println(onboardTemp);
      }
      }
      
    
  }
    

    pMetricsCharacteristic->setValue <nimble_metrics_t> (nimble_metrics_out);     // update value
    pMetricsCharacteristic->notify();                                             // and tell other end (will always be OPEN service/characteristic

    if (enablepowerdebug == 1) {
        Serial.println("-- NIMBLE Build Outputs --");
        Serial.print("VOLTAGE - ADC: ");
        Serial.println(round(adc_get_voltage() * 100));
        Serial.print("CURRENT - ADC: ");
        Serial.println(round(adc_get_current() * 100));
        Serial.print("Transmitted Outputs - plain: ");
        Serial.println(nimble_metrics_out.outputs);
        Serial.print("VOLTAGE - plain: ");
        Serial.println(nimble_metrics_out.voltage);
        Serial.print("VOLTAGE  - HEX: ");
        Serial.println(nimble_metrics_out.voltage, HEX);
        Serial.print("CURRENT - plain: ");
        Serial.println(nimble_metrics_out.current);
        Serial.print("CURRENT - HEX: ");
        Serial.println(nimble_metrics_out.current, HEX);
        Serial.printf("\n");
        Serial.println("-- NIMBLE Send Currents --");
        Serial.print("CURRENT - Last Hour: ");
        Serial.println(round(nimble_metrics_out.last_hour * 100));
        Serial.print("CURRENT - Last 12 Hours: ");
        Serial.println(round(nimble_metrics_out.last_12 * 100));
        Serial.printf("\n");
    }
}

//*****************************************************************************

void nimble_build_timers() {
    nimble_timeouts_t nimble_timeouts = {0};                      // clear all to zero
    
   for (int i=0;i<MAX_CHANNELS;i++) {
      nimble_timeouts.ch_timeouts[i] = ch_timeout[i];
    }

    pTimersCharacteristic->setValue <nimble_timeouts_t> (nimble_timeouts);  // update value
    pTimersCharacteristic->notify();
}

void nimble_build_lockouts() {
    nimble_lockout_t nimble_lockouts = {0};                      // clear all to zero
    
   for (int i=0;i<MAX_CHANNELS;i++) {
     nimble_lockouts.ch_lockout[i] = ch_lockout[i];
    //  Serial.print("Status of Lockout #: ");
    //  Serial.print(i);
    //  Serial.print(" = ");
    //  Serial.println(ch_lockout[i]);
    }

    
    pLocksCharacteristic->setValue <nimble_lockout_t> (nimble_lockouts);  // update value
    pLocksCharacteristic->notify();
}