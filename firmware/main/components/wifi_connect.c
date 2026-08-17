#include "wifi_connect.h"

#define SSID_WIFI "inserta_tu_SSID"           // SSID de la red de WiFi (Puede ser modificado)
#define PASSWORD_WIFI "inserta_tu_contraseña" // Contraseña de la red de WiFi (Puede ser modificado)

#define MAXIMUM_RETRY 15 // Intentos para reconectarse.

bool wifi_connected = false;

/*
HANDLER DE EVENTOS WIFI

Return:
    Retorna una respuesta dependiendo de los eventosd el WiFi
*/

void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    static uint8_t retry_num = 0;

    if (event_base == WIFI_EVENT)
    {
        switch (event_id)
        {
        case WIFI_EVENT_STA_START:
            ESP_LOGI("WIFI", "WiFi iniciado. Conectando...");
            break;

        case WIFI_EVENT_STA_STOP:
            ESP_LOGI("WIFI", "WiFi detenido");
            break;

        case WIFI_EVENT_STA_CONNECTED:
            ESP_LOGI("WIFI", "The WiFi ha sido conectado.");
            retry_num = 0;
            break;

        case WIFI_EVENT_STA_DISCONNECTED:
            ESP_LOGI("WIFI", "WiFi esta desconectado.");

            if (retry_num < MAXIMUM_RETRY)
            {
                ESP_LOGI("WIFI", "Intentado conectar al AP... Intento %d", retry_num + 1);
                esp_wifi_connect();
                retry_num++;
            }
            else
            {
                ESP_LOGW("WIFI", "WiFi no puede ser reconectado. Máximos intentos alcanzados.");
            }
            break;
        }
    }
    else if (event_base == IP_EVENT)
    {
        if (event_id == IP_EVENT_STA_GOT_IP)
        {
            ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
            ESP_LOGI("WIFI-IP", "IP obtenida: " IPSTR, IP2STR(&event->ip_info.ip));
            wifi_connected = true;
        }
    }
}

void wifi_init(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    esp_netif_init();

    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    // Monitorea todo lo relacionado con los eventos del wifi.
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));

    wifi_init_config_t init_cfg = WIFI_INIT_CONFIG_DEFAULT();

    wifi_config_t wifi_config = {
        .sta = {

            .ssid = SSID_WIFI,
            .password = PASSWORD_WIFI,
            .scan_method = WIFI_FAST_SCAN,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_init(&init_cfg));

    // Configurar modo estación. Para conectarse a una red WiFi
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));

    // Configuración optimizada para ESP32-C3
    esp_wifi_set_ps(WIFI_PS_NONE); // Sin ahorro de energía
    esp_wifi_set_max_tx_power(34); // 8.5 dBm

    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_connect());
}

bool is_wifi_connected(void)
{
    return wifi_connected;
}

// Función para esperar conexión WiFi
void wait_for_wifi(void)
{
    ESP_LOGI("WIFI", "Esperando conexión WiFi...");
    int timeout = 30; // 30 segundos máximo

    while (!wifi_connected && timeout > 0)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
        timeout--;
        ESP_LOGI("WIFI", "Esperando... %d segundos restantes", timeout);
    }

    if (wifi_connected)
    {
        ESP_LOGI("WIFI", "✅ WiFi listo!");
    }
    else
    {
        ESP_LOGE("WIFI", "❌ Tiempo de espera acabado! Continuando sin WiFi...");
    }
}