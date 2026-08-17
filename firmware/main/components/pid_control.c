// PID variables
#include "pid_control.h"

float kp = 8.0f; // Constante Proporcional
float ki = 0.5f; // Constante Integral
float kd = 0.0f; // Constante Derivativa
float dt = 1.0f; // Diferencial del tiempo

float integral = 0;
float error_anterior = 0;

/*
Calcula el PWM deseado dependiendo de las mediciones.
Args:
    setpoint (float): El valor objetivo seleccionado desde NODE-RED
    measure (float): Medición por parte del termocouple
Return:
    El porcentaje del PWM en un rango de -100 a 100.
*/

float PID(float setpoint, float measure)
{

    // If error is positive, then the system is lower to setpoint. Need to increased.
    // If error is negative, then the system is hight to setpoint. Need to decreased.
    float error = setpoint - measure;

    /*
    Detecta si el error ha cambiado de signo entre el ciclo anterior y el ciclo actual
        El primer parametro del operador OR (||) compara si el error actual es positivo y el error anterior es negativo
        El segundo parametro del operador OR (||) compara si el error actual es negativo y el error anterior es positivo
    */
    if ((error > 0 && error_anterior < 0) || (error < 0 && error_anterior > 0))
    {
        // Evita que la integral acumule si measure tarda en alcanzar el setpoint (Anti-Windup por cruce por cero)
        integral = 0; // Reseteo de la integral.
    }

    float p_term = kp * error;

    integral += error * dt;

    // Limitar la integral que no sobrepase de 100.
    if (integral > 100)
    {
        integral = 100;
    }
    else if (integral < -100)
    {
        integral = -100;
    }

    float i_term = ki * integral;

    float d_term = 0;

    if (dt > 0.0000001f) // Evita división por cero.
    {
        float derivative = (error - error_anterior) / dt;
        d_term = kd * derivative;
    }

    float output = p_term + i_term + d_term;

    // Limitar output hasta 100
    if (output > 100)
    {
        output = 100;
    }
    else if (output < -100)
    {
        output = -100;
    }

    error_anterior = error;

    return output;
}

void pwm_init(void)
{

    gpio_set_direction(9, GPIO_MODE_OUTPUT);
    gpio_set_level(9, 0);
    gpio_set_direction(8, GPIO_MODE_OUTPUT);
    gpio_set_level(8, 0);

    // CONFIGURACIÓN DE TIMER 0 (Para el Cartucho Calefactor)
    ledc_timer_config_t timer_heating = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_10_BIT,
        .freq_hz = 100,
        .timer_num = LEDC_TIMER_0,
        .clk_cfg = LEDC_AUTO_CLK};

    ESP_ERROR_CHECK(ledc_timer_config(&timer_heating));

    ledc_channel_config_t heating_cartridge_channel = {
        .gpio_num = 9,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0, // Canal 0
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_0, // Enlazado al Timer 0 (100 Hz)
        .duty = 0,
        .hpoint = 0,
        .flags.output_invert = 0,
    };

    // CONFIGURACIÓN DE TIMER 1 (Ventilador)
    ledc_timer_config_t timer_cooler = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_10_BIT,
        .freq_hz = 25000,
        .timer_num = LEDC_TIMER_1,
        .clk_cfg = LEDC_AUTO_CLK};
    ESP_ERROR_CHECK(ledc_timer_config(&timer_cooler));

    ledc_channel_config_t cooler_channel = {
        .gpio_num = 8,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_1, // Canal 1
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_1, // Enlazado al Timer 1 (25 kHz)
        .duty = 0,
        .hpoint = 0,
        .flags.output_invert = 0,
    };

    // Aplicar configuración de canales
    ESP_ERROR_CHECK(ledc_channel_config(&heating_cartridge_channel));
    ESP_ERROR_CHECK(ledc_channel_config(&cooler_channel));
}

/*
Aplica el PID mandando señales PWM modificando su duty cycle.
El microcontrolador (ESP32) los envia al gate del mosfet.

Args:
    out_pid (float): La salida del pit entre un rango de -100 a 100.
*/
void apply_pid(float out_pid)
{
    if (out_pid > 0)
    {
        uint16_t duty_heating_cartridge = (uint16_t)(out_pid * 10.23); // Se multiplica la salida por un rango de 0-1023 (LEDC_TIMER_10_BIT) y se convierte a uint16_t
        // Establecer duty del calefactor
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty_heating_cartridge);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);

        // Apagar el ventilador
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, 0);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1);
    }
    else if (out_pid < 0)
    {
        // Apagar el cartucho calefactor
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);

        uint16_t duty_cooler = (uint16_t)(-out_pid * 10.23); // Se multiplica la salida negativa por un rango de 0-1023 (LEDC_TIMER_10_BIT) y se convierte a uint16_t
                                                             // Al haberse multiplicado con un signo negativo, el resultado es un valor positivo.

        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, duty_cooler);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1);
    }
    else if (out_pid == 0)
    {
        // Apagar el cartucho calefactor
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
        // Apagar el ventilador.
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, 0);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1);
    }
}