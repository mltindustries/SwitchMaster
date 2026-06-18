#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "mlt_defs.h"
#include "mlt_nvs.h"
#include "mlt_temp.h"

#define A_SIZEOF(ARRAY)

extern int ch_temps[3]; // list of temperatures. -127degC is a inserted for a null read. Multiplied by 100.
extern int onboardTemp;



// DEFINE OUR VARIABLES
int numberOfDevices;
OneWire oneWire(TEMP_GPIO);
DallasTemperature sensors(&oneWire);
DeviceAddress tempDeviceAddress;
DeviceAddress onboardAdd;
DeviceAddress noAddress = {0};
String address1;

// function to print a device address
void printAddress(DeviceAddress deviceAddress) {
  for (uint8_t i = 0; i < 8; i++){
    if (deviceAddress[i] < 16) Serial.print("0");
      Serial.print(deviceAddress[i], HEX);
      address1 += deviceAddress[i];
  }
}

inline bool compareAddresses(byte *addr1, byte *addr2) {
  if (0 == memcmp(addr1, addr2, 8)) {
        //Serial.println("ADDRESSES MATCH!");
        return true;
    } else {
        //Serial.println("Different");
        return false;
    }
}

void temperature_init(void) {
    Serial.print("Initialising Temp Sensors....");
    sensors.begin();
    sensors.setResolution(10);
    numberOfDevices = sensors.getDeviceCount();
    for(int i=0;i<numberOfDevices; i++){
        // Search the wire for address
        if(sensors.getAddress(tempDeviceAddress, i)){            
            Serial.print("Found device ");
            Serial.print(i, DEC);
            Serial.print(" with address: ");
            printAddress(tempDeviceAddress);            

            // Check if only one device - this is the onboard sensor ID. Write it to NVS.
            if (numberOfDevices == 1) {
              Serial.printf("1 Device found - its onboard. Saving to NVS");
              // Save the address to the NVS, if it fails, read it into a fresh variable
              if (true == mlt_nvs_write_temp_sensor_id(tempDeviceAddress)) {
                
              } 
              mlt_nvs_read_temp_sensor_id(onboardAdd);
            } else {
              Serial.println("More than 1 device - not initial boot. Grab from NVS.... ");
              mlt_nvs_read_temp_sensor_id(onboardAdd);
              Serial.print("Retrieved from NVS: ");
              printAddress(onboardAdd);
              Serial.println();
            }

            
            
            
            
    } else {
      Serial.print("Found ghost device at ");
      Serial.print(i, DEC);
      Serial.print(" but could not detect address. Check power and cabling");
    }
}
}

void temperature_read(void) {
  int offset = 0;
  // Reset all value's back to known 'null' value
    for(int i=0;i<3; i++){
      ch_temps[i] = -127 * 100;
  }

  
    // Search the wire for address
    sensors.requestTemperatures(); // Send the command to get temperatures

    for(int i=0;i<numberOfDevices; i++){
        // Serial.print("Loop # ");
        // Serial.println(i);
        if(sensors.getAddress(tempDeviceAddress, i)){
          
          // Serial.print("Device Address: ");
          // printAddress(tempDeviceAddress);
          // Serial.println();
          // Serial.print("Loaded Onboard: ");
          // printAddress(onboardAdd);
          // Serial.println();
          // Serial.println("Start conditional checking\n");

          if (compareAddresses(tempDeviceAddress, onboardAdd)) {
            //Serial.println("MATCHED ADDRESSES - Temp read from onboard sensor");
            onboardTemp = sensors.getTempC(tempDeviceAddress) * 100;
            ch_temps[0] = onboardTemp;
            // Serial.print("Onboard Temp: ");
            // Serial.println(ch_temps[0]);
            // Serial.println(onboardTemp);
          // } else if(compareAddresses(noAddress, onboardAdd)) {
          //   Serial.println("MATCHED GHOST ADDRESS - Temp read from onboard sensor");
          //   onboardTemp = sensors.getTempC(tempDeviceAddress) * 100;
          //   ch_temps[0] = onboardTemp;
          //   Serial.print("Onboard Temp: ");
          //   Serial.println(ch_temps[0]);
          //   Serial.println(onboardTemp);
          } else {

            float tempCelsius = sensors.getTempC(tempDeviceAddress);
            if (i == 0) {
              //Serial.println("Temp isnt onboard sensor but in slot 0, move up");
              offset += 1;
              ch_temps[i + offset] = tempCelsius * 100;
              //Serial.println(ch_temps[i + offset]);
            } else {
              //Serial.println("temps in correct order, just add them");
              //Serial.print("Offset: ");
              //Serial.println(offset);
              ch_temps[i + offset] = tempCelsius * 100;
            }
          }
          
    }
    }
    // for(int i=0;i<numberOfDevices; i++){
    //     Serial.print("CH_TEMP ");
    //     Serial.print(i, DEC);
    //     Serial.print(" :");
    //     Serial.println(ch_temps[i]);
    // }
  //   if(enabletempdebug) {
      
  //     // Loop through outputs and display temps
  //     for(int i=0;i<3; i++){
  //       Serial.print("ch_temp variable ");
  //       Serial.print(i,DEC);
  //       Serial.print(" raw reading is ");
  //       Serial.println(ch_temps[i]);
  //       float tempC = ch_temps[i] / 100.0;
  //       Serial.print("After conversion ");
  //       Serial.println(tempC);
  //       Serial.println();
  //       Serial.print("Onboard sensor temp: ");
  //       Serial.println(onboardTemp);
  //   }
    
  // }
}



