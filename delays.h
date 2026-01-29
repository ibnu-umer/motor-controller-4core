#include "config.h"


#ifndef DELAYS_H
#define DELAYS_H


#if defined(PRODUCTION)

// PRODUCTION LEVEL TIMER DELAYS
#define DELAY_START_UP          20000
#define DELAY_STARTER_RELAY     600
#define DELAY_STARTED_RELAY_ON  600
#define DELAY_DRY_RUN           30000
#define DELAY_DRY_RUN_AFTER     3000
#define DELAY_DRY_RUN_RESET     30000
#define DELAY_TANK_FULL         300000

#else   // TESTING

#define DELAY_START_UP          5000
#define DELAY_STARTER_RELAY     30     // 30 x 100ms = 3s
#define DELAY_STARTED_RELAY_ON  30
#define DELAY_DRY_RUN           200    // 100 x 200ms = 20s
#define DELAY_DRY_RUN_AFTER     100    // 10s
#define DELAY_DRY_RUN_RESET     600    // 1m
#define DELAY_TANK_FULL         15000
#define DELAY_LED_PUMP_ON       5

#endif

#endif
