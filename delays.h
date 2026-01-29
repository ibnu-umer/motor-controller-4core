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
#define TANK_FULL_DELAY         300000

#else   // TESTING

#define DELAY_START_UP          5000
#define DELAY_STARTER_RELAY     600
#define DELAY_STARTED_RELAY_ON  600
#define DELAY_DRY_RUN           6000
#define DELAY_DRY_RUN_AFTER     2000
#define DELAY_DRY_RUN_RESET     30000
#define TANK_FULL_DELAY         15000

#endif

#endif
