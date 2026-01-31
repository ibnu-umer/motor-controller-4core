#include "config.h"

#ifndef GLOBALS_H
#define GLOBALS_H

extern unsigned int blink_cnt;
extern __bit blink_state;
extern unsigned int motor_on;
extern unsigned int relay_timer;
extern unsigned char relay_state;
extern unsigned int dry_run_timer;
extern unsigned int dry_run_latched;
extern unsigned int sump_low_latched;

#endif