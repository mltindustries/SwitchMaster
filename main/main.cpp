#include "Arduino.h"
#include <esp_now.h>
#include <WiFi.h>

// My headers
#include "mlt_defs.h"
#include "mlt_adc.h"
#include "mlt_calibration.h"
#include "mlt_config.h"
#include "mlt_eeprom.h"
#include "mlt_output.h"
#include "mlt_led.h"
#include "mlt_monitor.h"
#include "mlt_nimble.h"
#include "mlt_temp.h"
#include "mlt_usb.h"
#include "mlt_nvs.h"

bool enabledebug = false;
bool enabletimerdebug = false;
bool enablepowerdebug = false;
bool enablelowcutdebug = false;
bool enablelockoutdebug = false;
bool enabletempdebug = false;
bool enablemacdebug = false;
bool lvusbactivated = false;
bool enableusb = true;
bool usbchangeflag = false;
bool tswchangeflag = false;
bool nvschangeflag = false;
bool globallvactive = false;
bool chlvactive = false;
bool demo_mode_activated;
bool adc_activated = 0;

// Define global variables
uint8_t hdwr_ver; // Stores the hardware version number
uint16_t vendor_id; // Stores the vendor ID number
uint8_t override_active;
uint16_t  ch_cutout[MAX_CHANNELS];  // list of undervoltage cutout level, in mV for a channel. 0 means ignore
uint16_t  ch_cutin[MAX_CHANNELS]; // list of cut-in level per switch, in mV, to auto-turn on a channel. 0 means ignore.
uint16_t  ch_timeout[MAX_CHANNELS]; // list of timeout counts, ticked every second. 0 = inactive
uint16_t  ch_lockout[MAX_CHANNELS]; // list of lockouts 0 = inactive, 1 = channel locked on mobile
uint16_t  ch_lvactive[MAX_CHANNELS]; // list of low voltages 0 = inactive, 1 = channel locked on mobile
uint16_t v_adc; // measured voltage in mV
uint16_t a_adc; // measured voltage in mA
int ch_temps[3]; // list of temps from each sensor

uint8_t mac_list[6][6] = {0};
uint8_t mac_added[6] = {0};
uint8_t targetAddress[6] = {};

int onboardTemp;
nimble_temp_data_t temp_switch[3] = {0};
espnow_control rxdControl;
espnow_data txdData;
esp_now_peer_info_t peerInfo;


int temp_chk_count = 0;
uint16_t gblcutoutTemp;

// ** 
// BEGIN FUNCTIONS!
//


// Function declaration
void switch_toutputs_on( uint16_t switch_settings );
void switch_toutputs_off( uint16_t switch_settings );

// ESPNOW DATA RECEIVED
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  int mac_added = 0;
  memcpy(&rxdControl, incomingData, sizeof(rxdControl));
  Serial.print("MSG Type: ");
  Serial.println(rxdControl.msgType);
  
  // Display MAC
  Serial.print("Received from MAC: ");
  for (int i=0;i<6;i++) {                                   // loop through all 6
    Serial.print(rxdControl.apmac[i],HEX);
  }
  
  
  Serial.print("\nOutput Number: ");
  Serial.println(rxdControl.output_num);
  Serial.print("Lockout Number: ");
  Serial.println(rxdControl.lockout_num);
  
  for (int i=0;i<3;i++) {
    if (mac_added == 0) {
        // Serial.println("Passed check 1");
        // Serial.print("MAC LIST/RXD: ");
        // Serial.print(mac_list[i][4]);
        // Serial.print("/");
        // Serial.println(rxdControl.apmac[4]);
        if (mac_list[i][4] == rxdControl.apmac[4]) {
          //Serial.println("This MAC already added, skipping...");
          mac_added = 1; // Terminate the loop
        } else if (mac_list[i][4] == 0 ) {
          //Serial.println("Adding MAC Address");
          for (int j=0;j<6;j++) {                     // loop through all 6
             mac_list[i][j] = rxdControl.apmac[j];     // and stick data into structure for Tx
          }
          //Serial.println("Wifi Station MAC initialised into structure");
          mac_added = 1;
        } else {
          //Serial.println("MAC is not 0 or already added");
        }
    }
    // Serial.println("");
    // Serial.print("List ");
    // Serial.print(i);
    // Serial.print(": ");
    for (int j=0;j<6;j++) { // loop through all 6
      Serial.print(mac_list[i][j],HEX);     // and stick data into structure for Tx
    }
    //Serial.println();
  }

  int rxLockNum = rxdControl.lockout_num;
  int rxOutputNum = rxdControl.output_num;

  // Control message incoming
  if (rxdControl.msgType == 1) {
    if (ch_lockout[rxOutputNum] == 0) {
      // Check the current status of the switch and toggle it to the opposite
      if (output_status(rxOutputNum) == 0 ){
        output_set(rxOutputNum);
        // Serial.print("ESPNow turning on output: ");
        // Serial.println(rxOutputNum);
      } else {
        // Serial.print("ESPNow turning off output: ");
        // Serial.println(rxOutputNum);
        output_clear(rxOutputNum);
    }
    } else {
      Serial.println("Lockout is enabled on this output.");
    }
      
  } else if (rxdControl.msgType == 2) {
    // Check the current lockout status of the switch and toggle it to the opposite
    if (ch_lockout[rxLockNum] == 0) {
      // Serial.print("ESPNow locking out: ");
      // Serial.println(rxLockNum);
      ch_lockout[rxLockNum] = 1;
    } else {
      // Serial.print("ESPNow unlocking out: ");
      // Serial.println(rxLockNum);
      ch_lockout[rxLockNum] = 0;
    }
  } if (rxdControl.msgType == 3) {
    if (ch_lockout[rxOutputNum] == 0) {
      // Check the current status of the switch and toggle it to the opposite
          output_clear(rxOutputNum);
      } else {
        Serial.println("Lockout is enabled on this output.");
      }
  }
} 


