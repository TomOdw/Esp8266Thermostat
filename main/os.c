#include "os.h"

#include "ana_in.h"
#include "curve.h"
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
  InitCurve();
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

void IdleOs(void)
{
  /* Reserved for future idle-priority work. */
}
