#include <xc.h>
#include "pins.h"
#include "globals.h"
#include "delays.h"
#include "config.h"

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



// =============== VARIABLES =============== //
unsigned int blink_cnt = 0;
__bit blink_state = 0;
unsigned int motor_on = 0;
unsigned int relay_timer = 0;
unsigned char relay_state = 0;
unsigned int dry_run_timer = 0;
unsigned int dry_run_latched = 0;
unsigned int tank_low_all_zero = 0;
unsigned char tank_low_vec[TANK_LOW_VEC_SIZE];
unsigned char vec_idx = 0;



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
    
    for (unsigned char i = 0; i < TANK_LOW_VEC_SIZE; i++)
        tank_low_vec[i] = 1;  
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
        case 0: // 3 Seconds waiting
            if (++relay_timer >= DELAY_STARTER_RELAY)
            {
                relay_timer = 0;
                RELAY_2 = 1;
                RELAY_3 = 1;
                relay_state = 1;
            }
            break;

        case 1: // Relay on for 3 seconds
            if (++relay_timer >= DELAY_STARTED_RELAY_ON)
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
    
    if (dry_run_latched && dry_run_timer >= DELAY_DRY_RUN_AFTER) { return 1;} // Lost to dry run after 

    if (dry_run_timer >= DELAY_DRY_RUN) { return 1; } // Dry run detected
    
    if (DRY_RUN) { dry_run_timer++; }  // increase timer in DRY RUN not triggered
    else { dry_run_timer = 0; dry_run_latched = 1; }
    
    return 0;
}


void toggle_motor(void)
{
    LED_PUMP_ON = motor_on ? 1: 0;
    RELAY_1     = motor_on ? 1: 0;
    
    reset_starter_relay();
    
    if (motor_on) { // If motor turned on, reset dry run
        dry_run_timer = 0;
        LED_DRY_RUN = 0;
    }
}


unsigned int tank_low_check(void)
{
    unsigned char i;

    tank_low_vec[vec_idx] = !TANK_LOW;
    vec_idx++;
    if (vec_idx >= TANK_LOW_VEC_SIZE)
        vec_idx = 0;
        tank_low_all_zero = 1;
        
        for (i = 0; i < TANK_LOW_VEC_SIZE; i++)
        {
            // if vector contains any 1's
            if (tank_low_vec[i]) { tank_low_all_zero = 0; break; }
        }

    return tank_low_all_zero;   // 1: all zero, 0: not all zero
}


// ================= MAIN ================= //

void main(void)
{
    init_hw();
    
    // Delay before starting
    __delay_ms(DELAY_START_UP); alarm();
    
    while(1)
    {
        
        // Handle Reset switch trigger
        if (!RESET_SW && !motor_on) 
        {
            LED_DRY_RUN = 0;
            motor_on = 1; toggle_motor();
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
            motor_on = 0; toggle_motor();
            dry_run_latched = 0;

            LED_TANK_FULL = 1;
            alarm();

            __delay_ms(TANK_FULL_DELAY); 
            LED_TANK_FULL = 0;

            reset_starter_relay();
        }
       
        
        // Handle Tank Low 
        LED_TANK_LOW = tank_low_check() ? 1 : 0;
        if (LED_TANK_LOW && !motor_on) {
            motor_on = 1; toggle_motor();
            alarm();
        }

        
        if (motor_on) {
            pump_led_blink();
            run_starter_relay();
            
            if (dry_run_check()) // Dry run detected
            {
                LED_DRY_RUN = 1;
                LED_TANK_LOW = 0;

                alarm();
                motor_on = 0; toggle_motor();
            }
        }

        __delay_ms(5); // Small loop delay to reduce CPU load and stabilize loop timing
   }
}