
#include <Arduino.h>

#include "mlt_output.h"


//
// table to convert a output to the corresponding GPIO number
//
static uint8_t output_gpio_table[] = { 25, 26, 27, 14, 12, 4, 16, 17, 5, 18};


void output_init(void) {
    //Set all outputs to low
    for (int i=0;i<NUM_OUTPUTS;i++) {
      pinMode(output_gpio_table[i], OUTPUT);
      digitalWrite(output_gpio_table[i], LOW);
    }
}


void output_set(int output) {
    digitalWrite(output_gpio_table[output], HIGH);
}

void output_clear(int output) {
    digitalWrite(output_gpio_table[output], LOW);
}

bool output_status(int output) {
    return (digitalRead(output_gpio_table[output]) == HIGH);
}
