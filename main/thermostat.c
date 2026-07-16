#include "thermostat.h"

#include "ana_in.h"
#include "dig_out.h"
#include "nvs_store.h"

/* Placeholder defaults, in the same 0..UINT16_MAX scale as ReadAnaInFiltered. Tune to the sensor in use. */
#define THERMOSTAT_DEFAULT_LOW 20000
#define THERMOSTAT_DEFAULT_HIGH 45000

static volatile uint16_t s_u16ThresholdLow;
static volatile uint16_t s_u16ThresholdHigh;

void InitThermostat(void)
{
  uint16_t u16Low;
  uint16_t u16High;

  if (ReadNvs(NVS_PARAM_THERMOSTAT_LOW, &u16Low, sizeof(u16Low)) != ESP_OK)
  {
    u16Low = THERMOSTAT_DEFAULT_LOW;
    WriteNvs(NVS_PARAM_THERMOSTAT_LOW, &u16Low, sizeof(u16Low));
  }

  if (ReadNvs(NVS_PARAM_THERMOSTAT_HIGH, &u16High, sizeof(u16High)) != ESP_OK)
  {
    u16High = THERMOSTAT_DEFAULT_HIGH;
    WriteNvs(NVS_PARAM_THERMOSTAT_HIGH, &u16High, sizeof(u16High));
  }

  s_u16ThresholdLow = u16Low;
  s_u16ThresholdHigh = u16High;
}

void TimerThermostat(void)
{
  /* Reserved for future timer-driven diagnostics. */
}

void CycleThermostat(void)
{
  uint16_t u16Value = ReadAnaInFiltered(ANA_IN_0);

  if (u16Value >= s_u16ThresholdHigh)
  {
    WriteDigOut(DIG_OUT_0, true);
  }
  else if (u16Value <= s_u16ThresholdLow)
  {
    WriteDigOut(DIG_OUT_0, false);
  }
  /* Else: within the hysteresis band, leave the output unchanged. */
}

uint16_t GetThermostatLowThreshold(void)
{
  return s_u16ThresholdLow;
}

uint16_t GetThermostatHighThreshold(void)
{
  return s_u16ThresholdHigh;
}

void SetThermostatThresholds(uint16_t u16Low, uint16_t u16High)
{
  s_u16ThresholdLow = u16Low;
  s_u16ThresholdHigh = u16High;

  WriteNvs(NVS_PARAM_THERMOSTAT_LOW, &u16Low, sizeof(u16Low));
  WriteNvs(NVS_PARAM_THERMOSTAT_HIGH, &u16High, sizeof(u16High));
}