// Send message via ESP-NOW
void sendESPNowData() {
  uint8_t targetAddressList[6] = {};
  
  // Gather Data
  for (int i=0;i<MAX_CHANNELS;i++) {
     txdData.lockout_status[i] = ch_lockout[i];
     txdData.output_status |= (output_status(i) << i);   // set bits for HIGH outputs
  }  
  // Loop through and send to board that are registered
  for (int i=0;i<6;i++) {
    if (mac_list[i][4] != 0 ) {
      if (mac_added[i] == 0) {
        // Register the ESPNow peer address
        // Register peer
        memcpy(peerInfo.peer_addr, mac_list[i], 6);
        
        peerInfo.channel = 0;  
        peerInfo.encrypt = false;
        // Add peer        
        if (esp_now_add_peer(&peerInfo) != ESP_OK){
            //Serial.println("Failed to add peer");
            return;
        } else {
          Serial.println("Added peer");
        }
        mac_added[i] = 1; //Set added flag
      } else {
        //Serial.println("Already added MAC to peer list");
      }
      //Serial.print("Sending to board # ");
      memcpy(targetAddress, mac_list[i], 6);
      //Serial.println(i);
      esp_err_t result = esp_now_send(targetAddress, (uint8_t *) &txdData, sizeof(txdData));
    } else {
      // Serial.print("MAC LIST # ");
      // Serial.print(i);
      // Serial.print(" is null\n");
      // Serial.println("Sending test direct to board ");
      // esp_err_t result = esp_now_send(targetAddressList[i], (uint8_t *) &txdData, sizeof(txdData));
    }
  }
    
  }



  // callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
    //Serial.print("\r\nLast Packet Send Status:\t");
    //Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

//
// Check for an override
//
void check_overrides(void) {
    // read the state of the pushbutton value
  override_active = digitalRead(OVERRIDE_PIN);
}


//
// clear all timeouts
//
void clear_timeouts(void) {
    for (int i=0;i<MAX_CHANNELS;i++) {
        ch_timeout[i] = 0;
    }
}

//
// turn off output, if the timeout has expired
//
void check_timeouts(void) {
  for (int i=0;i<NUM_OUTPUTS;i++) { // loop through outputs
    if (ch_timeout[i] > 0) {   // if timeout in progress
            ch_timeout[i]--;         // decrement

      if (enabletimerdebug == 1) {
      Serial.print("Count for channel ");
      Serial.println(i);
      Serial.println(ch_timeout[i]);  
      }
      
      if (ch_timeout[i] == 0)  // if timeout is reached
        output_clear(i);            // turn off channel
    }
  }
}

