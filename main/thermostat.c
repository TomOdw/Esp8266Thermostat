#include "thermostat.h"

#include "ana_in.h"
#include "curve.h"
#include "dig_out.h"
#include "nvs_store.h"

/* Placeholder defaults in DegC. Tune to the sensor/curve in use. */
#define THERMOSTAT_DEFAULT_OFF_C 18.0f
#define THERMOSTAT_DEFAULT_ON_C 22.0f

static volatile float s_fThresholdOffC;
static volatile float s_fThresholdOnC;

void InitThermostat(void)
{
  float fOffC;
  float fOnC;

  if (ReadNvs(NVS_PARAM_THERMOSTAT_OFF, &fOffC, sizeof(fOffC)) != ESP_OK)
  {
    fOffC = THERMOSTAT_DEFAULT_OFF_C;
    WriteNvs(NVS_PARAM_THERMOSTAT_OFF, &fOffC, sizeof(fOffC));
  }

  if (ReadNvs(NVS_PARAM_THERMOSTAT_ON, &fOnC, sizeof(fOnC)) != ESP_OK)
  {
    fOnC = THERMOSTAT_DEFAULT_ON_C;
    WriteNvs(NVS_PARAM_THERMOSTAT_ON, &fOnC, sizeof(fOnC));
  }

  s_fThresholdOffC = fOffC;
  s_fThresholdOnC = fOnC;
}

void TimerThermostat(void)
{
  /* Reserved for future timer-driven diagnostics. */
}

void CycleThermostat(void)
{
  float fTempC = GetCurveTemperature(ReadAnaInFiltered(ANA_IN_0));

  if (fTempC >= s_fThresholdOnC)
  {
    WriteDigOut(DIG_OUT_0, true);
  }
  else if (fTempC <= s_fThresholdOffC)
  {
    WriteDigOut(DIG_OUT_0, false);
  }
  /* Else: within the hysteresis band, leave the output unchanged. */
}

float GetThermostatOffThresholdC(void)
{
  return s_fThresholdOffC;
}

float GetThermostatOnThresholdC(void)
{
  return s_fThresholdOnC;
}

void SetThermostatThresholds(float fOffC, float fOnC)
{
  s_fThresholdOffC = fOffC;
  s_fThresholdOnC = fOnC;

  WriteNvs(NVS_PARAM_THERMOSTAT_OFF, &fOffC, sizeof(fOffC));
  WriteNvs(NVS_PARAM_THERMOSTAT_ON, &fOnC, sizeof(fOnC));
}
