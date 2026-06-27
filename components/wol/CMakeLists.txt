idf_component_register(
    SRCS
        "wol.c"
    INCLUDE_DIRS
        "."
    REQUIRES
        lwip
        esp_netif
        esp_timer
)

# Ping feedback requires the MQTT client to publish results and opsec.h
# for OPSEC_TOPIC_MAX_LEN. These are only linked when the feature is on.
idf_component_optional_requires(PRIVATE espressif__mqtt opsec)
