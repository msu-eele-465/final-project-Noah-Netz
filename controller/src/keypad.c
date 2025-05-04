#include <stdio.h>
#include <msp430.h>
#include <stdbool.h>
#include "keypad.h"

// initialize vars
char current_key = 'N';
char prev_key = 'N';
volatile int col_1 = BIT4;  // P2.4
volatile int col_2 = BIT5;  // P2.5
volatile int col_3 = BIT6;  // P2.6
volatile unsigned int col_4 = BIT7; // P2.7
volatile int current_row = 0;
// 0 = locked, 1 = unlocking, 2 = unlocked
volatile int locked_state = 0;
volatile int password_unlock = false;
volatile int pass_inx_char = 0;
volatile int LED_speed = 0;

// initialize ports
int init_keypad_ports(void) {
    // rows (outputs for row_cycle) → P4.4 - P4.7
    P4DIR |= BIT4 | BIT5 | BIT6 | BIT7;
    P4OUT &= ~(BIT4 | BIT5 | BIT6 | BIT7);

    // columns (inputs for polling) → P2.4 - P2.7
    P2DIR &= ~(BIT4 | BIT5 | BIT6 | BIT7);
    P2REN |= BIT4 | BIT5 | BIT6 | BIT7;
    P2OUT |= BIT4 | BIT5 | BIT6 | BIT7;
    P2IES |= BIT4 | BIT5 | BIT6 | BIT7;

    return 0;
}

// init irqs
int init_keypad_irqs(void) {
    P2IFG &= ~(BIT4 | BIT5 | BIT6 | BIT7);
    P2IE  |= (BIT4 | BIT5 | BIT6 | BIT7);
    return 0;
}

// cycle through rows turn on/off
int row_cycle(void) {
    current_row = 1;
    P4OUT |= BIT4;
    P4OUT &= ~BIT4;
    __delay_cycles(10000);

    current_row = 2;
    P4OUT |= BIT5;
    P4OUT &= ~BIT5;
    __delay_cycles(10000);

    current_row = 3;
    P4OUT |= BIT6;
    P4OUT &= ~BIT6;
    __delay_cycles(10000);

    current_row = 4;
    P4OUT |= BIT7;
    P4OUT &= ~BIT7;
    __delay_cycles(10000);

    return 0;
}

// mask columns for easier reading
int col_masking(void) {
    col_1 = P2IN & BIT4;
    col_2 = P2IN & BIT5;
    col_3 = P2IN & BIT6;
    col_4 = P2IN & BIT7;
    return 0;
}

// determine lock state
int lock_state(void) {
    if ((current_key == 'D') && (prev_key != 'D')) {
        locked_state = 0;
        password_unlock = false;
        pass_inx_char = 0;
    } else if (!password_unlock) {
        if ((current_key == '1') && (prev_key != '1') && (pass_inx_char == 0)) {
            pass_inx_char = 1;
            locked_state = 1;
        } else if ((current_key == '1') && (prev_key != '1')) {
            locked_state = 0;
            pass_inx_char = 0;
        } else if ((current_key == '2') && (prev_key != '2') && (pass_inx_char == 1)) {
            pass_inx_char = 2;
        } else if ((current_key == '2') && (prev_key != '2')) {
            locked_state = 0;
            pass_inx_char = 0;
        } else if ((current_key == '3') && (prev_key != '3') && (pass_inx_char == 2)) {
            pass_inx_char = 3;
        } else if ((current_key == '3') && (prev_key != '3')) {
            locked_state = 0;
            pass_inx_char = 0;
        } else if ((current_key == '4') && (prev_key != '4') && (pass_inx_char == 3)) {
            locked_state = 2;
            password_unlock = true;
        } else if ((current_key == '4') && (prev_key != '4')) {
            locked_state = 0;
            pass_inx_char = 0;
        }
    } else {
        button_logic();  // unlocked logic
    }

    return 0;
}

// unlocked functionality
// int button_logic() {
//     if ((current_key == '1') && (prev_key != '1')) locked_state = 2;
//     else if ((current_key == '2') && (prev_key != '2')) locked_state = 3;
//     else if ((current_key == '3') && (prev_key != '3')) locked_state = 4;
//     else if ((current_key == '4') && (prev_key != '4')) locked_state = 5;
//     else if ((current_key == '5') && (prev_key != '5')) locked_state = 6;
//     else if ((current_key == '6') && (prev_key != '6')) locked_state = 7;
//     else if ((current_key == '7') && (prev_key != '7')) locked_state = 8;
//     else if ((current_key == '0') && (prev_key != '0')) locked_state = 9;
//     else if ((current_key == 'A') && (prev_key != 'A')) LED_speed = 1;
//     else if ((current_key == 'B') && (prev_key != 'B')) LED_speed = 2;

//     return 0;
// }

int button_logic() {
    if ((current_key == '0') && (prev_key != '0')) {
        locked_state = 9;
        sendCommandByte(0x00);  // Pattern 0
    }
    else if ((current_key == '1') && (prev_key != '1')) {
        locked_state = 2;
        sendCommandByte(0x01);  // Pattern 1
    }
    else if ((current_key == '2') && (prev_key != '2')) {
        locked_state = 3;
        sendCommandByte(0x02);  // Pattern 2
    }
    else if ((current_key == '3') && (prev_key != '3')) {
        locked_state = 4;
        sendCommandByte(0x03);  // Pattern 3
    }
    else if ((current_key == '4') && (prev_key != '4')) {
        locked_state = 5;
        sendCommandByte(0x04);
    }
    else if ((current_key == '5') && (prev_key != '5')) {
        locked_state = 6;
        sendCommandByte(0x05);
    }
    else if ((current_key == '6') && (prev_key != '6')) {
        locked_state = 7;
        sendCommandByte(0x06);
    }
    else if ((current_key == '7') && (prev_key != '7')) {
        locked_state = 8;
        sendCommandByte(0x07);
    }
    else if ((current_key == 'A') && (prev_key != 'A')) {
        locked_state = 10;
        sendCommandByte(0x41);  // Decrease transition time
    }
    else if ((current_key == 'B') && (prev_key != 'B')) {
        locked_state = 11;
        sendCommandByte(0x42);  // Increase transition time
    }

    return 0;
}

