/**
 * @file curve.h
 * @brief User-editable Volt-to-temperature characteristic curve, persisted in NVS.
 */

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Maximum number of (Volt, DegC) points the curve can hold.
 */
#define CURVE_POINT_MAX 10

/**
 * @brief One point of the characteristic curve.
 */
typedef struct
{
  float fVolts;
  float fTempC;
} CURVE_POINT;

/**
 * @brief Load the curve from NVS (seeding a placeholder default on first boot).
 */
void InitCurve(void);

/**
 * @brief Get the number of points currently defining the curve.
 *
 * @return Point count, 0..CURVE_POINT_MAX.
 */
uint8_t GetCurvePointCount(void);

/**
 * @brief Get one point of the curve, sorted ascending by fVolts.
 *
 * @param u8Index Index, 0..GetCurvePointCount()-1.
 * @return The point at that index.
 */
CURVE_POINT GetCurvePoint(uint8_t u8Index);

/**
 * @brief Replace and persist the curve.
 *
 * @param paxPoints Points to store; need not be pre-sorted.
 * @param u8Count Number of points in paxPoints, 0..CURVE_POINT_MAX.
 */
void SetCurvePoints(const CURVE_POINT *paxPoints, uint8_t u8Count);

/**
 * @brief Convert an analog-input digit value to a temperature via piecewise-linear interpolation
 *        over the stored curve, clamped flat beyond the first/last defined point.
 *
 * @param u16Digits Value as returned by ReadAnaIn/ReadAnaInFiltered.
 * @return Interpolated temperature in DegC. 0.0f if no curve points are defined.
 */
float GetCurveTemperature(uint16_t u16Digits);

#ifdef __cplusplus
}
#endif
