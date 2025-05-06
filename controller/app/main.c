#include <msp430fr2355.h>
#include <stdint.h>
#include "src/RGB_LED.h"

// State Machine
typedef enum { IDLE, FOCUS, BREAK, OVERTIME } PomodoroState;
PomodoroState current_state = IDLE;

#define TRIG_PORT   P2OUT
#define TRIG_DIR    P2DIR
#define TRIG_SEL0   P2SEL0
#define TRIG_SEL1   P2SEL1
#define TRIG_PIN    BIT1    // P2.1 for trigger (TB1.2)

#define ECHO_PIN    BIT0    // P2.0 for echo (TB1.1 / CCI1A)

#define LED_OUT     P1OUT
#define LED_DIR     P1DIR
#define LED_PIN     BIT0

#define BUTTON_PIN  BIT0
#define BUTTON_PORT P4IN
#define BUTTON_DIR  P4DIR
#define BUTTON_REN  P4REN
#define BUTTON_OUT  P4OUT
#define BUTTON_IE   P4IE
#define BUTTON_IES  P4IES
#define BUTTON_IFG  P4IFG

#define FOCUS_DURATION 100      // Seconds (change to 1500 for 25min)
#define BREAK_DURATION 50       // Seconds (change to 300 for 5min)
#define SLAVE_ADDR 0x42


volatile uint8_t overflow_count = 0;
volatile uint32_t seconds_elapsed = 0;
volatile uint8_t led_update_pending = 0;
volatile uint8_t led_level_to_send = 0;


volatile uint16_t t_start = 0;
volatile uint16_t t_end   = 0;
volatile uint8_t  ready   = 0;

volatile bool button_pressed = false;

char packet[] = {0x03};
int data_Cnt = 0;

void init_I2C() {
    //-- 1. Put eUSCI_B0 into software reset
	UCB0CTLW0 |= UCSWRST;                   // UCSWRST=1 for eUSCI_B0 in SW reset

	//-- 2. Configure eUSCI_B0
	UCB0CTLW0 |= UCSSEL__SMCLK;             // Choose BRCLK=SMCLK=1MHz
	UCB0BRW = 10;                           // Divide BRCLK by 10 for SCL=100kHz

	UCB0CTLW0 |= UCMODE_3;                  // Put into I2C mode
	UCB0CTLW0 |= UCMST;                     // Put into master mode
	UCB0CTLW0 |= UCTR;
	UCB0I2CSA = 0x0042;                     // Slave address = 0x42 (LED Bar)

	UCB0CTLW1 |= UCASTP_2;                  // Auto STOP when UCB0TBCNT reached
	UCB0TBCNT = sizeof(packet);             // # of bytes in packet

	//-- 3. Configure Ports
	P1SEL1 &= ~BIT3;                        // P1.3 = SCL
	P1SEL0 |= BIT3;

	P1SEL1 &= ~BIT2;                        // P1.2 = SDA
	P1SEL0 |= BIT2;

	PM5CTL0 &= ~LOCKLPM5;                   // Disable Low Power Mode

	//-- 4. Take eUSCI_B0 out of SW reset
	UCB0CTLW0 &= ~UCSWRST;                  // UCSWRST = 1 for eUSCI_B0 in SW reset

	//-- 5. Enable Interrupts
	UCB0IE |= UCTXIE0;                      // Enable I2C Tx0 IRQ
	__enable_interrupt();
}

void sendCommandByte(char byte) {
    packet[0] = byte;
    data_Cnt = 0;                          // Reset before transmit
    UCB0TBCNT = 1;
    UCB0CTLW0 |= UCTR | UCTXSTT;           // Transmit mode + START
    while (UCB0CTLW0 & UCTXSTT);           // Wait for START to clear
    while (!(UCB0IFG & UCSTPIFG));         // Wait for STOP
    UCB0IFG &= ~UCSTPIFG;
}

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


void init_button(void) {
    BUTTON_DIR &= ~BUTTON_PIN;     // Set as input
    BUTTON_REN |= BUTTON_PIN;      // Enable pull resistor
    BUTTON_OUT |= BUTTON_PIN;      // Pull-up selected

    BUTTON_IFG &= ~BUTTON_PIN;     // Clear flag
    BUTTON_IES |= BUTTON_PIN;      // Falling edge
    BUTTON_IE  |= BUTTON_PIN;      // Enable interrupt
}


void transitionTo(PomodoroState next) {
    current_state = next;
    seconds_elapsed = 0;

    switch (next) {
        case FOCUS:
            updateRGB(255, 0, 255);  // Green
            break;
        case BREAK:
            updateRGB(255, 255, 0);  // Blue
            break;
        case OVERTIME:
            updateRGB(0, 255, 255);  // Start with Red
            break;
        case IDLE:
        default:
            updateRGB(0, 120, 235); // Orange/locked
            //updateRGB(0, 255, 0);
            break;
    }
}

// int main(void) {
//     WDTCTL = WDTPW | WDTHOLD;

//     // Setup LED
//     P1DIR |= BIT0;
//     P1OUT &= ~BIT0;

