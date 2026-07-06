#ifndef __TENSION_H__
#define __TENSION_H__
#include <Arduino.h>


void tension_init();
void tension_start();
void tension_stop();
void tension_process();

int32_t tension_get_feeder_freq();
int32_t tension_get_brake_freq();
int32_t tension_get_tension_bins();
int32_t tension_get_tension_g();


#endif // __TENSION_H__