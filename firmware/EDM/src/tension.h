#ifndef __TENSION_H__
#define __TENSION_H__
#include "core.h"

void tension_init();
void tension_start();
void tension_stop();
void tension_process();

uint16_t tension_get_feeder_period_us();
uint16_t tension_get_brake_period_us();
uint16_t tension_get_tension_g();


#endif // __TENSION_H__