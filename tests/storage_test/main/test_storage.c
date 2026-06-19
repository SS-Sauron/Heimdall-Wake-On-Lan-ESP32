/*
 * test_storage.c
 *
 * Unity tests for the storage component.
 *
 * Each test gets a fully clean NVS partition:
 *   setUp()    – nvs_flash_erase() + nvs_flash_init()
 *   tearDown() – nvs_flash_deinit()
 *
 * This guarantees no state leaks between tests regardless of execution order.
 */

#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include "unity.h"
#include "nvs_flash.h"
#include "storage.h"

/* -------------------------------------------------------------------------
 * Fixtures
 * ------------------------------------------------------------------------- */

void setUp(void)
{
    /* Wipe the entire NVS flash partition, then re-initialise from scratch. */
    ESP_ERROR_CHECK(nvs_flash_erase());
    ESP_ERROR_CHECK(nvs_flash_init());
}

void tearDown(void)
{
    nvs_flash_deinit();
}

/* -------------------------------------------------------------------------
 * 1. load_hostname_empty_nvs
 *
 * On a fresh NVS partition the "wol" namespace does not exist.
 * storage_load_hostname() calls nvs_open(NS, NVS_READONLY) which returns
 * ESP_ERR_NVS_NOT_FOUND when the namespace has never been written.
 * That error is returned unchanged to the caller.
 * ------------------------------------------------------------------------- */
static void test_load_hostname_empty_nvs(void)
{
    char buf[STORAGE_HOSTNAME_MAX + 1];
    esp_err_t err = storage_load_hostname(buf, sizeof(buf));
    TEST_ASSERT_EQUAL(ESP_ERR_NVS_NOT_FOUND, err);
}

/* -------------------------------------------------------------------------
 * 2. save_and_load_hostname_roundtrip
 * ------------------------------------------------------------------------- */
static void test_save_and_load_hostname_roundtrip(void)
{
    const char *hostname = "relay-test";
    TEST_ASSERT_EQUAL(ESP_OK, storage_save_hostname(hostname));

    char buf[STORAGE_HOSTNAME_MAX + 1];
    memset(buf, 0, sizeof(buf));
    esp_err_t err = storage_load_hostname(buf, sizeof(buf));

    TEST_ASSERT_EQUAL(ESP_OK, err);
    TEST_ASSERT_EQUAL_STRING(hostname, buf);
}

/* -------------------------------------------------------------------------
 * 3. hostname_erased_by_erase_all
 *
 * After storage_erase_all() the "wol" namespace is empty. A subsequent
 * nvs_open(NVS_READONLY) will return ESP_ERR_NVS_NOT_FOUND because the
 * namespace has no entries and was erased.
 * ------------------------------------------------------------------------- */
static void test_hostname_erased_by_erase_all(void)
{
    /* First write something so the namespace exists */
    TEST_ASSERT_EQUAL(ESP_OK, storage_save_hostname("before-erase"));

    /* Erase the whole namespace */
    TEST_ASSERT_EQUAL(ESP_OK, storage_erase_all());

    /* Now load — namespace is gone */
    char buf[STORAGE_HOSTNAME_MAX + 1];
    esp_err_t err = storage_load_hostname(buf, sizeof(buf));
    TEST_ASSERT_EQUAL(ESP_ERR_NVS_NOT_FOUND, err);
}

/* -------------------------------------------------------------------------
 * 4. reboot_count_empty_nvs_returns_zero
 *
 * storage_get_reboot_count() returns ESP_OK and sets *count = 0 when the
 * key has never been written (defined contract — not an error).
 * ------------------------------------------------------------------------- */
static void test_reboot_count_empty_nvs_returns_zero(void)
{
    uint8_t count = 0xFF; /* poison value */
    esp_err_t err = storage_get_reboot_count(&count);
    TEST_ASSERT_EQUAL(ESP_OK, err);
    TEST_ASSERT_EQUAL_UINT8(0, count);
}

