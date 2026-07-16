#include "ana_in.h"

#include "esp_adc/adc_oneshot.h"

#define ANA_IN_ADC_MAX_RAW 4095
#define ANA_IN_FILTER_SHIFT 4

static adc_oneshot_unit_handle_t s_xAdcHandle;
static const adc_channel_t s_aeAdcChannel[ANA_IN_END] = { ADC_CHANNEL_6 }; /* GPIO34 */

static volatile uint16_t s_au16LastRaw[ANA_IN_END];
static volatile uint32_t s_au32FilterAccum[ANA_IN_END];

void InitAnaIn(void)
{
  adc_oneshot_unit_init_cfg_t xUnitConfig =
  {
    .unit_id = ADC_UNIT_1
  };
  adc_oneshot_chan_cfg_t xChanConfig =
  {
    .atten = ADC_ATTEN_DB_12,
    .bitwidth = ADC_BITWIDTH_DEFAULT
  };
  ENUM_ANA_IN eAnaIn;

  ESP_ERROR_CHECK(adc_oneshot_new_unit(&xUnitConfig, &s_xAdcHandle));

  for (eAnaIn = 0; eAnaIn < ANA_IN_END; eAnaIn++)
  {
    ESP_ERROR_CHECK(adc_oneshot_config_channel(s_xAdcHandle, s_aeAdcChannel[eAnaIn], &xChanConfig));
  }
}

void TimerAnaIn(void)
{
  ENUM_ANA_IN eAnaIn;

  for (eAnaIn = 0; eAnaIn < ANA_IN_END; eAnaIn++)
  {
    int i32Raw = 0;
    uint16_t u16Raw;

    ESP_ERROR_CHECK(adc_oneshot_read(s_xAdcHandle, s_aeAdcChannel[eAnaIn], &i32Raw));

    u16Raw = (uint16_t) (((uint32_t) i32Raw * UINT16_MAX) / ANA_IN_ADC_MAX_RAW);
    s_au16LastRaw[eAnaIn] = u16Raw;
    s_au32FilterAccum[eAnaIn] += (uint32_t) u16Raw - (s_au32FilterAccum[eAnaIn] >> ANA_IN_FILTER_SHIFT);
  }
}

void CycleAnaIn(void)
{
  /* Reserved for future cyclic diagnostics. */
}

uint16_t ReadAnaIn(ENUM_ANA_IN eAnaIn)
{
  return s_au16LastRaw[eAnaIn];
}

uint16_t ReadAnaInFiltered(ENUM_ANA_IN eAnaIn)
{
  return (uint16_t) (s_au32FilterAccum[eAnaIn] >> ANA_IN_FILTER_SHIFT);
}
