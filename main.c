#include <xc.h>
#include "pins.h"
#include "globals.h"

// ================= CONFIG BITS =================
#pragma config FOSC  = INTRC_NOCLKOUT   // Internal oscillator
#pragma config WDTE  = OFF
#pragma config PWRTE = ON
#pragma config MCLRE = ON
#pragma config CP    = OFF
#pragma config CPD   = OFF
#pragma config BOREN = ON
#pragma config IESO  = OFF
#pragma config FCMEN = OFF
#pragma config LVP   = OFF

#define _XTAL_FREQ 8000000


unsigned int blink_cnt = 0;
__bit blink_state = 0;
unsigned int motor_on = 0;
unsigned int r2_timer = 0;
unsigned char r2_state = 0;
unsigned int dry_run_timer = 0;
unsigned int dry_run = 0;

// ================= INITIALIZATION =================
void init_hw(void)
{
    // Oscillator: 8 MHz internal
    OSCCON = 0b01110010;

    // Disable all analog
    ANSEL  = 0x00;
    ANSELH = 0x00;

    // Directions
    TRISA = 0b00111110;   // RA1?RA5 inputs
    TRISB = 0b00000000;   // LEDs outputs
    TRISC = 0b00000010;   // RC1 input, others outputs

    // Clear outputs
    PORTB = 0x00;
    PORTC = 0x00;
}


void alarm(void)
{
    BUZZER = 1;
    __delay_ms(300);
    BUZZER = 0;
}


void pump_led_blink(void)
{
    if (motor_on)   // pump is ON ? blink
    {
        blink_cnt++;

        if (blink_cnt >= 100)   // ~500ms (depends on loop speed)
        {
            blink_cnt = 0;
            blink_state ^= 1;
            LED_PUMP_ON = blink_state;
        }
    }
    else
    {
        LED_PUMP_ON = 0;
        blink_cnt = 0;
        blink_state = 0;
    }
}


void relay2_on_pump_start(void)
{
    switch (r2_state)
    {
        case 0: // idle
            r2_timer = 0;
            r2_state = 1;
            break;

        case 1: // wait 2 seconds
            if (++r2_timer >= 400)
            {
                r2_timer = 0;
                RELAY_2 = 1;
                r2_state = 2;
            }
            break;

        case 2: // relay ON for 3 seconds
            if (++r2_timer >= 600)
            {
                RELAY_2 = 0;
                r2_state = 3;
            }
            break;

        case 3:
            // done, do nothing
            break;
    }
}


void reset_relay2_logic(void)
{
    r2_state = 0;
    r2_timer = 0;
    RELAY_2 = 0;
}


unsigned char dry_run_check(void)
{
    if (!motor_on)
    {
        dry_run_timer = 0;
        return 0;
    }

    if (dry_run_timer >= 2000)
    {
        return 1;   // dry run detected
    }
    
    if (DRY_RUN) {
        dry_run_timer++;
    } else {
        dry_run_timer = 0;
    }
    return 0;
}


    
void main(void)
{
    init_hw();
    
    while(1)
    {
        // Handle Dry Run
        if (dry_run)
        {
            LED_DRY_RUN = 1;
            if (!RESET_SW)
            {
                dry_run = 0;
                LED_DRY_RUN = 0;
                dry_run_timer = 0;
                
                motor_on = 1;
                LED_PUMP_ON = 1;
                RELAY_1 = 1;
            } 
            continue;
            
        }
        
        
       // Handle Tank full 
       if (!TANK_FULL && motor_on) {
            motor_on = 0;
            LED_TANK_LOW = 0;
            LED_PUMP_ON = 0;
            RELAY_1 = 0;
            dry_run_timer = 0;

            LED_TANK_FULL = 1;
            alarm();
           __delay_ms(2000); // no loop delay, 2s
           LED_TANK_FULL = 0;
           reset_relay2_logic();
       }
       
       // Handle Tank Low 
       if (!TANK_LOW && !motor_on) {
           motor_on = 1;
           LED_PUMP_ON = 1;
           RELAY_1 = 1;
           LED_TANK_LOW = 1;
           alarm();
       } else if (!TANK_LOW && motor_on) {
           LED_TANK_LOW = 1;
       } else {
           LED_TANK_LOW = 0;
       }
       
       // Blink PUMP ON LED when motor is on
       if (motor_on) {
            pump_led_blink();
            relay2_on_pump_start();
            
            if (dry_run_check())
            {
                motor_on = 0;
                LED_PUMP_ON = 0;
                LED_TANK_LOW = 0;
                LED_TANK_FULL = 0;
                RELAY_1 = 0;

                dry_run = 1;
                alarm();
            }
       }

       __delay_ms(5);
   }
}