idf_component_register(SRCS "curve.c" "main.c" "os.c" "ana_in.c" "dig_out.c" "nvs_store.c" "wifi_ap.c" "thermostat.c" "webpage.c"
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES esp_adc esp_driver_gpio esp_wifi esp_netif esp_event nvs_flash esp_http_server esp_timer)