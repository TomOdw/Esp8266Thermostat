/**
 * @file thermostat.h
 * @brief Hysteresis control of the digital output based on the curve-derived temperature.
 */

#pragma once

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
 * @brief Get the threshold at/below which the output turns off.
 *
 * @return Threshold in DegC.
 */
float GetThermostatOffThresholdC(void);

/**
 * @brief Get the threshold at/above which the output turns on.
 *
 * @return Threshold in DegC.
 */
float GetThermostatOnThresholdC(void);

/**
 * @brief Set and persist new hysteresis thresholds.
 *
 * @param fOffC New off threshold in DegC (output turns off at/below this).
 * @param fOnC New on threshold in DegC (output turns on at/above this).
 */
void SetThermostatThresholds(float fOffC, float fOnC);

#ifdef __cplusplus
}
#endif
