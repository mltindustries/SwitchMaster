#include <Arduino.h>

#include "mlt_defs.h"
#include "mlt_adc.h"
#include "mlt_config.h"
#include "mlt_calibration.h"
#include "mlt_output.h"
#include "mlt_eeprom.h"

#include "mlt_monitor.h"
#include "mlt_nimble.h"
#include "mlt_nvs.h"

#include "mlt_usb.h"

// list of undervoltage cutout level, in mV for a channel. 0 means ignore
extern uint16_t  ch_cutout[MAX_CHANNELS];

// list of cut-in level per switch, in mV, to auto-turn on a channel. 0 means ignore.
extern uint16_t  ch_cutin[MAX_CHANNELS];

//*****************************************************************************

typedef struct cmd_table_t {
    char *  command_string;
    void (*function)(char *param);
} cmd_table_t;
extern cmd_table_t cmd_table[];

#define MAX_COMMAND_LENGTH  30
char indata[MAX_COMMAND_LENGTH+1] = {0};


//*****************************************************************************


void calibrate(char *param) {
    calibration_run();
}




void run(char *param) {
    if (param == NULL) {
        Serial.println("Missing Parameter");
        return;
    }

    if (!strcmp(param, "test")) {
        Serial.println("*** BEGIN TEST PROTOCOL ***");
        Serial.println("** CYCLE ALL INPUTS FOR 1 SECOND **");
        // Loop through the outputs to test
        for (int i=0;i<NUM_OUTPUTS;i++) {
            Serial.printf("Testing Output %d\r\n ", i+1);
            output_set(i);
            delay(500);
            output_clear(i);
        }
        // Stop the test
        Serial.printf("\n*** END TEST PROTOCOL ***\n");
    }
}

void read_swnvs(char *param) {
    char *p1 = param;
    
    nimble_sw_data_t monitor_read = {0};
    if ( (*p1 > 0) && (*p1 < 10) ) {
    uint8_t i = atoi(p1);
            if (i > NUM_OUTPUTS-1) {
                Serial.println("Read SW: Incorrect Parameter");
                return;
            } else {
                mlt_nvs_read_switch_config(i, &monitor_read);
                Serial.printf( "Reading from NVS for switch");
                Serial.printf( "Switch position number : %d\n", monitor_read.switch_number );
                Serial.printf( "Switch name   : %s\n", monitor_read.switch_name );
                Serial.printf( "Switch cutin  : %d\n", monitor_read.voltage_cutin );
                Serial.printf( "Switch cutout : %d\n", monitor_read.voltage_cutout );
            }
    } else {
        Serial.println("Read SW: Incorrect Parameter");
    }
    
    }

void erase_swnvs(char *param) {
    char *p1 = param;
    
    uint8_t i = atoi(p1);
        if (i > NUM_OUTPUTS-1) {
            Serial.println("Erase SW: Incorrect Parameter");
            return;
        } else {
            Serial.printf( "Erased switch position number : %d\n", i);
            mlt_nvs_erase_switch_config(i);
        }
    }

    void read_tswitch(char *param) {
    char *p1 = param;
    
    nimble_temp_data_t temp_read = {0};
    if ( (*p1 > 0) && (*p1 < 10) ) {
    uint8_t i = atoi(p1);
            if (i > NUM_OUTPUTS-1) {
                Serial.println("Read TEMP SW: Incorrect Parameter");
                return;
            } else {
                mlt_nvs_read_temp_sw_data(i, &temp_read);
                Serial.printf( "Reading from NVS for Temp Switches");
          Serial.printf( "Temp Condition: %d\n", temp_read.ch_temp_switch );
          Serial.printf( "Switch condition : %d\n", temp_read.switching_condition);
          Serial.printf( "Switching value  : %d\n", temp_read.switching_value );
          Serial.printf( "Output Number  : %d\n", temp_read.switch_on);
            }
    } else {
        Serial.println("Read SW: Incorrect Parameter");
    }
    
    }

void erase_tswitch(char *param) {
    char *p1 = param;
    
    uint8_t i = atoi(p1);
        if (i > NUM_OUTPUTS-1) {
            Serial.println("Erase Temp SW: Incorrect Parameter");
            return;
        } else {
            Serial.printf( "Erased temp switch position number : %d\n", i);
            mlt_nvs_erase_temp_sw_data(i);
        }
    }

