#include "wifi_ap.h"

#include <string.h>

#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "nvs_store.h"

#define WIFI_AP_DEFAULT_NAME "ESP32-Thermostat"
#define WIFI_AP_CHANNEL 1
#define WIFI_AP_MAX_CONNECTIONS 4

void InitWifiAp(void)
{
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  esp_netif_create_default_wifi_ap();

  wifi_init_config_t xInitConfig = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&xInitConfig));
}

void StartWifiAp(void)
{
  char acApName[33];
  uint32_t u32ApNameSize = sizeof(acApName);
  wifi_config_t xWifiConfig = { 0 };

  if (ReadNvs(NVS_PARAM_AP_NAME, acApName, u32ApNameSize) != ESP_OK)
  {
    strncpy(acApName, WIFI_AP_DEFAULT_NAME, sizeof(acApName) - 1);
    acApName[sizeof(acApName) - 1] = '\0';
    WriteNvs(NVS_PARAM_AP_NAME, acApName, (uint32_t) (strlen(acApName) + 1));
  }

  memcpy(xWifiConfig.ap.ssid, acApName, strlen(acApName));
  xWifiConfig.ap.ssid_len = (uint8_t) strlen(acApName);
  xWifiConfig.ap.channel = WIFI_AP_CHANNEL;
  xWifiConfig.ap.max_connection = WIFI_AP_MAX_CONNECTIONS;
  xWifiConfig.ap.authmode = WIFI_AUTH_OPEN;

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &xWifiConfig));
  ESP_ERROR_CHECK(esp_wifi_start());
}
