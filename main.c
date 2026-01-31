#include <xc.h>
#include "config.h"
#include "pins.h"
#include "globals.h"
#include "delays.h"

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
unsigned int motor_on = 0;
unsigned int relay_timer = 0;
unsigned char relay_state = 0;
unsigned int dry_run_timer = 0;
unsigned int dry_run_reset_timer = 0;
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

void alarm(unsigned int type)  // type - 0: normal, 1: warning
{
    if (type) {
        for (unsigned int i = 0; i < 5; i++)
        { BUZZER = 1; __delay_ms(700); BUZZER = 0; __delay_ms(700); }
    }
    
    else { 
        for (unsigned int i = 0; i < 3; i++)
        { BUZZER = 1; __delay_ms(400); BUZZER = 0; __delay_ms(400); }
    }
}


void pump_led_blink(void)
{ 
    if (blink_cnt >= DELAY_LED_PUMP_ON)
    { blink_cnt = 0; LED_PUMP_ON ^= 1;  }
}


void run_starter_relay(void)
{
    switch (relay_state)
    {
        case 0: // 3 Seconds waiting
            if (relay_timer >= DELAY_STARTER_RELAY)
            {
                relay_timer = 0;
                RELAY_2 = 1;
                RELAY_3 = 1;
                relay_state = 1;
            }
            break;

        case 1: // Relay on for 3 seconds
            if (relay_timer >= DELAY_STARTED_RELAY_ON)
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
    if (dry_run_latched && dry_run_timer >= DELAY_DRY_RUN_AFTER) { return 1;} // Lost to dry run after 

    if (dry_run_timer >= DELAY_DRY_RUN) { return 1; } // Dry run detected
    
    return 0;
}


void toggle_motor(unsigned char on)
{
    LED_PUMP_ON = on ? 1 : 0;
    RELAY_1     = on ? 1 : 0;
    motor_on    = on;
    
    reset_starter_relay();
    
    if (motor_on) { // If motor turned on, reset dry run
        dry_run_timer = 0;
        dry_run_latched = 0;
        dry_run_reset_timer = 0;
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


// =============== TIMER ================== //

void __interrupt() isr(void)
{
    if (PIR1bits.TMR1IF) {
        PIR1bits.TMR1IF = 0;
        TMR1 = TMR1_RELOAD;

        // Increment counters
        blink_cnt++;
        relay_timer++;
        
        if (DRY_RUN) { dry_run_timer++; }
        
        if (LED_DRY_RUN) { dry_run_reset_timer++; }
    }
}


void init_timer(void)
{
    T1CON = 0;
    T1CONbits.T1CKPS = 0b11;   // prescaler 1:8
    TMR1 = TMR1_RELOAD;

    PIR1bits.TMR1IF = 0;
    PIE1bits.TMR1IE = 1;

    INTCONbits.PEIE = 1;
    INTCONbits.GIE  = 1;

    T1CONbits.TMR1ON = 1;
}


// ================= MAIN ================= //

void main(void)
{
    OSCCON = 0b01110000;
    
    init_hw();
    init_timer();
    
    // Delay before starting
    __delay_ms(DELAY_START_UP); alarm(0);
    
    while(1)    
    {
        LED_TANK_LOW = tank_low_check() ? 1 : 0;
        
        
        
        if (motor_on)
        {
            pump_led_blink();
            run_starter_relay();
            
            
            // Handle Tank full 
            if (!TANK_FULL && relay_timer >= 30) {
                LED_TANK_LOW = 0;
                toggle_motor(0);
                dry_run_latched = 0;

                LED_TANK_FULL = 1;
                alarm(0);

                __delay_ms(DELAY_TANK_FULL); 
                LED_TANK_FULL = 0;

                reset_starter_relay();
                continue;
            }
            
            // Handle Dry Run
            if (DRY_RUN) {
                LED_DRY_RUN = dry_run_check() ? 1: 0;
                if (LED_DRY_RUN) { toggle_motor(0); alarm(1);  continue; }
            } 
            else { dry_run_timer = 0; dry_run_latched = 1; }

        }
        
        else 
        {
            // Handle Reset switch trigger
            if (!RESET_SW) { toggle_motor(1); alarm(0); } 
            
            // Handle Dry Run
            else if (LED_DRY_RUN)
            {
                if (dry_run_reset_timer >= DELAY_DRY_RUN_RESET) {
                    toggle_motor(1); alarm(0);
                }
            }

            // Handle Tank Low 
            else if (LED_TANK_LOW) { toggle_motor(1); alarm(0); }
        }

        __delay_ms(5); // Small loop delay to reduce CPU load and stabilize loop timing
   }
}