//
// turn off outputs if undervoltage is seen
// handles both global and individual cutout
// values are compared with one decimal
//
void check_lowvoltagecutout(void) {
    // check global cutout
  if (v_adc < (config_new.lowcut)) {        // if voltage is lower than cutout
        // Set the flag as active
        globallvactive = true;
        led_set();
        // // Sort the rest of it
        // Serial.print("v_adc: ");
        // Serial.println(v_adc);
        // Serial.print("Global LV Cutout: ");
        // Serial.println(config_new.lowcut);
        // Serial.println("Global LV Cutout reached. Shutting down outputs and USB...");

        // Shutdown USBs
        usb_clear(USB_1);
        lvusbactivated = true;
        
        //Shutdown Outputs
        for (int i=0;i<NUM_OUTPUTS;i++)       // loop through outputs
          output_clear(i);                    // turn all off
      } else {
        //Turn the flag off
        globallvactive = false;
        led_clear();
        if (lvusbactivated == true){
        // Reactivate USB ports
        usb_set(USB_1);
        lvusbactivated = false; // reactivate USB ports
        // Serial.println("Global LV Cutout cleared. Turning on USB power and reactivating outputs.");
      }
  }
 
  // check individual shutoffs
  for (int i=0;i<NUM_OUTPUTS;i++) {       // loop through outputs
    if (ch_cutout[i] > 0) {               // if turnoff is active
      // Serial.printf("CUT-OUT on SWITCH %d is active!\n", i);
      if (v_adc < (ch_cutout[i])) {      // and undervoltage is seenCheck
        //Serial.printf("Voltage cut-out reached! Turning off %d!\n", i);
        ch_lvactive[i] = true; // Set the channels LV status to true
        output_clear(i);                  // turn output off
      } else {
        if (ch_lvactive[i] == true) {
          ch_lvactive[i] = false;
        }
      }
    }
  }


}

//
// turn on outputs if high voltages are seen
//
void check_cutins(void) {
  // check individual cutin
  // Serial.println("Checking CUT-INS");
  for (int i=0;i<NUM_OUTPUTS;i++) {       // loop through outputs
    if (ch_cutin[i] > 0) {               // if turn on is active
      // Serial.printf("CUT-IN on SWITCH %d is active!\n", i);
       if (v_adc > (ch_cutin[i])) {      // and voltage is above the set cut-in
        output_set(i);                  // turn output on!
        // Serial.printf("Voltage cut-in reached! Turning on %d!\n", i);
      }
    }
  }
}

void check_temp_conditions(void) {
  // Reset the counter to 0
  temp_chk_count = 0;

  // Check for active LV flags, skip the lot if on.
  if ( globallvactive != true) {
      // Check if the temp switch flags have been changed and reload if so
      if (nvschangeflag == true) {
        // Pull latest temperature switching info from NVS
        for (int i=0;i<3;i++) {
          mlt_nvs_read_temp_sw_data(i, &temp_switch[i]);
        }
        // Don't reset the flag, their may be more NVS data we need to read next loop.
      }
      
      for (int i=0;i<3;i++) {
      // Run 'GREATER THAN' Checks
      // Serial.printf("Temp Switch #%d ",i);
      // Serial.printf("\nStored value: %d", temp_switch[i].switching_value);
      // Serial.printf(" reading: %d\n", ((ch_temps[i] /100) + 273));


      if ( temp_switch[i].switching_condition == 1 ) {

        if ( ((ch_temps[i] /100) + 273) > temp_switch[i].switching_value ) {
          // Temp is higher, turn them on but check low voltage cutout isnt activated.
          
          if (v_adc > ch_cutout[temp_switch[i].switch_on]) {
            //Serial.println("HIGH TEST - TEMP IS HIGHER. LV CUTOUT IS INACTIVE. TURN ON SWITCHES");
            switch_toutputs_on(temp_switch[i].switch_on);
          } else {
            //Serial.println("HIGH TEST - TEMP IS HIGHER BUT CHANNEL LV CUTOUT IS IN EFFECT. DO NOTHING");
          }
        }
        else {
          // Temp is lower, turn them off
          //Serial.println("HIGH TEST - TEMP IS LOWER. DEACTIVATE SWITCHES");
          switch_toutputs_off(temp_switch[i].switch_on);
        }
      }
      else if ( temp_switch[i].switching_condition == 2 ) {
       
        if ( (ch_temps[i]/100) + 273 < temp_switch[i].switching_value ) {
         // Temp is lower, turn them on but check low voltage cutout isnt activated.
          if (v_adc > ch_cutout[temp_switch[i].switch_on]) {
            //Serial.println("LOW TEST - TEMP IS LOWER. LV CUTOUT IS INACTIVE. TURN ON SWITCHES");
            switch_toutputs_on(temp_switch[i].switch_on);
          } else {
            //Serial.println("HIGH TEST - TEMP IS HIGHER BUT CHANNEL LV CUTOUT IS IN EFFECT. DO NOTHING");
          }
        }
        else {
          // Temp is higher, turn them off
          //Serial.println("LOW TEST - TEMP IS HIGHER. DEACTIVATE SWITCHES");
          switch_toutputs_off(temp_switch[i].switch_on);
        }
      } else {
        //Serial.println("Switch cond 0");
        // Condition is 0 - do absolutely nothing.
      }
      }
    }
  }

