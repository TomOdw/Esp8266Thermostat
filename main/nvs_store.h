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
  NVS_PARAM_THERMOSTAT_OFF = 0,
  NVS_PARAM_THERMOSTAT_ON,
  NVS_PARAM_AP_NAME,
  NVS_PARAM_CURVE,
  NVS_PARAM_DIG_OUT_INVERT,
  NVS_PARAM_END
} ENUM_NVS_PARAM;

/**
 * @brief Initialize the NVS flash partition and open the application namespace.
 */
void InitNvs(void);

/**
 * @brief Read a persisted parameter.
 *
 * @param eParam Parameter to read. NVS_PARAM_AP_NAME is a NUL-terminated string; every other
 *               parameter is read/written as a raw blob of the caller's own struct/scalar type.
 * @param pvValue Destination buffer.
 * @param u32Size Capacity of pvValue in bytes.
 * @return ESP_OK on success, ESP_ERR_NVS_NOT_FOUND if the parameter was never written,
 *         or another esp_err_t on failure.
 */
esp_err_t ReadNvs(ENUM_NVS_PARAM eParam, void *pvValue, uint32_t u32Size);

/**
 * @brief Write and persist a parameter.
 *
 * @param eParam Parameter to write. NVS_PARAM_AP_NAME is a NUL-terminated string; every other
 *               parameter is read/written as a raw blob of the caller's own struct/scalar type.
 * @param pvValue Source buffer.
 * @param u32Size Size of pvValue in bytes.
 * @return ESP_OK on success, or another esp_err_t on failure.
 */
esp_err_t WriteNvs(ENUM_NVS_PARAM eParam, const void *pvValue, uint32_t u32Size);

#ifdef __cplusplus
}
#endif
