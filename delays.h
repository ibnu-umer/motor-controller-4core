#ifndef DELAYS_H
#define DELAYS_H

#define DELAY_STARTER_RELAY     30     // 30 x 100ms = 3s
#define DELAY_STARTED_RELAY_ON  30
#define DELAY_LED_BLINK         5 
#define DELAY_LED_TANK_FULL     200    // 20s


// PRODUCTION LEVEL TIMER DELAYS
#define DELAY_START_UP          20000   // 20s
#define DELAY_DRY_RUN           1500    // 2m 30s (150s x 100 tick)
#define DELAY_DRY_RUN_AFTER     200     // 20s
//#define DELAY_DRY_RUN_RESET     108000UL// 3h (600 (1m tick) x 180)
#define DELAY_DRY_RUN_RESET     54000UL   // 1h 30m (600 (1m tick) x 90)
#define DELAY_TANK_FULL         1800    // 3m

// TESTING LEVEL TIMER DELAYS
//#define DELAY_START_UP          5000   // 5s
//#define DELAY_DRY_RUN           200    // 100 x 200ms = 20s
//#define DELAY_DRY_RUN_AFTER     100    // 10s
//#define DELAY_DRY_RUN_RESET     600    // 1m
//#define DELAY_TANK_FULL         300    // 30s

#endif