// Turn on the outputs (note this is only for use with the temperature switching as it does not toggle other outputs)  
void switch_toutputs_on(uint16_t switch_settings) {
  output_set(switch_settings);
}

// Turn off the outputs (note this is only for use with the temperature switching as it does not toggle other outputs)
void switch_toutputs_off(uint16_t switch_settings) {
  output_clear(switch_settings);
}

const uint8_t acc_temp_sample_count = 30;
static uint8_t acc_temp_idx = 0;
static int acc_temp_samples[acc_temp_sample_count] = { 0 };
static int acc_temp_value = 0;

void accumulate_temp( void ) {
  // Read temperature sensors, and store temp data to global variables-- onboardTemp and ch_temps
  temperature_read();
  // Copy on-board temp data from global variable to accumulation variable
  acc_temp_samples[acc_temp_idx++] = onboardTemp;

  if ( acc_temp_idx == acc_temp_sample_count ) {
    acc_temp_value = 0;
    for ( uint8_t sample_idx = 0; sample_idx < acc_temp_sample_count; sample_idx++ ) {
      acc_temp_value += acc_temp_samples[sample_idx];
    }
    acc_temp_idx = 0;

    acc_temp_value = acc_temp_value / acc_temp_sample_count;
  }
}


/*

 accumlate and average current draw

- take a sample every second
- store it in an array with 60 entries
- on overflow, averrage and move to a minute array
- on minute array overflow, averrage and move to an hour array
- save 12 hours of data

*/

static float current_accum_seconds[60];
static float current_accum_minutes[60];
static float current_accum_hours[12];
static float current_accum_lasthour = 0;
static float current_accum_12 = 0;
static uint16_t cas_index = 0;
static uint16_t cam_index = 0;
static uint16_t cah_index = 0;

void accumulate_current(void) {
  current_accum_seconds[cas_index++] = (a_adc / 1000);   // get and store current this second
  if (cas_index == 60) {                                    // if we have been going for a minute
    float accumulated = 0;
    for (cas_index=0;cas_index<60;cas_index++)
      accumulated += current_accum_seconds[cas_index];      // accumulate currents over the last minute
    cas_index = 0;
    current_accum_minutes[cam_index++] = accumulated / 60;  // average the current and store in in minutes array
    if (cam_index == 60) {                                  // if we have been going for an hour
      float accumulated = 0;
      for (cam_index=0;cam_index<60;cam_index++)
        accumulated += current_accum_minutes[cam_index];    // accumulate currents over the last hour
      cam_index = 0;
      current_accum_hours[cah_index++] = accumulated / 60;  // average the current and store in in hours array
      current_accum_lasthour = accumulated / 60;
      if (cah_index == 12)                                  // wrap the index if necessary
        cah_index = 0;
      for (int i=0;i<12;i++)
        current_accum_12 += current_accum_hours[i];       // accumulate over the 12 hours
      current_accum_12 /= 12;                             // and average the result
    }
  }


}


//*****************************************************************************

