#include "nvs_store.h"

#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"

static const char *TAG = "nvs_store";
static const char *NVS_NAMESPACE = "thermo";

static nvs_handle_t s_xNvsHandle;

static const char *GetKeyForParam(ENUM_NVS_PARAM eParam)
{
  switch (eParam)
  {
    case NVS_PARAM_THERMOSTAT_OFF:
      return "off";
    case NVS_PARAM_THERMOSTAT_ON:
      return "on";
    case NVS_PARAM_AP_NAME:
      return "apname";
    case NVS_PARAM_CURVE:
      return "curve";
    case NVS_PARAM_DIG_OUT_INVERT:
      return "invert";
    default:
      return NULL;
  }
}

void InitNvs(void)
{
  esp_err_t eErr = nvs_flash_init();

  if (eErr == ESP_ERR_NVS_NO_FREE_PAGES || eErr == ESP_ERR_NVS_NEW_VERSION_FOUND)
  {
    /* The NVS partition layout changed or is out of free pages; erase and retry once. */
    ESP_ERROR_CHECK(nvs_flash_erase());
    eErr = nvs_flash_init();
  }
  ESP_ERROR_CHECK(eErr);

  ESP_ERROR_CHECK(nvs_open(NVS_NAMESPACE, NVS_READWRITE, &s_xNvsHandle));
}

esp_err_t ReadNvs(ENUM_NVS_PARAM eParam, void *pvValue, uint32_t u32Size)
{
  const char *pcKey = GetKeyForParam(eParam);
  size_t u32Required = u32Size;

  if (pcKey == NULL)
  {
    return ESP_ERR_INVALID_ARG;
  }

  if (eParam == NVS_PARAM_AP_NAME)
  {
    return nvs_get_str(s_xNvsHandle, pcKey, (char *) pvValue, &u32Required);
  }

  return nvs_get_blob(s_xNvsHandle, pcKey, pvValue, &u32Required);
}

esp_err_t WriteNvs(ENUM_NVS_PARAM eParam, const void *pvValue, uint32_t u32Size)
{
  const char *pcKey = GetKeyForParam(eParam);
  esp_err_t eErr;

  if (pcKey == NULL)
  {
    return ESP_ERR_INVALID_ARG;
  }

  if (eParam == NVS_PARAM_AP_NAME)
  {
    eErr = nvs_set_str(s_xNvsHandle, pcKey, (const char *) pvValue);
  }
  else
  {
    eErr = nvs_set_blob(s_xNvsHandle, pcKey, pvValue, u32Size);
  }

  if (eErr != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to write parameter %d: %s", (int) eParam, esp_err_to_name(eErr));
    return eErr;
  }

  return nvs_commit(s_xNvsHandle);
}
