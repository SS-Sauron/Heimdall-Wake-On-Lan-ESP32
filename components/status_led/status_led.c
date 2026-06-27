#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "status_led.h"
#include "sdkconfig.h"

static const char *TAG = "status_led";

#if CONFIG_WOL_STATUS_LED

static volatile status_led_state_t s_current_state = STATUS_LED_STATE_OFF;
static TaskHandle_t s_led_task_handle = NULL;

static void led_blink_task(void *arg)
{
    int pin = CONFIG_WOL_STATUS_LED_PIN;
    int level = 0;
    
    while (1) {
        status_led_state_t state = s_current_state;
        
        if (state == STATUS_LED_STATE_OFF) {
            gpio_set_level((gpio_num_t)pin, 0);
            vTaskDelay(pdMS_TO_TICKS(500));
        } else if (state == STATUS_LED_STATE_READY) {
            gpio_set_level((gpio_num_t)pin, 1);
            vTaskDelay(pdMS_TO_TICKS(500));
        } else if (state == STATUS_LED_STATE_PORTAL) {
            level = !level;
            gpio_set_level((gpio_num_t)pin, level);
            vTaskDelay(pdMS_TO_TICKS(200)); /* Fast blink */
        } else if (state == STATUS_LED_STATE_CONNECTING) {
            level = !level;
            gpio_set_level((gpio_num_t)pin, level);
            vTaskDelay(pdMS_TO_TICKS(1000)); /* Slow pulse */
        } else if (state == STATUS_LED_STATE_WOL_SENT) {
            /* Rapid flash sequence */
            for (int i = 0; i < 6; i++) {
                level = !level;
                gpio_set_level((gpio_num_t)pin, level);
                vTaskDelay(pdMS_TO_TICKS(50));
            }
            /* Automatically return to READY state after flashing */
            s_current_state = STATUS_LED_STATE_READY;
        } else {
            vTaskDelay(pdMS_TO_TICKS(500));
        }
    }
}

esp_err_t status_led_init(void)
{
    int pin = CONFIG_WOL_STATUS_LED_PIN;
    if (pin < 0 || pin >= GPIO_NUM_MAX) {
        ESP_LOGE(TAG, "Invalid status LED pin: %d", pin);
        return ESP_ERR_INVALID_ARG;
    }

    gpio_reset_pin((gpio_num_t)pin);
    gpio_set_direction((gpio_num_t)pin, GPIO_MODE_OUTPUT);
    gpio_set_level((gpio_num_t)pin, 0);

    BaseType_t res = xTaskCreate(led_blink_task, "led_task", 2048, NULL, 5, &s_led_task_handle);
    if (res != pdPASS) {
        ESP_LOGE(TAG, "Failed to create led_task");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Status LED initialized on GPIO %d", pin);
    return ESP_OK;
}

void status_led_set_state(status_led_state_t state)
{
    s_current_state = state;
}

#else

esp_err_t status_led_init(void)
{
    ESP_LOGD(TAG, "Status LED is disabled via Kconfig");
    return ESP_OK;
}

void status_led_set_state(status_led_state_t state)
{
    (void)state;
}

#endif
