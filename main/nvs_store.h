/**
 * @file nvs_store.h
 * @brief Persistent key/value storage for thermostat settings, backed by NVS flash.
 */

#pragma once

#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Identifiers for the parameters persisted in NVS.
 */
typedef enum
{
  NVS_PARAM_THERMOSTAT_LOW = 0,
  NVS_PARAM_THERMOSTAT_HIGH,
  NVS_PARAM_AP_NAME,
  NVS_PARAM_END
} ENUM_NVS_PARAM;

/**
 * @brief Initialize the NVS flash partition and open the application namespace.
 */
void InitNvs(void);

/**
 * @brief Read a persisted parameter.
 *
 * @param eParam Parameter to read.
 * @param pvValue Destination buffer (uint16_t* for the threshold params, char* for NVS_PARAM_AP_NAME).
 * @param u32Size Capacity of pvValue in bytes. Only used for NVS_PARAM_AP_NAME.
 * @return ESP_OK on success, ESP_ERR_NVS_NOT_FOUND if the parameter was never written,
 *         or another esp_err_t on failure.
 */
esp_err_t ReadNvs(ENUM_NVS_PARAM eParam, void *pvValue, uint32_t u32Size);

/**
 * @brief Write and persist a parameter.
 *
 * @param eParam Parameter to write.
 * @param pvValue Source buffer (uint16_t* for the threshold params, NUL-terminated char* for NVS_PARAM_AP_NAME).
 * @param u32Size Capacity of pvValue in bytes. Only used for NVS_PARAM_AP_NAME.
 * @return ESP_OK on success, or another esp_err_t on failure.
 */
esp_err_t WriteNvs(ENUM_NVS_PARAM eParam, const void *pvValue, uint32_t u32Size);

#ifdef __cplusplus
}
#endif
