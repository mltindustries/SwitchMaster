
#include <Arduino.h>

#include "mlt_led.h"

void led_init(void) {
    pinMode(LED_GPIO, OUTPUT);
    led_clear();
}

void led_set(void) {
    digitalWrite(LED_GPIO, HIGH);
}

void led_clear(void) {
    digitalWrite(LED_GPIO, LOW);
}
