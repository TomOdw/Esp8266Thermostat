#include "dig_out.h"

#include "driver/gpio.h"

static const gpio_num_t s_aeGpio[DIG_OUT_END] = { GPIO_NUM_4 };

static volatile bool s_axState[DIG_OUT_END];

void InitDigOut(void)
{
  ENUM_DIG_OUT eDigOut;

  for (eDigOut = 0; eDigOut < DIG_OUT_END; eDigOut++)
  {
    gpio_config_t xConfig =
    {
      .pin_bit_mask = 1ULL << s_aeGpio[eDigOut],
      .mode = GPIO_MODE_OUTPUT,
      .pull_up_en = GPIO_PULLUP_DISABLE,
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
      .intr_type = GPIO_INTR_DISABLE
    };

    ESP_ERROR_CHECK(gpio_config(&xConfig));
    WriteDigOut(eDigOut, false);
  }
}

void TimerDigOut(void)
{
  /* Reserved for future timer-driven diagnostics. */
}

void CycleDigOut(void)
{
  /* Reserved for future cyclic diagnostics. */
}

void WriteDigOut(ENUM_DIG_OUT eDigOut, bool xState)
{
  s_axState[eDigOut] = xState;
  gpio_set_level(s_aeGpio[eDigOut], xState ? 1 : 0);
}

bool ReadDigOut(ENUM_DIG_OUT eDigOut)
{
  return s_axState[eDigOut];
}