void setup() {
  uint16_t cutoutTemp;
  Serial.begin(115200);     // set serial communications speed
  Serial.setTimeout(10000); // maximum timeout waiting for a string input
  
    Serial.println("\r\n---------------------\r\nSwitchMaster V2 1.2.0-PRODUCTION");
  Serial.println("COPYRIGHT (C) MLT INDUSTRIES/MATT THURTELL, 2024");
  Serial.println(__DATE__ "  " __TIME__ "\r\n---------------------\r\n");
  
  clear_timeouts();
  // Uncomment below to unfuck a borked ADC.
  //mlt_nvs_write_adc_mode(false);

  // Read the hardware version
  mlt_nvs_read_hardware_version(&hdwr_ver);
  mlt_nvs_read_vendor_id(&vendor_id);

  Serial.print("Hardware version ID: ");
  Serial.println(hdwr_ver);

  Serial.print("Hardware vendor ID: ");
  Serial.println(vendor_id);
  if (hdwr_ver == 1 ){ // SwitchMaster Maxi
      Serial.println("Set MAXI hardware variant");
    } else { // it's not a maxi,
      Serial.println("Set SwitchMaster hardware variant");
    }

  // Load cutin, cutout and temp switching to RAM
  Serial.print( "Loading switch info to RAM..." );
  mlt_copy_nvs_to_ram();
  Serial.println(".Done" );

  // Load temp switching from NVS
  Serial.print( "Loading Temp Switch Cutouts..." );
  for (int i=0;i<3;i++) {
    mlt_nvs_read_temp_sw_data(i, &temp_switch[i]);
  }
  Serial.println(".Done" );
  
  // Load onboard cutout temperature and set the lower bound as 80 deg C
  // in case it is set lower.
  Serial.print( "Loading Board Temp Cutout..." );
  mlt_nvs_read_onboard_cutout_temp(&cutoutTemp);
  Serial.print(".Done.....Loaded: " );
  if (cutoutTemp < 8000) {
    Serial.println( "Too low - setting to 80 deg C" );
    gblcutoutTemp = 8000;
    mlt_nvs_write_onboard_cutout_temp(gblcutoutTemp);
  } else {
    // Serial.println( "Loaded temperature is fine." );
    gblcutoutTemp = cutoutTemp;
  }
  Serial.print(gblcutoutTemp);

  // Loads the main config data from NVS.
  config_init();
  calibration_init();
  
    // Setup override PIN Mode
  Serial.println("\nSetting up GPIO39 for override notifications.....Success");
  pinMode(OVERRIDE_PIN, INPUT);

  // See if demo mode is on
  if (demo_mode_activated ==true) {
    strcpy((char *) config_new.ssid, DEMO_SSID);
    strcpy((char *) config_new.pass, DEMO_PASS);
    config_new.pin = DEMO_PIN;
    Serial.println("\nDemo Mode is activated");
  } else {
    // Start Normal Mode, leave all settings as is
    Serial.println("\nDemo is DISABLED");
  }

  // Check if the ADC is activated or not
  if (adc_activated == true) {
    Serial.println("ADC is ENABLED");
    adc_init();
  } else {
    Serial.println("ADC is DISABLED\nSetting fake values V=13.1, A=5.00");
  }

  
  output_init();
  usb_init();
  led_init();

  //Set device as a Wi-Fi Station
  WiFi.mode(WIFI_MODE_STA);
  Serial.println("Wifi has successfully booted");
  Serial.print("Wifi MAC: ");
  Serial.println(WiFi.macAddress());

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  } else {
    Serial.println("ESP-NOW Initialised - awaiting commands");
  }
  
  // Once ESPNow is successfully Init, we will register for recv CB to
  // get the status of Trasnmitted packet
  esp_now_register_send_cb(OnDataSent);
  // get recv packer info
  esp_now_register_recv_cb(OnDataRecv);
  
  nimble_init((char*) config_new.ssid, config_new.pin);
  Serial.println("BLE has successfully booted");

  temperature_init();
  Serial.println("\nTemp Sensors Initiated");

#ifdef USE_WEBSERVER
  web_init((char*) config_new.ssid, (char*) config_new.pass);
#endif
   


  Serial.println("\r\nNVS Data Read:");
  Serial.printf("SSID  : %s\r\n", config_new.ssid);
  Serial.printf("PASS  : %s\r\n", config_new.pass);
  Serial.printf("PIN   : %d\r\n", config_new.pin);
  Serial.printf("LOWCUT: %d V\r\n\r", config_new.lowcut);
  Serial.printf("TEMP CUTOUT (°C * 100): %d \r\n\r\n", (gblcutoutTemp));

  Serial.println("\r\n** SETUP COMPLETE ***\r\n");
  Serial.println("Enable debugging to view additional info\r\n");
  Serial.println("Use Command: help for more info on commands");
}

//*****************************************************************************

