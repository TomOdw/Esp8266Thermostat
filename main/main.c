#include <stdint.h>

#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "os.h"

#define MAIN_TICK_BUDGET_US 1000

void app_main(void)
{
  uint8_t u8TickCounter = 0;

  InitOs();

  for (;;)
  {
    int64_t i64TickStartUs = esp_timer_get_time();

    TimerOs();
    u8TickCounter++;
    if (u8TickCounter >= 20)
    {
      u8TickCounter = 0;
      CycleOs();
    }

    if (esp_timer_get_time() - i64TickStartUs < MAIN_TICK_BUDGET_US)
    {
      IdleOs();
    }

    /* ESP-IDF has no bare-metal path: the WiFi driver and HTTP server are both hard FreeRTOS
     * task dependencies (confirmed against the IDF source), so this task must still yield the
     * CPU every tick for them - and for the idle task's watchdog - to run. This also paces the
     * loop to the 1ms tick. */
    vTaskDelay(pdMS_TO_TICKS(1));
  }
}
