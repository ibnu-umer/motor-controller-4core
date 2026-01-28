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
#define BLINK_TICKS 100


// =============== VARIABLES =============== //
unsigned int blink_cnt = 0;
__bit blink_state = 0;
unsigned int motor_on = 0;
unsigned int relay_timer = 0;
unsigned char relay_state = 0;
unsigned int dry_run_timer = 0;
unsigned int dry_run_latched = 0;      
__bit on = 0;



// ============ INITIALIZATION ============ // 

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


// ================ HELPERS ================ //

void alarm(void)
{
    BUZZER = 1; __delay_ms(400);
    BUZZER = 0; __delay_ms(400);
    BUZZER = 1; __delay_ms(400);
    BUZZER = 0;
}


void pump_led_blink(void)
{
    if (!motor_on)
    {
        LED_PUMP_ON = 0;
        blink_cnt   = 0;
        blink_state = 0;
        return;
    }

    if (++blink_cnt >= BLINK_TICKS)
    {
        blink_cnt = 0;
        blink_state ^= 1;
    }

    LED_PUMP_ON = blink_state;
}


void run_starter_relay(void)
{
    switch (relay_state)
    {
        case 0: // 2 Seconds waiting
            if (++relay_timer >= 400)
            {
                relay_timer = 0;
                RELAY_2 = 1;
                RELAY_3 = 1;
                relay_state = 1;
            }
            break;

        case 1: // Relay on for 3 seconds
            if (++relay_timer >= 600)
            {
                RELAY_2 = 0;
                RELAY_3 = 0;
                relay_state = 2;
            }
            break;

        case 2: // Done, do nothing
            break;
    }
}


void reset_starter_relay(void)
{
    relay_state = 0; relay_timer = 0;
    RELAY_2 = 0; RELAY_3 = 0;
}


unsigned char dry_run_check(void)
{
    // dry_run_timer : 10 seconds = 2000
    
    if (!motor_on) { return 0; }
    
    if (dry_run_latched && dry_run_timer >= 4000) { return 1;} // Lost to dry run after 

    if (dry_run_timer >= 30000) { return 1; } // Dry run detected
    
    if (DRY_RUN) { dry_run_timer++; }  // increase timer in DRY RUN not triggered
    else { dry_run_timer = 0; dry_run_latched = 1; }
    
    return 0;
}


void toggle_motor(void)
{
    LED_PUMP_ON = on;
    RELAY_1     = on;
    motor_on    = on;
    
    reset_starter_relay();
    
    if (on) {
        dry_run_timer = 0;
        LED_DRY_RUN = 0;
    }
}



// ================= MAIN ================= //

void main(void)
{
    init_hw();
    
    // Delay before starting
    __delay_ms(20000); alarm();
    
    while(1)
    {
        
        // Handle Reset switch trigger
        if (!RESET_SW && !motor_on) 
        {
            LED_DRY_RUN = 0;
            on = 1; toggle_motor();
            alarm();
        } 
        
        
        // Handle Dry Run
        if (LED_DRY_RUN)
        {
            dry_run_latched = 1;
            __delay_ms(5);
            continue;
        }
        
        
        // Handle Tank full 
        if (!TANK_FULL && motor_on && relay_timer >= 600) {
            LED_TANK_LOW = 0;
            on = 0; toggle_motor();
            dry_run_latched = 0;

            LED_TANK_FULL = 1;
            alarm();

            __delay_ms(10000); // no loop delay, 10s
            LED_TANK_FULL = 0;

            reset_starter_relay();
        }
       
        
        // Handle Tank Low 
        if (!TANK_LOW && !motor_on) {
            on = 1; toggle_motor();
            LED_TANK_LOW = 1;
            alarm();
        }
        else if (!TANK_LOW && motor_on) { LED_TANK_LOW = 1; }
        else { LED_TANK_LOW = 0; }
       
        
        if (motor_on) {
            pump_led_blink();
            run_starter_relay();
            
            if (dry_run_check()) // Dry run detected
            {
                LED_DRY_RUN = 1;
                LED_TANK_LOW = 0;

                alarm();
                on = 0; toggle_motor();
            }
        }

        __delay_ms(5); // Small loop delay to reduce CPU load and stabilize loop timing
   }
}