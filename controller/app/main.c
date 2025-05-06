#include <msp430fr2355.h>
#include <stdint.h>
#include "src/RGB_LED.h"

#define TRIG_PORT   P2OUT
#define TRIG_DIR    P2DIR
#define TRIG_SEL0   P2SEL0
#define TRIG_SEL1   P2SEL1
#define TRIG_PIN    BIT1    // P2.1 for trigger (TB1.2)

#define ECHO_PIN    BIT0    // P2.0 for echo (TB1.1 / CCI1A)

volatile uint16_t t_start = 0;
volatile uint16_t t_end   = 0;
volatile uint8_t  ready   = 0;

void init_trigger_pwm(void) {
    // Set P2.1 to output PWM (TB1.2)
    TRIG_DIR  |= TRIG_PIN;
    TRIG_SEL0 |= TRIG_PIN;
    TRIG_SEL1 &= ~TRIG_PIN;

    TB1CCR0 = 37500 - 1;             // ~26.6ms period (assuming 1.5MHz SMCLK)
    TB1CCR2 = 750;                  // ~0.5ms high pulse
    TB1CCTL2 = OUTMOD_7;           // Reset/Set mode
    TB1CTL = TBSSEL__SMCLK | MC__UP | TBCLR;
}

void init_echo_capture(void) {
    P2DIR &= ~ECHO_PIN;            // P2.0 as input
    P2SEL0 |= ECHO_PIN;           // Set as TB1.1 input
    P2SEL1 &= ~ECHO_PIN;

    TB1CCTL1 = CM_3 | CCIS_0 | SCS | CAP | CCIE;  // Capture both edges, CCI1A
    TB1CTL |= TBSSEL__SMCLK | MC__CONTINUOUS;    // Continuous mode, SMCLK
}

void setup_gpio_led(void) {
    P1DIR |= BIT0;
    P1OUT &= ~BIT0;
}

uint16_t get_pulse_width_us(void) {
    ready = 0;
    while (!ready);
    return t_end - t_start;  // Timer ticks
}

#pragma vector = TIMER1_B1_VECTOR
__interrupt void TIMER1_B1_ISR(void) {
    if (TB1IV == TB1IV_TBCCR1) {
        if (!(TB1CCTL1 & CCI))
            t_start = TB1CCR1; // Rising edge
        else {
            t_end = TB1CCR1;   // Falling edge
            ready = 1;
        }
    }
}

int main(void) {
    WDTCTL = WDTPW | WDTHOLD;
    PM5CTL0 &= ~LOCKLPM5; // Unlock GPIOs

    init_trigger_pwm();
    init_echo_capture();
    setup_gpio_led();
    setupRGBLED();             // Sets up RGB LED and buzzer pin (P6.3 on TB3.4)
    __enable_interrupt();

    while (1) {
        buzzer_on();
        __delay_cycles(10000);

        buzzer_off();
        __delay_cycles(10000);
        
        uint16_t ticks = get_pulse_width_us();
        float distance_cm = ticks / 58.0f;  // rough conversion for 1.5MHz clock

        if (distance_cm < 10) P1OUT |= BIT0;
        else P1OUT &= ~BIT0;
    }
}
