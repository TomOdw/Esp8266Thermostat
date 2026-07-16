#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "os.h"

void app_main(void)
{
  uint8_t u8TickCounter = 0;

  InitOs();

  for (;;)
  {
    vTaskDelay(pdMS_TO_TICKS(1));
    TimerOs();
    u8TickCounter++;
    if (u8TickCounter >= 20)
    {
      u8TickCounter = 0;
      CycleOs();
    }
  }
}
