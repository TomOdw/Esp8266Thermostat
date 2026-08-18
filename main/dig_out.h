/**
 * @file dig_out.h
 * @brief Digital output control.
 */

#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Identifiers for the digital output channels.
 */
typedef enum
{
  DIG_OUT_0 = 0,
  DIG_OUT_END
} ENUM_DIG_OUT;

/**
 * @brief Configure the GPIO(s) used for the digital output(s). Defaults to low.
 */
void InitDigOut(void);

/**
 * @brief Timer housekeeping for the digital output(s). Must be called every 1ms.
 */
void TimerDigOut(void);

/**
 * @brief Cyclic housekeeping for the digital output(s). Must be called every 20ms.
 */
void CycleDigOut(void);

/**
 * @brief Drive a digital output.
 *
 * @param eDigOut Channel to drive.
 * @param xState true for high, false for low.
 */
void WriteDigOut(ENUM_DIG_OUT eDigOut, bool xState);

/**
 * @brief Read back the last logical state written to a digital output (before polarity inversion).
 *
 * @param eDigOut Channel to read.
 * @return true if logically on, false if logically off.
 */
bool ReadDigOut(ENUM_DIG_OUT eDigOut);

/**
 * @brief Get whether output polarity is currently inverted.
 *
 * @return true if inverted (e.g. for an active-low relay), false for normal polarity.
 */
bool GetDigOutInvert(void);

/**
 * @brief Set and persist output polarity inversion. Immediately re-applies the already-written
 *        logical state of every channel to its GPIO under the new polarity.
 *
 * @param xInvert true to invert (e.g. for an active-low relay), false for normal polarity.
 */
void SetDigOutInvert(bool xInvert);

#ifdef __cplusplus
}
#endif