//     // Setup Pomodoro Timer
//     TB0CTL |= TBCLR;
//     TB0CTL |= TBSSEL__SMCLK;    // Source=SMCLK
//     TB0CTL |= MC__CONTINUOUS;
//     TB0CTL |= ID__8;            // Divide-by-8;

//     // Setup Timer Overflow IRQ
//     TB0CTL &= ~TBIFG;
//     TB0CTL |= TBIE;

//     init_I2C();
//     __delay_cycles(4800000); // Wait

//     sendCommandByte(0x00);

//     while (1) {
//         // sendCommandByte(0x00);
//         // __delay_cycles(24000000);
//     }
// }

int main(void) {
    WDTCTL = WDTPW | WDTHOLD;

    init_trigger_pwm();
    init_echo_capture();
    setup_gpio_led();
    setupRGBLED();             // Sets up RGB LED and buzzer pin (P6.3 on TB3.4)
    init_button();
    init_I2C();
    
    // Setup LED
    P1DIR |= BIT0;
    P1OUT &= ~BIT0;

    // Setup Pomodoro Timer
    TB0CTL |= TBCLR;
    TB0CTL |= TBSSEL__SMCLK;    // Source=SMCLK
    TB0CTL |= MC__CONTINUOUS;
    TB0CTL |= ID__8;            // Divide-by-8;

    // Setup Timer Overflow IRQ
    TB0CTL &= ~TBIFG;
    TB0CTL |= TBIE;

    transitionTo(IDLE);
    
    __delay_cycles(4800000); // Wait
    
    
    //__enable_interrupt();

    //sendCommandByte(0x00);

    while (1) {
        if (led_update_pending) {
            sendCommandByte(led_level_to_send);
            led_update_pending = 0;
        }

        switch (current_state) {
            case IDLE:
                if (button_pressed) {
                    button_pressed = false;
                    transitionTo(FOCUS);
                }
                break;

            case FOCUS:
                if (button_pressed) {
                    button_pressed = false;
                    transitionTo(BREAK);
                } else if (seconds_elapsed >= FOCUS_DURATION) {
                    transitionTo(OVERTIME);
                }
                break;

            case BREAK:
                if (button_pressed) {
                    button_pressed = false;
                    transitionTo(FOCUS);
                } else if (seconds_elapsed >= BREAK_DURATION) {
                    transitionTo(OVERTIME);
                }
                break;

            case OVERTIME:
                if (button_pressed) {
                    button_pressed = false;
                    transitionTo(IDLE);
                }
                break;
        }

        //updateRGB(255, 0, 0);

        // buzzer_on();
        // __delay_cycles(10000);

        // buzzer_off();
        // __delay_cycles(10000);
        
        // uint16_t ticks = get_pulse_width_us();
        // float distance_cm = ticks / 58.0f;  // rough conversion for 1.5MHz clock

        // if (distance_cm < 10) P1OUT |= BIT0;
        // else P1OUT &= ~BIT0;
    }
}


//-- Interrupt Service Routines --------
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


#pragma vector = TIMER0_B1_VECTOR
__interrupt void ISR_TB0_Overflow(void)
{
    if (TB0IV == TB0IV_TBIFG) {
        overflow_count++;
        if (overflow_count >= 2) {
            overflow_count = 0;
            seconds_elapsed++;

            LED_OUT ^= LED_PIN;  // Heartbeat

            uint8_t level = 0;
            switch (current_state) {
                case FOCUS:
                    level = (seconds_elapsed * 8) / FOCUS_DURATION;
                    break;
                case BREAK:
                    level = (seconds_elapsed * 8) / BREAK_DURATION;
                    break;
                case OVERTIME:
                    if (seconds_elapsed % 2 == 0)
                        updateRGB(0, 255, 255);  // Red
                    else
                        updateRGB(255, 255, 0);  // Blue
                    return;
                default:
                    return;
            }
            if (level > 8) level = 8;

            led_level_to_send = level;
            led_update_pending = 1;
        }
    }
}


#pragma vector = EUSCI_B0_VECTOR
__interrupt void EUSCI_B0_I2C_ISR(void){
    UCB0TXBUF = packet[data_Cnt];          // Send the byte
    data_Cnt++;

    if (data_Cnt >= sizeof(packet)) {
        data_Cnt = 0;                      // Reset for next transmission
    }
}

#pragma vector = PORT4_VECTOR
__interrupt void Port_4_ISR(void) {
    if (BUTTON_IFG & BUTTON_PIN) {
        BUTTON_IE &= ~BUTTON_PIN;       // Disable button interrupt
        BUTTON_IFG &= ~BUTTON_PIN;      // Clear flag

        __delay_cycles(16000);          // ~1ms debounce at 16MHz (adjust if needed)

        if (!(BUTTON_PORT & BUTTON_PIN)) { // Confirm button is still pressed
            button_pressed = true;
        }

        BUTTON_IFG &= ~BUTTON_PIN;      // Clear flag again just in case
        BUTTON_IE |= BUTTON_PIN;        // Re-enable interrupt
    }
}

