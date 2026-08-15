#include "mqtt_configuration.h"

// ============================================================
// DEFINICIONES DE VARIABLES GLOBALES
// ============================================================
bool is_heating = false;
bool system_power = false;
uint16_t setpoint = 20;
esp_mqtt_client_handle_t mqtt_client = NULL; // ← Variable global

static const char *TAG = "MQTT";

/*
HANDLER DE EVENTOS MQTT

Return:
    Retorna una respuesta dependiendo el MQTT event types
*/

void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;

    switch ((esp_mqtt_event_id_t)event_id)
    {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "✅ MQTT Conectado al broker");

        esp_mqtt_client_subscribe(mqtt_client, "furnace/setpoint", 1);
        esp_mqtt_client_subscribe(mqtt_client, "furnace/command", 1);
        ESP_LOGI(TAG, "📥 Suscrito a furnace/setpoint y furnace/command");
        break;

    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGW(TAG, "❌ MQTT Desconectado");
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "✅ Suscrito, msg_id=%d", event->msg_id);
        break;

    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "Desuscrito, msg_id=%d", event->msg_id);
        break;

    case MQTT_EVENT_PUBLISHED:
        ESP_LOGD(TAG, "Publicado, msg_id=%d", event->msg_id);
        break;

    // ============================================================
    // RECEPCIÓN DE DATOS: Actualiza el valor del setpoint y comandos
    // ============================================================
    case MQTT_EVENT_DATA:
    {
        char *payload = (char *)event->data;
        int payload_len = event->data_len;

        char received[64];
        int len = payload_len < sizeof(received) - 1 ? payload_len : sizeof(received) - 1;
        strncpy(received, payload, len);
        received[len] = '\0';

        // PROCESAR SETPOINT
        if (strcmp(event->topic, "furnace/setpoint") == 0)
        {
            uint16_t new_setpoint = atoi(received);

            // El setpoint debe de ser entre 30 hasta 400 (grados)
            if (new_setpoint >= 30 && new_setpoint <= 400)
            {
                setpoint = new_setpoint;
                ESP_LOGI(TAG, "🎯 Setpoint actualizado: %d°C", setpoint);

                // Confirmar recepción
                char ack[16];
                snprintf(ack, sizeof(ack), "%d", setpoint);
                esp_mqtt_client_publish(mqtt_client, "furnace/setpoint_ack", ack, 0, 1, 0);
            }
            else
            {
                ESP_LOGW(TAG, "⚠️ Setpoint fuera de rango: %d (0-400)", new_setpoint);
            }
        }
        // PROCESAR COMANDO
        else if (strcmp(event->topic, "furnace/command") == 0)
        {
            ESP_LOGI(TAG, "📟 Comando recibido: '%s'", received);

            if (strcmp(received, "ON") == 0)
            {
                system_power = true;
                ESP_LOGI(TAG, "🔛 Sistema ENCENDIDO");
            }
            else if (strcmp(received, "OFF") == 0)
            {
                system_power = false;
                ESP_LOGI(TAG, "🔴 Sistema APAGADO");
            }
            else
            {
                ESP_LOGW(TAG, "⚠️ Comando desconocido: '%s'", received);
            }
        }
        break;
    }

    case MQTT_EVENT_ERROR:
        ESP_LOGE(TAG, "⚠️ Error MQTT");
        break;

    default:
        ESP_LOGI(TAG, "Otro evento: %d", event->event_id);
        break;
    }
}

// ============================================================
// INICIALIZACIÓN MQTT
// ============================================================
esp_mqtt_client_handle_t mqtt_init(void)
{
    if (mqtt_client != NULL)
    {
        ESP_LOGW(TAG, "MQTT ya inicializado");
        return mqtt_client;
    }

    esp_mqtt_client_config_t mqtt_config = {
        .broker.address.uri = "mqtt://192.168.1.101", // Dirección IP del broker. (Puede ser modificado)
        .broker.address.port = 1883,
        .session.protocol_ver = MQTT_PROTOCOL_V_3_1_1,
        .network.timeout_ms = 10000,
        .session.keepalive = 60,
        .session.disable_clean_session = false,
    };

    // Crear cliente
    mqtt_client = esp_mqtt_client_init(&mqtt_config);

    if (mqtt_client == NULL)
    {
        ESP_LOGE(TAG, "❌ Error al inicializar MQTT");
        return NULL;
    }

    // Registrar el evento. Este evento es el que analiza la función mqtt_event_handler que se encuentra como parametro de esta funcion.
    esp_err_t err = esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID,
                                                   mqtt_event_handler, NULL);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "❌ Error al registrar evento: %s", esp_err_to_name(err));
        return NULL;
    }

    // Iniciar cliente
    err = esp_mqtt_client_start(mqtt_client);

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "❌ Error al iniciar MQTT: %s", esp_err_to_name(err));
        return NULL;
    }

    ESP_LOGI(TAG, "✅ MQTT iniciado correctamente");
    return mqtt_client;
}