void setdevice(char *param) {
    char *p1 = param;
    uint8_t i = atoi(p1);
        if ( (i < 255) ) {
            Serial.printf( "Detected Hardware ID: %d. Writing...done\nRESTART NOW", i);
            mlt_nvs_write_hardware_version(i);
        } else {
            Serial.printf( "Incorrect hardware ID detected: %d... not writing\n", i);
        }
    }

    void setvendor(char *param) {
    char *p1 = param;
    uint8_t i = atoi(p1);
        if ( (i < 255) ) {
            Serial.printf( "Detected Vendor ID: %d. Writing...done\nRESTART NOW", i);
            mlt_nvs_write_vendor_id(i);
        } else {
            Serial.printf( "Incorrect vendor ID detected: %d... not writing\n", i);
        }
    }
    

void debug(char *param) {
    if (strstr(param, "enable")) {
        Serial.printf("\n*** DEBUG ENABLED ***\n");
        enabledebug = 1;
        enablemacdebug = 1;
    } else {
        Serial.print("\n*** DEBUG DISABLED ***\n");
        enabledebug = false;
        enablemacdebug = false;
    }
}

void timer(char *param) {
    if (strstr(param, "enable")) {
        Serial.printf("\n*** TIMER DEBUG ENABLED ***\n");
        enabletimerdebug = 1;
        enablemacdebug = 1;
    } else {
        Serial.print("\n*** TIMER DEBUG DISABLED ***\n");
        enabletimerdebug = false;
        enablemacdebug = false;
    }
}

void power(char *param) {
    if (strstr(param, "enable")) {
        Serial.printf("\n*** POWER READING DEBUG ENABLED ***\n");
        enablepowerdebug = 1;
        enablemacdebug = 1;
    } else {
        Serial.print("\n*** POWER READING DEBUG DISABLED ***\n");
        enablepowerdebug = false;
        enablemacdebug = false;
    }
}

void lowcut(char *param) {
    if (strstr(param, "enable")) {
        Serial.printf("\n*** LOWCUT DEBUG ENABLED ***\n");
        enablelowcutdebug = 1;
        enablemacdebug = true;
    } else {
        Serial.print("\n*** LOWCUT DEBUG DISABLED ***\n");
        enablelowcutdebug = false;
        enablemacdebug = false;
    }
}

void lockout(char *param) {
    if (strstr(param, "enable")) {
        Serial.printf("\n*** LOCKOUT DEBUG ENABLED ***\n");
        enablelockoutdebug = 1;
        enablemacdebug = 1;
    } else {
        Serial.print("\n*** LOCKOUT DEBUG DISABLED ***\n");
        enablelockoutdebug = false;
    }
}

void demo(char *param) {
    if (strstr(param, "enable")) {
        Serial.printf("\n*** DEMO MODE ENABLED ***\n");
        config_demo_on();
    } else {
        Serial.printf("\n*** DEMO MODE DISABLED ***\n");
        config_demo_off();
    }
}

void adc(char *param) {
    if (strstr(param, "enable")) {
        Serial.printf("\n*** ADC ACTIVATED ***\n");
            config_adc_activate();
    } else {
        Serial.printf("\n*** ADC DEACTIVE ***\n");
            config_adc_deactivate();
    }
}

void temp(char *param) {
    if (strstr(param, "enable")) {
        Serial.printf("\n*** TEMPERATURE DEBUGGING ENABLED ***\n");
            enabletempdebug = true;
    } else {
        Serial.printf("\n*** TEMPERATURE DEBUGGING DISABLED ***\n");
            enabletempdebug = false;
    }
}



void read_eeprom(char *param) {
    Serial.print("\n*** PRINTING EEPROM INFO ***\n");
    eeprom_display_values();
}

void print_cutins(char *param) {
    Serial.print("\n*** PRINTING CUT-IN INFO ***\n");
     for (int i=0;i<NUM_OUTPUTS;i++) {
            Serial.printf("\nCUTIN FOR SWITCH %d\r ", i+1);
            Serial.println(ch_cutin[i]);
        }
}

void print_cutouts(char *param) {
    Serial.print("\n*** PRINTING CUT-OUT INFO ***\n");
     for (int i=0;i<NUM_OUTPUTS;i++) {
            Serial.printf("\nCUTOUT FOR SWITCH %d: ", i+1);
            Serial.println(ch_cutout[i]);
        }
}

