#ifndef MQTT_CONFIGURATION_H
#define MQTT_CONFIGURATION_H

#include "mqtt_client.h"
#include "esp_log.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

extern bool is_heating;
extern bool system_power;
extern uint16_t setpoint;
extern esp_mqtt_client_handle_t mqtt_client;

esp_mqtt_client_handle_t mqtt_init(void);

#endif