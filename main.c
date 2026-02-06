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
#pragma config CP    = ON
#pragma config CPD   = OFF
#pragma config BOREN = ON
#pragma config IESO  = OFF
#pragma config FCMEN = OFF
#pragma config LVP   = OFF



// =============== VARIABLES =============== //
unsigned int blink_cnt            = 0;
unsigned int motor_on             = 0;
unsigned int relay_timer          = 0;
unsigned char relay_state         = 0;
unsigned int dry_run_timer        = 0;
unsigned long dry_run_reset_timer = 0;
unsigned int dry_run_latched      = 0;
unsigned int sump_low             = 0;
unsigned int sump_low_latched     = 0;
unsigned int tank_full_delay_cnt  = 0;
unsigned int tank_full            = 0;


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

void alarm(unsigned int type)  // type - 0: normal, 1: warning
{
    if (type) {
        for (unsigned int i = 0; i < 4; i++)
        { BUZZER = 1; __delay_ms(1000); BUZZER = 0; __delay_ms(700); }
    }
    
    else { 
        for (unsigned int i = 0; i < 3; i++)
        { BUZZER = 1; __delay_ms(400); BUZZER = 0; __delay_ms(400); }
    }
}


void led_blink(void)
{ 
    if (blink_cnt >= DELAY_LED_BLINK) {
        if (motor_on) { LED_PUMP_ON ^= 1; } else { LED_PUMP_ON = 0; }
        if (sump_low) { LED_SUMP_LOW ^= 1; } else { LED_SUMP_LOW = 0; }
        blink_cnt = 0;
    }
}


void run_starter_relay(void)
{
    switch (relay_state)
    {
        case 0: // 3 Seconds waiting
            if (relay_timer >= DELAY_STARTER_RELAY)
            {
                relay_timer = 1;
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


void reset_dry_run(void) 
{
    dry_run_timer = 0;
    dry_run_latched = 0;
    dry_run_reset_timer = 0;
    LED_DRY_RUN = 0;
}


void toggle_motor(unsigned char on)
{
    LED_PUMP_ON   = on ? 1 : 0;
    RELAY_1       = on ? 1 : 0;
    motor_on      = on;
    
    reset_starter_relay();
    
    if (motor_on) { reset_dry_run(); sump_low_latched = 0; }
}


typedef struct {
    unsigned int count;
    unsigned int idx;
    unsigned char triggered;
} sensor_filter_t;

sensor_filter_t tank_full_filt = {0};
sensor_filter_t tank_low_filt = {0};
sensor_filter_t sump_low_filt = {0};

// The function checks the sensor over 10 readings and reports it as triggered if even one reading is HIGH, 
// otherwise it reports not triggered.
unsigned char is_triggered(sensor_filter_t *s, unsigned char input)
{
    s->count += !input;   
    s->idx++;

    if (s->idx >= 10)
    {
        s->idx = 0;
        s->triggered = (s->count > 0);  // any HIGH = triggered
        s->count = 0;
    }

    return s->triggered;
}


void init_level_sensors(void)
{
    for (unsigned int i = 0; i < 10; i++) {
        is_triggered(&tank_full_filt, TANK_FULL);
        is_triggered(&tank_low_filt, TANK_LOW);
        is_triggered(&sump_low_filt, SUMP_LOW);
        __delay_ms(5);
    }
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
        
        
        if (tank_full) {tank_full_delay_cnt++; }
        
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
    
    init_level_sensors();
    
    while(1)    
    {
        // Read TANK FULL, LOW, SUMP LOW sensors and set LEDs after the tank full led delay
        if (tank_full_delay_cnt >= DELAY_LED_TANK_FULL || !tank_full_delay_cnt) 
        { 
            LED_TANK_FULL = is_triggered(&tank_full_filt, TANK_FULL) ? 1 : 0;
            LED_TANK_LOW = is_triggered(&tank_low_filt, TANK_LOW) ? 1 : 0;
            sump_low = !is_triggered(&sump_low_filt, SUMP_LOW);
            if (sump_low) { sump_low_latched = 1; }
        }
        
        led_blink();

        if (tank_full) {
            
            // Handle Tank Full Delay
            if (tank_full_delay_cnt >= DELAY_TANK_FULL)
            {
                tank_full = 0;
                tank_full_delay_cnt = 0;
                reset_starter_relay();
            }
            
            // Handle RESET
            else if (!RESET_SW) {
                reset_starter_relay();
                toggle_motor(1); alarm(0);
                tank_full = 0;
                tank_full_delay_cnt = 0;
            }
            
            __delay_ms(10);
            continue;
        }
  
        if (motor_on)
        {
            run_starter_relay();
            
            // Handle Tank full 
            if (!TANK_FULL) {
                toggle_motor(0);
                dry_run_latched = 0;

                LED_TANK_FULL = 1;
                LED_TANK_LOW = 1;
                tank_full = 1;
                alarm(0);

                tank_full_delay_cnt = 1;
                continue;
            }
            
            // Handle Dry Run
            if (DRY_RUN) {
                LED_DRY_RUN = dry_run_check() ? 1: 0;
                if (LED_DRY_RUN) { toggle_motor(0); alarm(1);  continue; }
            } 
            else { dry_run_timer = 0; dry_run_latched = 1; }
            
            
            // Handle Sump Low
            if (sump_low) {
                LED_SUMP_LOW = 1;
                toggle_motor(0); alarm(1); sump_low_latched = 1;
            }
           
        }
        
        else 
        {
            if (!sump_low) {
                
                // Handle Reset switch trigger
                if (!RESET_SW) { toggle_motor(1); alarm(0); continue; } 
                
                // TANK LOW and DRY RUN auto reset only works if not off by sump low
                if (!sump_low_latched) {
            
                    // Handle Dry Run
                    if (LED_DRY_RUN)
                    {
                        if (dry_run_reset_timer >= DELAY_DRY_RUN_RESET) {
                            toggle_motor(1); alarm(0);
                        }
                    }

                    // Handle Tank Low 
                    else if (!LED_TANK_LOW) { toggle_motor(1); alarm(0);  }
                }
                
                else if (!SUMP_HIGH) {
                    toggle_motor(1); alarm(0); 
                }
            } 
        }

        __delay_ms(5); // Small loop delay to reduce CPU load and stabilize loop timing
   }
}
