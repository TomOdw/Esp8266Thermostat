#include "webpage.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_http_server.h"
#include "esp_system.h"

#include "ana_in.h"
#include "dig_out.h"
#include "nvs_store.h"
#include "thermostat.h"

/* httpd_query_key_value() does not percent-decode its input, so the AP name entered by the
 * user must stick to characters that survive x-www-form-urlencoded as-is (letters, digits, -, _). */

/* Nominal ESP32 ADC full-scale voltage at ADC_ATTEN_DB_12, used only to present/accept Volts on
 * the webpage. All other modules keep working in the raw 0..UINT16_MAX digit scale. */
#define WEBPAGE_ADC_VMAX_VOLTS 3.3f

static const char *WEBPAGE_STYLE =
  "<style>"
  "body{font-family:sans-serif;max-width:480px;margin:0 auto;padding:16px;"
  "font-size:18px;background:#f4f6f8;color:#222}"
  "h1{font-size:22px;margin-bottom:4px}"
  ".card{background:#fff;border-radius:10px;padding:16px;margin:12px 0;"
  "box-shadow:0 1px 3px rgba(0,0,0,0.15)}"
  ".reading{font-size:32px;font-weight:bold;margin:4px 0}"
  ".badge{display:inline-block;padding:4px 14px;border-radius:14px;"
  "font-weight:bold;color:#fff;font-size:16px}"
  ".on{background:#2ecc71}"
  ".off{background:#95a5a6}"
  "label{display:block;margin:12px 0 4px;font-weight:bold}"
  "input{width:100%;box-sizing:border-box;font-size:18px;padding:8px;"
  "border:1px solid #ccc;border-radius:6px}"
  "button{margin-top:16px;width:100%;font-size:18px;padding:10px;"
  "background:#2980b9;color:#fff;border:none;border-radius:6px}"
  "</style>";

/* Polls /status once per second and updates the reading/badge in place, without reloading the
 * page (and without touching the settings form, so it doesn't clobber values being edited). */
static const char *WEBPAGE_SCRIPT =
  "<script>"
  "function pollStatus(){"
  "fetch('/status').then(function(r){return r.json();}).then(function(d){"
  "document.getElementById('measured').textContent=d.voltage.toFixed(2);"
  "var o=document.getElementById('output');"
  "o.textContent=d.output?'ON':'OFF';"
  "o.className='badge '+(d.output?'on':'off');"
  "}).catch(function(){});"
  "}"
  "setInterval(pollStatus,1000);"
  "document.getElementById('settingsForm').addEventListener('submit',function(e){"
  "e.preventDefault();"
  "var body=new URLSearchParams(new FormData(e.target)).toString();"
  "fetch('/settings',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:body})"
  ".then(function(r){return r.json();})"
  ".then(function(d){alert(d.rebooting?'Saved. Rebooting...':'Saved.');})"
  ".catch(function(){alert('Error while saving.');});"
  "});"
  "</script>";

static float DigitsToVolts(uint16_t u16Digits)
{
  return ((float) u16Digits * WEBPAGE_ADC_VMAX_VOLTS) / (float) UINT16_MAX;
}

static uint16_t VoltsToDigits(float fVolts)
{
  float fClamped = fVolts;

  if (fClamped < 0.0f)
  {
    fClamped = 0.0f;
  }
  if (fClamped > WEBPAGE_ADC_VMAX_VOLTS)
  {
    fClamped = WEBPAGE_ADC_VMAX_VOLTS;
  }

  return (uint16_t) ((fClamped * (float) UINT16_MAX) / WEBPAGE_ADC_VMAX_VOLTS + 0.5f);
}

#define WEBPAGE_HTML_BUFFER_SIZE 4096

static esp_err_t WebpageIndexGetHandler(httpd_req_t *pxReq)
{
  char *pcHtml = malloc(WEBPAGE_HTML_BUFFER_SIZE);
  char acApName[33];
  float fMeasuredVolts = DigitsToVolts(ReadAnaInFiltered(ANA_IN_0));
  bool xOutputState = ReadDigOut(DIG_OUT_0);
  float fLowVolts = DigitsToVolts(GetThermostatLowThreshold());
  float fHighVolts = DigitsToVolts(GetThermostatHighThreshold());
  const char *pcOutputClass = xOutputState ? "on" : "off";
  const char *pcOutputText = xOutputState ? "ON" : "OFF";

  if (pcHtml == NULL)
  {
    httpd_resp_send_500(pxReq);
    return ESP_FAIL;
  }

  if (ReadNvs(NVS_PARAM_AP_NAME, acApName, sizeof(acApName)) != ESP_OK)
  {
    strcpy(acApName, "Thermostat");
  }

  snprintf(pcHtml, WEBPAGE_HTML_BUFFER_SIZE,
    "<html><head><title>%s</title>"
    "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
    "%s"
    "</head><body>"
    "<h1>%s</h1>"
    "<div class=\"card\">"
    "<div class=\"reading\"><span id=\"measured\">%.2f</span> V</div>"
    "<div>Output: <span id=\"output\" class=\"badge %s\">%s</span></div>"
    "</div>"
    "<div class=\"card\">"
    "<form id=\"settingsForm\" method=\"POST\" action=\"/settings\">"
    "<label>AP name: <input type=\"text\" name=\"ap_name\" value=\"%s\"></label>"
    "<label>Low threshold (V): <input type=\"number\" step=\"0.01\" min=\"0\" max=\"%.2f\" name=\"low\" value=\"%.2f\"></label>"
    "<label>High threshold (V): <input type=\"number\" step=\"0.01\" min=\"0\" max=\"%.2f\" name=\"high\" value=\"%.2f\"></label>"
    "<button type=\"submit\">Save</button>"
    "</form>"
    "</div>"
    "%s"
    "</body></html>",
    acApName, WEBPAGE_STYLE, acApName, fMeasuredVolts, pcOutputClass, pcOutputText, acApName,
    (double) WEBPAGE_ADC_VMAX_VOLTS, fLowVolts, (double) WEBPAGE_ADC_VMAX_VOLTS, fHighVolts,
    WEBPAGE_SCRIPT);

  httpd_resp_set_type(pxReq, "text/html");
  httpd_resp_send(pxReq, pcHtml, HTTPD_RESP_USE_STRLEN);
  free(pcHtml);

  return ESP_OK;
}

