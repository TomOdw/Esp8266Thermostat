/**
 * @file thermostat.h
 * @brief Hysteresis control of the digital output based on the filtered analog input.
 */

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Load the thresholds from NVS (seeding defaults on first boot).
 */
void InitThermostat(void);

/**
 * @brief Timer housekeeping for the thermostat. Must be called every 1ms.
 */
void TimerThermostat(void);

/**
 * @brief Evaluate the hysteresis control and drive the digital output. Must be called every 20ms.
 */
void CycleThermostat(void);

/**
 * @brief Get the currently active low threshold.
 *
 * @return Threshold, scaled to 0..UINT16_MAX.
 */
uint16_t GetThermostatLowThreshold(void);

/**
 * @brief Get the currently active high threshold.
 *
 * @return Threshold, scaled to 0..UINT16_MAX.
 */
uint16_t GetThermostatHighThreshold(void);

/**
 * @brief Set and persist new hysteresis thresholds.
 *
 * @param u16Low New low threshold, scaled to 0..UINT16_MAX.
 * @param u16High New high threshold, scaled to 0..UINT16_MAX.
 */
void SetThermostatThresholds(uint16_t u16Low, uint16_t u16High);

#ifdef __cplusplus
}
#endif
