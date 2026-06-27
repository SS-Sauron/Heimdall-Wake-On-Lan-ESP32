#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    STATUS_LED_STATE_OFF = 0,
    STATUS_LED_STATE_PORTAL,      /* Fast blink (e.g., 200ms) */
    STATUS_LED_STATE_CONNECTING,  /* Slow pulse (e.g., 1000ms) */
    STATUS_LED_STATE_READY,       /* Solid ON */
    STATUS_LED_STATE_WOL_SENT     /* Rapid flash (e.g., 50ms) then back to READY */
} status_led_state_t;

/**
 * @brief Initialize the status LED component.
 * 
 * If CONFIG_WOL_STATUS_LED is disabled, this is a no-op.
 * 
 * @return ESP_OK on success.
 */
esp_err_t status_led_init(void);

/**
 * @brief Set the current state of the status LED.
 * 
 * @param state The desired state.
 */
void status_led_set_state(status_led_state_t state);

#ifdef __cplusplus
}
#endif
