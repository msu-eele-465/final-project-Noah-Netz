#include "RGB_LED.h"
#include "msp430fr2355.h"

// Initial state
volatile uint8_t system_state = STATE_LOCKED;

// Function to update RGB LED colors
void updateRGB(uint8_t r, uint8_t g, uint8_t b) {
    // Scale 0-255 to 0-Max_PWM
    TB3CCR1 = (r * Max_PWM) / 255;  // Red (P6.0)
    TB3CCR2 = (g * Max_PWM) / 255;  // Green (P6.1)
    TB3CCR3 = (b * Max_PWM) / 255;  // Blue (P6.2)
}

// change LED color
void LEDState(void) {
    switch (system_state) {
        case STATE_LOCKED:
            updateRGB(196, 62, 29);  // #c43e1d
            break;
        case STATE_UNLOCKING:
            updateRGB(196, 146, 29); // #c4921d
            break;
        case STATE_UNLOCKED:
            updateRGB(29, 162, 196); // #1da2c4
            break;
        case STATE_UNLOCKED_2:
            updateRGB(0, 255, 0);    // #00ff00
            break;
        case STATE_UNLOCKED_3:
            updateRGB(255, 0, 255);  // #ff00ff
            break;
        case STATE_UNLOCKED_4:
            updateRGB(255, 127, 0);  // #ff7f00
            break;
        case STATE_UNLOCKED_5:
            updateRGB(0, 255, 255);// #00ffff
            break;
        case STATE_UNLOCKED_6:
            updateRGB(255, 255, 255);// #ffffff
            break;
        case STATE_UNLOCKED_7:
            updateRGB(0, 0, 0);  // #000000
            break;
        case STATE_UNLOCKED_0:
            updateRGB(255, 0, 0);  // #ff0000
            break;
    }
}

// Function to Setup RGB LED
void setupRGBLED(void) {
    P6DIR |= BIT0 | BIT1 | BIT2 | BIT3;   // Set P6.0, P6.1, P6.2, P6.3 as output
    P6SEL0 |= BIT0 | BIT1 | BIT2 | BIT3;  // Assign to Timer3_B7
    P6SEL1 &= ~(BIT0 | BIT1 | BIT2 | BIT3); // Ensure primary function mode
    
    // Configure Timer3_B7 for PWM
    TB3CCR0 = Max_PWM;
    TB3CCTL1 = OUTMOD_7;
    TB3CCTL2 = OUTMOD_7;
    TB3CCTL3 = OUTMOD_7;
    TB3CCTL4 = OUTMOD_7;       // For Buzzer

    TB3CTL = TBSSEL_2 | MC_1;  // Use SMCLK, Up mode

    // Set initial state to "Locked" color
    updateRGB(196, 62, 29);  // #c43e1d
}

// Turn buzzer ON with 50% duty cycle
void buzzer_on(void) {
    TB3CCR4 = Max_PWM / 2;  // 50% duty
}

// Turn buzzer OFF
void buzzer_off(void) {
    TB3CCR4 = 0;
}
