#include "temperature_converter.h"

static spi_device_handle_t max6675_spi_handle = NULL;

static const char *TAG = "MAX6675";

void max6675_init(void)
{
    spi_bus_config_t bus_config = {
        .mosi_io_num = -1,
        .quadhd_io_num = -1,
        .quadwp_io_num = -1,
        .miso_io_num = MAX6675_MISO_GPIO,
        .sclk_io_num = MAX6675_SCK_GPIO,
        .max_transfer_sz = 16};

    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &bus_config, SPI_DMA_DISABLED));

    spi_device_interface_config_t device_config = {
        .clock_speed_hz = 1000000,
        .mode = 1,
        .spics_io_num = MAX6675_CS_GPIO,
        .queue_size = 1,
        .flags = 0,

        .command_bits = 0,
        .address_bits = 0,
        .dummy_bits = 0,
        .input_delay_ns = 0,
    };

    ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &device_config, &max6675_spi_handle));

    ESP_LOGI(TAG, "MAX6675 inicializado");
}

float max6675_read_temperature(void)
{
    if (max6675_spi_handle == NULL)
    {
        ESP_LOGE(TAG, "MAX6675 don't initialized");
    }

    uint8_t rx_data[2] = {0};

    spi_transaction_t transaction = {
        .length = 16,
        .rxlength = 16,
        .tx_buffer = NULL,
        .rx_buffer = rx_data};

    esp_err_t ret = spi_device_transmit(max6675_spi_handle, &transaction);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "❌ Error SPI: %s", esp_err_to_name(ret));
    }

    uint16_t raw_value = ((uint16_t)rx_data[0] << 8) | rx_data[1];

    // CONVERTIR A TEMPERATURA
    uint16_t temperature_bits = raw_value >> 3;
    float temperature_celsius = temperature_bits * 0.25f;

    return temperature_celsius;
}