static esp_err_t WebpageStatusGetHandler(httpd_req_t *pxReq)
{
  char acJson[64];
  float fMeasuredVolts = DigitsToVolts(ReadAnaInFiltered(ANA_IN_0));
  bool xOutputState = ReadDigOut(DIG_OUT_0);

  snprintf(acJson, sizeof(acJson), "{\"voltage\":%.2f,\"output\":%s}",
    fMeasuredVolts, xOutputState ? "true" : "false");

  httpd_resp_set_type(pxReq, "application/json");
  httpd_resp_send(pxReq, acJson, HTTPD_RESP_USE_STRLEN);

  return ESP_OK;
}

static esp_err_t WebpageSettingsPostHandler(httpd_req_t *pxReq)
{
  char acBody[256];
  char acApName[33];
  char acCurrentApName[33];
  char acLow[16];
  char acHigh[16];
  int i32Remaining = (int) pxReq->content_len;
  int i32Received = 0;
  uint16_t u16Low;
  uint16_t u16High;
  bool xApNameChanged = false;

  if (i32Remaining >= (int) sizeof(acBody))
  {
    httpd_resp_send_err(pxReq, HTTPD_400_BAD_REQUEST, "Body too large");
    return ESP_FAIL;
  }

  while (i32Remaining > 0)
  {
    int i32Ret = httpd_req_recv(pxReq, acBody + i32Received, i32Remaining);

    if (i32Ret <= 0)
    {
      if (i32Ret == HTTPD_SOCK_ERR_TIMEOUT)
      {
        continue;
      }
      return ESP_FAIL;
    }
    i32Received += i32Ret;
    i32Remaining -= i32Ret;
  }
  acBody[i32Received] = '\0';

  if (httpd_query_key_value(acBody, "ap_name", acApName, sizeof(acApName)) != ESP_OK)
  {
    acApName[0] = '\0';
  }
  if (httpd_query_key_value(acBody, "low", acLow, sizeof(acLow)) != ESP_OK)
  {
    strcpy(acLow, "0");
  }
  if (httpd_query_key_value(acBody, "high", acHigh, sizeof(acHigh)) != ESP_OK)
  {
    strcpy(acHigh, "0");
  }

  u16Low = VoltsToDigits(strtof(acLow, NULL));
  u16High = VoltsToDigits(strtof(acHigh, NULL));
  SetThermostatThresholds(u16Low, u16High);

  if (ReadNvs(NVS_PARAM_AP_NAME, acCurrentApName, sizeof(acCurrentApName)) != ESP_OK)
  {
    acCurrentApName[0] = '\0';
  }

  if (acApName[0] != '\0' && strcmp(acApName, acCurrentApName) != 0)
  {
    WriteNvs(NVS_PARAM_AP_NAME, acApName, (uint32_t) (strlen(acApName) + 1));
    xApNameChanged = true;
  }

  httpd_resp_set_type(pxReq, "application/json");

  if (xApNameChanged)
  {
    httpd_resp_send(pxReq, "{\"rebooting\":true}", HTTPD_RESP_USE_STRLEN);
    esp_restart();
  }
  else
  {
    httpd_resp_send(pxReq, "{\"rebooting\":false}", HTTPD_RESP_USE_STRLEN);
  }

  return ESP_OK;
}

void InitWebpage(void)
{
  httpd_handle_t xServer = NULL;
  httpd_config_t xConfig = HTTPD_DEFAULT_CONFIG();
  static const httpd_uri_t xIndexUri =
  {
    .uri = "/",
    .method = HTTP_GET,
    .handler = WebpageIndexGetHandler,
    .user_ctx = NULL
  };
  static const httpd_uri_t xStatusUri =
  {
    .uri = "/status",
    .method = HTTP_GET,
    .handler = WebpageStatusGetHandler,
    .user_ctx = NULL
  };
  static const httpd_uri_t xSettingsUri =
  {
    .uri = "/settings",
    .method = HTTP_POST,
    .handler = WebpageSettingsPostHandler,
    .user_ctx = NULL
  };

  ESP_ERROR_CHECK(httpd_start(&xServer, &xConfig));
  ESP_ERROR_CHECK(httpd_register_uri_handler(xServer, &xIndexUri));
  ESP_ERROR_CHECK(httpd_register_uri_handler(xServer, &xStatusUri));
  ESP_ERROR_CHECK(httpd_register_uri_handler(xServer, &xSettingsUri));
}