void loop() {
  static uint32_t previousMillis = 0;
  static uint32_t buttonDownMillis = 0;
  static int restore = 0;
  static int calibration = 0;
  static int calibration_mode = 0;
  monitor();

  // Check to see if data has changed and load it up!
  if (nvschangeflag == true) {
          // Re-read the info from NVS
          mlt_copy_nvs_to_ram();
          
          // Reload temp switching from NVS
          for (int i=0;i<3;i++) {
            mlt_nvs_read_temp_sw_data(i, &temp_switch[i]);
          }

          nvschangeflag = false;
          // Serial.println("Change detected - reload from NVS");
  }
  
  //
  // send a notification every second
  // and do various housekeeping
  //
  uint32_t currentMillis = millis();
  if (currentMillis - previousMillis >= 1000) { // run this code every second
    previousMillis = currentMillis;

    // Start gathering info for notifications and calculations
    v_adc = round(adc_get_voltage() * 1000);
    a_adc = round(adc_get_current() * 1000);

    accumulate_current();     // collect and accumulate current use
    
    temperature_read(); // Read and display temp info
    check_overrides(); // Checks if an override is turned on to deliver the status to the app

    // Call notifications
    nimble_build_output(current_accum_lasthour,current_accum_12,override_active);     // assemble and send metrics
    nimble_build_lockouts();  // Build and notify the lockouts
    nimble_build_timers();  // Build and notify the timers
    sendESPNowData(); // Build and send data to ESPNow remotes

    // Run on-device checks
    check_timeouts();         // check and handle timeouts

    // Check if board temp is over 80°C and shutdown all outputs if it is
    
    // BRAD - comment out this if statement here, but keep the 'else'
    if ( onboardTemp > gblcutoutTemp ) {
      // Serial.println("BOARD IS HOT - SHUTTING DOWN OUTPUTS");
      // Serial.print("\nOnboard Temp: ");
      // Serial.println(onboardTemp);
      // Serial.print("\nGlobal Cutout Temp: ");
      // Serial.println(gblcutoutTemp);
      for ( int i = 0; i < NUM_OUTPUTS; i++ ) {       // loop through outputs
        output_clear( i );                  // turn output off
      }
    } else { // BRAD - keep this block below
      // Check these conditions once per minute to avoid constantly flicking relays on/off etc.
      // Temp conditions, lowvoltage cutouts, cutins
      if (temp_chk_count >= 60) {
        led_set();
        check_temp_conditions(); // check temp switches
        check_lowvoltagecutout(); // check and handle low voltage situations
        check_cutins(); // check and handle high voltage situations
        temp_chk_count = 0;      // set the flag back to 0
        led_clear();
      } else {
        temp_chk_count++;        // Increment the counter by 1
      }  
    }

    // Check USB status
    if (usbchangeflag == true) {
      if (enableusb == true) {
          // Serial.println("Enable USB Power");
          usb_set(USB_1); // Turn USB ports on
      } else {
          // Serial.println("Disable USB Power");    
          usb_clear(USB_1);
    }
    usbchangeflag = false;
    }
  }


  
  //
  // Check if BOOT button is pressed more than 10 seconds for a 'soft' reboot
  //
  if (digitalRead(0) == 0) {                        // button is down
    if ((millis() - buttonDownMillis) >= 10000) {   // if 10 seconds have passed
      // set default settings
      if (restore == 0) {
        config_restore_defaults();          // restore system defaults
        clear_timeouts();           // clear timeouts and lowcuts
        Serial.println("\r\n** RESTORED DEFAULT CONNECTION PARAMETERS **\r\n");
        restore = 1;                        // flag
        led_set();
      }
      // Check if BOOT button is pressed more than 20 seconds for a 'hard' reboot
      if ((millis() - buttonDownMillis) >= 20000) {
        Serial.println("\r\n** BOARD RETURNED TO FACTORY **\r\n");
        // Erase Switch Names
        for ( int i = 0; i < NUM_OUTPUTS; i++ ) {
          mlt_nvs_erase_switch_config(i);
        }

        // Erase temp switches
        for ( int i = 0; i < 3; i++ ) {
          mlt_nvs_erase_temp_sw_data(i);
        }

        // Set global temp cutout to 80 deg C
        mlt_nvs_write_onboard_cutout_temp(8000);
        Serial.println("\r\n** RESTORED ALL FACTORY PARAMETERS **\r\n");
      }
    }
  } else {                                  // button is up
    buttonDownMillis = millis();            // restart timing
    if (restore)                            // if settings were restored
      ESP.restart();                        // restart system
  }
}
