/**
 * @file ana_in.h
 * @brief Analog input reading, scaled to the full uint16_t range and low-pass filtered.
 */

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Identifiers for the analog input channels.
 */
typedef enum
{
  ANA_IN_0 = 0,
  ANA_IN_END
} ENUM_ANA_IN;

/**
 * @brief Nominal ESP32 ADC full-scale voltage at ADC_ATTEN_DB_12, corresponding to UINT16_MAX.
 *        Shared by any module that needs to convert a Read/ReadFiltered digit value to/from Volts.
 */
#define ANA_IN_VMAX_VOLTS 3.3f

/**
 * @brief Configure the ADC unit/channel(s) used for the analog inputs.
 */
void InitAnaIn(void);

/**
 * @brief Sample the analog input(s). Must be called every 1ms.
 */
void TimerAnaIn(void);

/**
 * @brief Cyclic housekeeping for the analog input(s). Must be called every 20ms.
 */
void CycleAnaIn(void);

/**
 * @brief Read the last sampled, unfiltered value of an analog input.
 *
 * @param eAnaIn Channel to read.
 * @return Value scaled to 0..UINT16_MAX, where UINT16_MAX represents the maximum ADC voltage.
 */
uint16_t ReadAnaIn(ENUM_ANA_IN eAnaIn);

/**
 * @brief Read the low-pass filtered value of an analog input.
 *
 * @param eAnaIn Channel to read.
 * @return Value scaled to 0..UINT16_MAX, where UINT16_MAX represents the maximum ADC voltage.
 */
uint16_t ReadAnaInFiltered(ENUM_ANA_IN eAnaIn);

#ifdef __cplusplus
}
#endif
