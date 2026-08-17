#include <stdint.h>
#include "driver/spi_master.h"
#include "hal/spi_types.h"
#include "esp_log.h"
#include <math.h>

#define MAX6675_MISO_GPIO 5
#define MAX6675_SCK_GPIO 4
#define MAX6675_CS_GPIO 7

void max6675_init(void);

float max6675_read_temperature(void);
