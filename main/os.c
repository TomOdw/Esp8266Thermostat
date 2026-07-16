#include "os.h"

#include "ana_in.h"
#include "dig_out.h"
#include "nvs_store.h"
#include "thermostat.h"
#include "webpage.h"
#include "wifi_ap.h"

void InitOs(void)
{
  InitNvs();
  InitAnaIn();
  InitDigOut();
  InitWifiAp();
  StartWifiAp();
  InitThermostat();
  InitWebpage();
}

void TimerOs(void)
{
  TimerAnaIn();
  TimerDigOut();
  TimerThermostat();
}

void CycleOs(void)
{
  CycleAnaIn();
  CycleDigOut();
  CycleThermostat();
}
