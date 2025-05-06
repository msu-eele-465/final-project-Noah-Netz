#include "msp430fr2310.h"
#include "src/LED_Patterns.h"

volatile uint8_t led_level = 0;  // Value received via I2C (0–8)


void init_heartbeat(void) {
    // Set P1.0 as output
    P1DIR |= BIT0;
    P1OUT &= ~BIT0;

    // Timer_B1 CCR0 setup (1 Hz assuming ACLK = 32.768 kHz)
    TB1CCTL0 = CCIE;              // Enable interrupt
    TB1CCR0 = 32768;              // 1 second period
    TB1CTL = TBSSEL__ACLK | MC__UP | TBCLR;  // ACLK, up mode, clear
}


// Initialize LED GPIOs
void array_init(void) {
    PM5CTL0 &= ~LOCKLPM5;

    P1DIR |= (BIT0 | LED4 | LED5 | LED6 | LED7 | LED8);  // Set P1 LEDs as outputs
    P2DIR |= (LED1 | LED2 | LED3);                // Set P2 LEDs as outputs

    // Set P2.6 & P2.7 to GPIO mode
    P2SEL0 &= ~(BIT6 | BIT7);
    P2SEL1 &= ~(BIT6 | BIT7);

    P1OUT = 0x00;  // Turn off P1 LEDs
    P2OUT = 0x00;  // Turn off P2 LEDs
}

void i2c_init_slave(void) {
    // Set P1.2 (SDA) and P1.3 (SCL) for I2C functionality
    P1SEL0 |= BIT2 | BIT3;
    P1SEL1 &= ~(BIT2 | BIT3);

    P1REN |= BIT2 | BIT3;  // Enable resistor on P1.2 (SDA) and P1.3 (SCL)
    P1OUT |= BIT2 | BIT3;  // Set as pull-up


    // Put eUSCI_B0 into reset
    UCB0CTLW0 = UCSWRST;

    // I2C mode, synchronous
    UCB0CTLW0 |= UCMODE_3 | UCSYNC;

    // 7-bit slave address
    UCB0I2COA0 = SLAVE_ADDR | UCOAEN;

    // Clear reset to start operation
    UCB0CTLW0 &= ~UCSWRST;

    // Enable I2C interrupts
    UCB0IE |= UCRXIE0 | UCSTPIE | UCNACKIE;
}

void update_led_bar(uint8_t level) {
    // Turn everything off first
    LED_PORT1 &= ~LED_MASK_P1;
    LED_PORT2 &= ~ALL_LED_P2;

    if (level >= 1) LED_PORT2 |= LED1;
    if (level >= 2) LED_PORT2 |= LED2;
    if (level >= 3) LED_PORT2 |= LED3;
    if (level >= 4) LED_PORT1 |= LED4;
    if (level >= 5) LED_PORT1 |= LED5;
    if (level >= 6) LED_PORT1 |= LED6;
    if (level >= 7) LED_PORT1 |= LED7;
    if (level >= 8) LED_PORT1 |= LED8;
}



int main(void) {
    WDTCTL = WDTPW | WDTHOLD;  // Stop watchdog timer
    PM5CTL0 &= ~LOCKLPM5;

    array_init();              // Initialize LEDs and timer
    init_heartbeat();         // Start the heartbeat LED
    i2c_init_slave();

    __enable_interrupt();
    __bis_SR_register(GIE);        // Enable global interrupts

    while (1) {
    }
}


// -- Interrupt Service Routines -------------------------

// I2C
#pragma vector = EUSCI_B0_VECTOR
__interrupt void EUSCI_B0_ISR(void) {
    switch (__even_in_range(UCB0IV, USCI_I2C_UCBIT9IFG)) {
        case USCI_I2C_UCRXIFG0:  // Receive buffer full
            led_level = UCB0RXBUF;
            update_led_bar(led_level);  // Update LEDs immediately
            break;

        case USCI_I2C_UCSTPIFG:  // Stop condition
            UCB0IFG &= ~UCSTPIFG;
            break;

        case USCI_I2C_UCNACKIFG:
            UCB0CTLW0 |= UCTXSTT;  // Retry
            break;
    }
}


// Timer_B1 CCR0 ISR
#pragma vector = TIMER1_B0_VECTOR
__interrupt void Timer_B1_ISR(void) {
    P1OUT ^= BIT0;   // Toggle heartbeat LED
}
