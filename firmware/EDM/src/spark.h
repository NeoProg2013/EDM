#ifndef __SPARK_H__
#define __SPARK_H__
#include "Arduino.h"


void spark_pwm_init();
void spark_pwm_start();
void spark_pwm_stop();
void spark_pwm_update();
void spark_pwm_process();

void spark_set_t1_us(int32_t t);
void spark_set_t0_us(int32_t t);
int32_t spark_get_t1_us();
int32_t spark_get_t0_us();
int32_t spark_get_freq();

bool spark_is_enabled();


#endif // __SPARK_H__