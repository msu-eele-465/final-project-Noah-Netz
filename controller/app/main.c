#include <msp430.h>
#include <stdint.h>
#include "src/RGB_LED.h"

int main(void) {
    WDTCTL = WDTPW | WDTHOLD;  // Stop watchdog timer
    PM5CTL0 &= ~LOCKLPM5;      // Enable GPIO

    setupRGBLED();             // Sets up RGB LED and buzzer pin (P6.3 on TB3.4)

    while (1) {
        buzzer_on();
        __delay_cycles(10000);

        buzzer_off();
        __delay_cycles(10000);
    }
}
