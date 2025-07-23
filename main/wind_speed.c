#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"

#include "wind_speed.h"

static volatile uint32_t pulse_count = 0;
// Interrupt Service Routine
static void IRAM_ATTR wind_isr_handler(void *arg)
{
    pulse_count++;
}

void wind_speed_init()
{
    gpio_config(&(gpio_config_t){
        .intr_type = GPIO_INTR_POSEDGE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << WIND_SPEED_GPIO),
        .pull_up_en = 1,
    });
    gpio_install_isr_service(0);
    gpio_isr_handler_add(WIND_SPEED_GPIO, wind_isr_handler, NULL);
}

float get_wind_speed()
{
    pulse_count = 0;
    vTaskDelay(pdMS_TO_TICKS(1000));
    if (pulse_count != 0)
    {
        return (pulse_count * 2.73f + 0.53) * 0.44704; // meters per second
    }
    return 0.0f;
}