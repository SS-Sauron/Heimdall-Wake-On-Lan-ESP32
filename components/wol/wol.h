/*
 * wol.h
 *
 * Wake-on-LAN Magic Packet builder and broadcaster.
 *
 * Accepts a target MAC address as a colon-separated string
 * (e.g. "AA:BB:CC:DD:EE:FF") or as a 6-byte array, constructs
 * the standard 102-byte Magic Packet, and broadcasts it over UDP
 * to the configured LAN broadcast address on port 9.
 */

#pragma once

#include "esp_err.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Send a Magic Packet to wake the machine with the given MAC address.
 *
 * Parses @p mac_str (format "AA:BB:CC:DD:EE:FF"), constructs the 102-byte
 * Magic Packet, and sends it as a UDP broadcast to
 * the broadcast address derived at send time from the STA interface IP and netmask.
 *
 * The broadcast is sent three times with a 10 ms gap to improve
 * delivery reliability on lossy local networks.
 *
 * @param mac_str  Colon-separated MAC string, e.g. "AA:BB:CC:DD:EE:FF".
 *                 Case-insensitive. NULL or malformed strings return
 *                 ESP_ERR_INVALID_ARG.
 * @return ESP_OK on success, or an error code.
 */
esp_err_t wol_send(const char *mac_str);

/**
 * @brief Raw variant — accepts a pre-parsed 6-byte MAC array.
 *
 * @param mac  6-byte MAC address in network byte order.
 * @return ESP_OK on success.
 */
esp_err_t wol_send_raw(const uint8_t mac[6]);

/**
 * @brief Parse a colon-separated MAC string into a 6-byte array.
 *
 * @param mac_str  Input string, e.g. "AA:BB:CC:DD:EE:FF".
 * @param mac_out  6-byte output buffer.
 * @return ESP_OK on success, ESP_ERR_INVALID_ARG on parse failure.
 */
esp_err_t wol_parse_mac(const char *mac_str, uint8_t mac_out[6]);

#if CONFIG_WOL_PING_FEEDBACK
#include "sdkconfig.h"
#include "mqtt_client.h"

/**
 * @brief Start a non-blocking ping session after a WoL packet.
 *
 * Returns immediately. Publishes to @p rsp_topic via MQTT when the
 * target responds ("awake") or when the count of pings is exhausted
 * ("timeout"). Total session duration ≈ CONFIG_WOL_PING_TIMEOUT_SEC s.
 *
 * @param target_ip     Dotted-decimal IPv4 string, e.g. "192.168.1.100".
 * @param mac           Target MAC address (6 bytes) for the response payload.
 * @param mqtt_client   Active MQTT client handle.
 * @param rsp_topic     Response topic string.
 * @param wol_send_time esp_timer_get_time() value recorded when WoL was sent.
 * @return ESP_OK on success.
 *         ESP_ERR_INVALID_STATE if a session is already active.
 *         Other esp_err_t on setup failure.
 */
esp_err_t wol_ping_start(const char *target_ip,
                          const uint8_t mac[6],
                          esp_mqtt_client_handle_t mqtt_client,
                          const char *rsp_topic,
                          int64_t wol_send_time);

/**
 * @brief Clean up any completed ping session resources.
 *
 * Must NOT be called from ping callbacks — call from the health monitor
 * loop in mqtt_relay.c. Safe to call when no session is active.
 */
void wol_ping_cleanup(void);
#endif /* CONFIG_WOL_PING_FEEDBACK */

#ifdef __cplusplus
}
#endif