/* -------------------------------------------------------------------------
 * 5. reboot_count_roundtrip
 * ------------------------------------------------------------------------- */
static void test_reboot_count_roundtrip(void)
{
    TEST_ASSERT_EQUAL(ESP_OK, storage_set_reboot_count(5));

    uint8_t count = 0xFF;
    TEST_ASSERT_EQUAL(ESP_OK, storage_get_reboot_count(&count));
    TEST_ASSERT_EQUAL_UINT8(5, count);
}

/* -------------------------------------------------------------------------
 * 6. reboot_count_cleared_by_erase_all
 * ------------------------------------------------------------------------- */
static void test_reboot_count_cleared_by_erase_all(void)
{
    TEST_ASSERT_EQUAL(ESP_OK, storage_set_reboot_count(3));
    TEST_ASSERT_EQUAL(ESP_OK, storage_erase_all());

    uint8_t count = 0xFF;
    TEST_ASSERT_EQUAL(ESP_OK, storage_get_reboot_count(&count));
    TEST_ASSERT_EQUAL_UINT8(0, count);
}

/* -------------------------------------------------------------------------
 * 7. credentials_roundtrip
 * ------------------------------------------------------------------------- */
static void test_credentials_roundtrip(void)
{
    storage_credentials_t saved;
    memset(&saved, 0, sizeof(saved));

    strncpy(saved.wifi_ssid, "TestNetwork",    sizeof(saved.wifi_ssid)  - 1);
    strncpy(saved.wifi_pass, "s3cr3tpassword", sizeof(saved.wifi_pass)  - 1);
    strncpy(saved.mqtt_url,  "mqtt://broker.local", sizeof(saved.mqtt_url) - 1);
    saved.mqtt_port = 1883;
    strncpy(saved.mqtt_user, "heimdall",       sizeof(saved.mqtt_user)  - 1);
    strncpy(saved.mqtt_pass, "mqttpassword",   sizeof(saved.mqtt_pass)  - 1);

    TEST_ASSERT_EQUAL(ESP_OK, storage_save_credentials(&saved));

    storage_credentials_t loaded;
    memset(&loaded, 0, sizeof(loaded));
    TEST_ASSERT_EQUAL(ESP_OK, storage_load_credentials(&loaded));

    TEST_ASSERT_EQUAL_STRING(saved.wifi_ssid, loaded.wifi_ssid);
    TEST_ASSERT_EQUAL_STRING(saved.wifi_pass, loaded.wifi_pass);
    TEST_ASSERT_EQUAL_STRING(saved.mqtt_url,  loaded.mqtt_url);
    TEST_ASSERT_EQUAL_UINT16(saved.mqtt_port, loaded.mqtt_port);
    TEST_ASSERT_EQUAL_STRING(saved.mqtt_user, loaded.mqtt_user);
    TEST_ASSERT_EQUAL_STRING(saved.mqtt_pass, loaded.mqtt_pass);
}

/* -------------------------------------------------------------------------
 * 8. is_provisioned_false_on_empty_nvs
 *
 * storage_is_provisioned() returns bool. On a fresh NVS partition it must
 * return false (no "pv" key exists; nvs_open in READONLY fails, returns false).
 * ------------------------------------------------------------------------- */
static void test_is_provisioned_false_on_empty_nvs(void)
{
    bool provisioned = storage_is_provisioned();
    TEST_ASSERT_FALSE(provisioned);
}

/* -------------------------------------------------------------------------
 * Entry point
 * ------------------------------------------------------------------------- */

void app_main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_load_hostname_empty_nvs);
    RUN_TEST(test_save_and_load_hostname_roundtrip);
    RUN_TEST(test_hostname_erased_by_erase_all);
    RUN_TEST(test_reboot_count_empty_nvs_returns_zero);
    RUN_TEST(test_reboot_count_roundtrip);
    RUN_TEST(test_reboot_count_cleared_by_erase_all);
    RUN_TEST(test_credentials_roundtrip);
    RUN_TEST(test_is_provisioned_false_on_empty_nvs);

    UNITY_END();
}
