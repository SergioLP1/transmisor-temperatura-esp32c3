#ifndef PID_CONTROL_H
#define PID_CONTROL_H

#include "driver/ledc.h"
#include "esp_log.h"
#include "driver/gpio.h"

void pwm_init(void);

void apply_pid(float out_pid);

float PID(float setpoint, float measure);

#endif