void control_outputs(char *param) {
    char *p1 = param;
    char *p2 = NULL;
    char *p = strchr(param, ' ');
    if (p == NULL) {
        Serial.println("Missing Parameter");
        return;
    } else {
        p2 = p+1;
        *p = 0;
    }

    bool onoff = false;
    if ( !strcmp(p2, "on"))
        onoff = true;

    if ( !strcmp(p1, "*") ) {    // working with all at once
        Serial.printf("*** TURN %s ALL OUTPUTS ***", onoff ? "ON":"OFF");
        Serial.printf("** START %s **", onoff ? "ON":"OFF");
        // Loop through the outputs, make HIGH
        for (int i=0;i<NUM_OUTPUTS;i++) {
            Serial.printf("\r\nTurning %s Output %d\r\n", onoff ? "ON":"OFF", i+1);
            if (onoff)
                output_set(i);
            else
                output_clear(i);
            delay(500);
        }
        // FINISH
        Serial.printf("\n\n*** END ALL OUTPUTS %s ***\r\n", onoff ? "ON":"OFF");
    } else {    // individual outputs
        uint16_t i = atoi(p1) - 1;
        if (i > NUM_OUTPUTS-1) {
            Serial.println("Incorrect Parameter");
            return;
        }
        Serial.printf("\r\nTurning %s Output %d\r\n", onoff ? "ON":"OFF", i+1);
        if (onoff)
            output_set(i);
        else
            output_clear(i);
    }
}

void usb(char *param) {
    if (strstr(param, "enable")) {
        Serial.printf("\n*** USB OUTPUTS ENABLED ***\n");
        usb_set(USB_1);
        enableusb = true;
        usbchangeflag = true;
    } else {
        Serial.printf("\n*** USB OUTPUTS DISABLED ***\n");
        usb_clear(USB_1);
        enableusb = false;
        usbchangeflag = true;
    }
}

void help(char *param) {
    cmd_table_t *pt = cmd_table;
    Serial.println("Command List:");
    while (pt->command_string != NULL) {
        Serial.printf("  %s\r\n", pt->command_string);
        pt++;
    }
    Serial.println("");
}

void read_adc(char *param) {
    if (adc_activated) {
        Serial.println("\nADC is activated. Values are real.");
    } else {
        Serial.println("\nADC is deactivated. Values are fake!");
    }
   Serial.print("Voltage: ");
   Serial.println(adc_get_voltage());
   Serial.print("Current: ");
   Serial.println(adc_get_current());
}


//*****************************************************************************


cmd_table_t cmd_table[] = {
    "calibrate",                calibrate,
    "run <test>",               run,
    "output <0-9|*> <on/off>" , control_outputs,
    "usb <enable|disable>",   usb,
    "readsw <0-9> " , read_swnvs, 
    "erasesw <0-9> " , erase_swnvs,
    "readtsw <0-2> " , read_tswitch, 
    "erasetsw <0-2> " , erase_tswitch,
    "sethard <0-1> " , setdevice,
    "setvendor <x> " , setvendor,
    "debug <enable|disable>",   debug,
    "power <enable|disable>",   power,
    "timer <enable|disable>",   timer,
    "lowcut <enable|disable>",  lowcut,
    "lockout <enable|disable>",  lockout,
    "demo <enable|disable>",  demo,
    "adc <enable|disable>",  adc,
    "temp <enable|disable>",  temp,
    "readeeprom" ,              read_eeprom,
    "cutins" ,              print_cutins,
    "cutouts" ,              print_cutouts,
    "readadc" ,              read_adc,
    "help",                     help,
    NULL, NULL
};


//*****************************************************************************

void monitor(void) {
    static bool cmdcomplete = false;
    char inchars[2] = {0};

    if (Serial.available()) {
        inchars[0] = Serial.read();
//        Serial.printf("Got char (0x%02x) %c\r\n", inchars[0], inchars[0]);
        switch (inchars[0]) {
            case '\r':
            case '\n':
                if (*indata) {
                    Serial.print("Command Received: ");
                    Serial.println(indata);
                    Serial.println();
                    cmdcomplete = true;
                }
                break;
            case 0x08: // backspace
            case 0x7F: // mac backspace ?
                if (strlen(indata)) // abd
                    indata[strlen(indata)-1] = 0;
                break;
            default:
                if (strlen(indata) < MAX_COMMAND_LENGTH)
                    strcat(indata, inchars);
        }

        if (cmdcomplete) {
            cmdcomplete = false;

            char *param = NULL;
            char *p = strchr(indata, ' ');
            if (p) {
                param = p+1;
                *p = 0;
            }

            cmd_table_t *pt = cmd_table;
            while (pt->command_string != NULL) {
                char *p = strchr(pt->command_string, ' ');
                if (p != NULL)
                    p--;
                else
                    p = pt->command_string + strlen(pt->command_string) + 1;
                if (strncmp(indata, pt->command_string, p - pt->command_string) == 0)
                    pt->function(param);
                pt++;
            }
            Serial.println();
            *indata = 0;
        }
    }
}

void monitor_task( void *pvParams ) {
    (void) pvParams;

    while ( 1 ) {
        monitor();
        vTaskDelay( pdMS_TO_TICKS( 250UL ) );
    }
}

/******************************* END OF FILE *******************************/
