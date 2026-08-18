#include "curve.h"

#include "ana_in.h"
#include "nvs_store.h"

typedef struct
{
  uint8_t u8Count;
  CURVE_POINT axPoint[CURVE_POINT_MAX];
} CURVE_STORAGE;

static volatile CURVE_STORAGE s_xCurve;

static void SortCurvePoints(CURVE_POINT *paxPoints, uint8_t u8Count)
{
  uint8_t u8I;

  for (u8I = 1; u8I < u8Count; u8I++)
  {
    CURVE_POINT xKey = paxPoints[u8I];
    uint8_t u8J = u8I;

    while (u8J > 0 && paxPoints[u8J - 1].fVolts > xKey.fVolts)
    {
      paxPoints[u8J] = paxPoints[u8J - 1];
      u8J--;
    }
    paxPoints[u8J] = xKey;
  }
}

void InitCurve(void)
{
  CURVE_STORAGE xStorage = { 0 };
  uint8_t u8Idx;

  if (ReadNvs(NVS_PARAM_CURVE, &xStorage, sizeof(xStorage)) != ESP_OK)
  {
    /* Placeholder default (0V -> 0DegC, VMAX -> 100DegC); calibrate the real curve via the webpage. */
    xStorage.u8Count = 2;
    xStorage.axPoint[0].fVolts = 0.0f;
    xStorage.axPoint[0].fTempC = 0.0f;
    xStorage.axPoint[1].fVolts = ANA_IN_VMAX_VOLTS;
    xStorage.axPoint[1].fTempC = 100.0f;
    WriteNvs(NVS_PARAM_CURVE, &xStorage, sizeof(xStorage));
  }

  s_xCurve.u8Count = xStorage.u8Count;
  for (u8Idx = 0; u8Idx < CURVE_POINT_MAX; u8Idx++)
  {
    s_xCurve.axPoint[u8Idx] = xStorage.axPoint[u8Idx];
  }
}

uint8_t GetCurvePointCount(void)
{
  return s_xCurve.u8Count;
}

CURVE_POINT GetCurvePoint(uint8_t u8Index)
{
  return s_xCurve.axPoint[u8Index];
}

void SetCurvePoints(const CURVE_POINT *paxPoints, uint8_t u8Count)
{
  CURVE_STORAGE xStorage = { 0 };
  uint8_t u8Idx;

  if (u8Count > CURVE_POINT_MAX)
  {
    u8Count = CURVE_POINT_MAX;
  }

  xStorage.u8Count = u8Count;
  for (u8Idx = 0; u8Idx < u8Count; u8Idx++)
  {
    xStorage.axPoint[u8Idx] = paxPoints[u8Idx];
  }
  SortCurvePoints(xStorage.axPoint, xStorage.u8Count);

  s_xCurve.u8Count = xStorage.u8Count;
  for (u8Idx = 0; u8Idx < u8Count; u8Idx++)
  {
    s_xCurve.axPoint[u8Idx] = xStorage.axPoint[u8Idx];
  }

  WriteNvs(NVS_PARAM_CURVE, &xStorage, sizeof(xStorage));
}

float GetCurveTemperature(uint16_t u16Digits)
{
  float fVolts = ((float) u16Digits * ANA_IN_VMAX_VOLTS) / (float) UINT16_MAX;
  uint8_t u8Count = s_xCurve.u8Count;
  uint8_t u8Idx;

  if (u8Count == 0)
  {
    return 0.0f;
  }
  if (u8Count == 1 || fVolts <= s_xCurve.axPoint[0].fVolts)
  {
    return s_xCurve.axPoint[0].fTempC;
  }
  if (fVolts >= s_xCurve.axPoint[u8Count - 1].fVolts)
  {
    return s_xCurve.axPoint[u8Count - 1].fTempC;
  }

  for (u8Idx = 0; u8Idx < u8Count - 1; u8Idx++)
  {
    CURVE_POINT xA = s_xCurve.axPoint[u8Idx];
    CURVE_POINT xB = s_xCurve.axPoint[u8Idx + 1];

    if (fVolts <= xB.fVolts)
    {
      float fSpan = xB.fVolts - xA.fVolts;
      float fFrac = (fSpan > 0.0f) ? (fVolts - xA.fVolts) / fSpan : 0.0f;

      return xA.fTempC + fFrac * (xB.fTempC - xA.fTempC);
    }
  }

  return s_xCurve.axPoint[u8Count - 1].fTempC;
}
