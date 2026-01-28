#ifndef PINS_H
#define PINS_H

// ===== Sensor Inputs =====
#define TANK_FULL      RA1
#define TANK_LOW       RA2
#define DRY_RUN        RA3
#define SUMP_HIGH      RA4
#define SUMP_LOW       RA5

// ===== Control Input =====
#define RESET_SW       RC1

// ===== LED Outputs =====
#define LED_TANK_FULL  RB7
#define LED_PUMP_ON    RB6
#define LED_TANK_LOW   RB5
#define LED_DRY_RUN    RB4
#define LED_SUMP_LOW   RB3
#define LED_HIGH_VOLT  RB2
#define LED_LOW_VOLT   RB1

// ===== Outputs =====
#define BUZZER         RC6
#define RELAY_3        RC5
#define RELAY_2        RC4
#define RELAY_1        RC3

#endif
