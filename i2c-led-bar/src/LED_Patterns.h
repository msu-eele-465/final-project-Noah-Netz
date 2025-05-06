#ifndef LED_PATTERNS_H
#define LED_PATTERNS_H

#include <msp430.h>
#include <stdint.h>
#include <stdbool.h>

// LED Array Pins (Updated for MSP430FR2310)
#define LED1  BIT0  // P2.0
#define LED2  BIT6  // P2.6
#define LED3  BIT7  // P2.7
#define LED4  BIT7  // P1.7 (was LED8)
#define LED5  BIT6  // P1.6 (was LED7)
#define LED6  BIT5  // P1.5 (unchanged)
#define LED7  BIT4  // P1.4 (was LED5)
#define LED8  BIT1  // P1.1 (was LED4)

// LED Port Definitions
#define LED_PORT1  P1OUT
#define LED_PORT2  P2OUT


// Grouped LED Pins
#define LED_MASK_P1 (LED4 | LED5 | LED6 | LED7 | LED8)
#define ALL_LED_P2 (LED1 | LED2 | LED3)

// I2C
#define SLAVE_ADDR 0x42


#endif // LED_PATTERNS_H
