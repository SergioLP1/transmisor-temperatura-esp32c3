#include <stdio.h>
#include <esp_log.h>
#include <esp_wifi.h>
#include <stdlib.h>
#include <string.h>
#include "driver/gpio.h"

#include "temperature_converter.h"
#include "pid_control.h"
#include "mqtt_configuration.h"
#include "wifi_connect.h"

/*Size Buffer*/
#define SB_TEMPERATURE 20
#define SB_PWM_HEAT 15
#define SB_PWM_COOLER 15

/*Variables externas: El valor lo decide el usuario en la interfaz de NODE-RED*/
extern bool is_heating;
extern bool system_power;
extern uint16_t setpoint;

// ============================================================
// ESTRUCTURA DE DATOS
// ============================================================
typedef struct
{
    float temperature;
    uint32_t pwm_heat;
    uint32_t pwm_cooler;
} data_furnace_t;

// ============================================================
// PUBLICAR TODOS LOS DATOS
// ============================================================
void publish_all_data(esp_mqtt_client_handle_t client, data_furnace_t *data)
{
    if (client == NULL)
    {
        ESP_LOGE("MQTT", "❌ MQTT no inicializado");
        return;
    }

    char temperature_buffer[SB_TEMPERATURE];
    char pwm_heat_buffer[SB_PWM_HEAT];
    char pwm_cooler_buffer[SB_PWM_COOLER];

    // Convertir datos en string.
    snprintf(temperature_buffer, SB_TEMPERATURE, "%.2f", data->temperature);
    snprintf(pwm_heat_buffer, SB_PWM_HEAT, "%ld", data->pwm_heat);
    snprintf(pwm_cooler_buffer, SB_PWM_COOLER, "%ld", data->pwm_cooler);

    esp_mqtt_client_publish(client, "furnace/temperature", temperature_buffer, 0, 0, 0);
    esp_mqtt_client_publish(client, "furnace/pwm_heat", pwm_heat_buffer, 0, 0, 0);
    esp_mqtt_client_publish(client, "furnace/pwm_cooler", pwm_cooler_buffer, 0, 0, 0);

    esp_mqtt_client_publish(client, "furnace/status", is_heating ? "HEATING" : "NOT ACTIVE", 0, 0, 0);
    esp_mqtt_client_publish(client, "furnace/power", system_power ? "ON" : "OFF", 0, 0, 0);

    ESP_LOGI("MQTT", "📊 T:%.2f | H:%ld | C:%ld | SP:%d",
             data->temperature, data->pwm_heat, data->pwm_cooler, setpoint);
}

// ============================================================
// INITS
// ============================================================
void system_init()
{
    pwm_init();
    max6675_init();
    wifi_init();
}

// ============================================================
// MAIN
// ============================================================
void app_main(void)
{
    ESP_LOGI("MAIN", "🚀 Sistema iniciando...");

    system_init();
    wait_for_wifi(); // Espera a que se conecte a la red wifi

    esp_mqtt_client_handle_t mqtt_client = mqtt_init();

    if (mqtt_client == NULL)
    {
        ESP_LOGE("MAIN", "❌ MQTT no iniciado");
    }

    data_furnace_t data = {0};
    float out_pid = 0;

    while (1)
    {
        // Leer temperatura del termocouple
        data.temperature = max6675_read_temperature();

        // Procesa el PID solo si system_power = true
        if (system_power)
        {
            out_pid = PID(setpoint, data.temperature);
            apply_pid(out_pid);
            is_heating = (out_pid > 0);
        }
        else
        {
            // Si esta apagado
            apply_pid(0); // No aplica PID. Apaga los actuadores
            is_heating = false;
        }

        // Leer PWM mediante la función ledc_get_duty. Se convierte el resultado de bits a porcentaje.
        data.pwm_heat = (uint32_t)((ledc_get_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0) * 100) / 1023);
        data.pwm_cooler = (uint32_t)((ledc_get_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1) * 100) / 1023);

        // Publicar los datos
        publish_all_data(mqtt_client, &data);

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}