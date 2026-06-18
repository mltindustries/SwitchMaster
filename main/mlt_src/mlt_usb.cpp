


#include <Arduino.h>

#include "mlt_usb.h"


void usb_init(void) {
    pinMode(USB_1_CTRL_GPIO, OUTPUT);
    pinMode(USB_2_CTRL_GPIO, OUTPUT); // unused in v2
    usb_set(USB_1);
    usb_set(USB_2); // unused in v2
}


void usb_set(usb_e channel) {
    channel == USB_1 ? digitalWrite(USB_1_CTRL_GPIO, HIGH) :  digitalWrite(USB_2_CTRL_GPIO, HIGH) ;
}

void usb_clear(usb_e channel) {
    channel == USB_1 ? digitalWrite(USB_1_CTRL_GPIO, LOW) :  digitalWrite(USB_2_CTRL_GPIO, LOW) ;
}

bool usb_hasfault(usb_e channel) {
    return ! ( (channel == USB_1) ? digitalRead(USB_1_FAULT_GPIO) :  digitalRead(USB_1_FAULT_GPIO) ) ;
}
