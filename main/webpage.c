#include "webpage.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_http_server.h"
#include "esp_system.h"

#include "ana_in.h"
#include "curve.h"
#include "dig_out.h"
#include "nvs_store.h"
#include "thermostat.h"

/* httpd_query_key_value() does not percent-decode its input, so the AP name entered by the
 * user must stick to characters that survive x-www-form-urlencoded as-is (letters, digits, -, _). */

#define WEBPAGE_HTML_BUFFER_SIZE 6144
#define WEBPAGE_SVG_WIDTH 280
#define WEBPAGE_SVG_HEIGHT 160
#define WEBPAGE_SVG_MARGIN_L 30
#define WEBPAGE_SVG_MARGIN_R 10
#define WEBPAGE_SVG_MARGIN_T 10
#define WEBPAGE_SVG_MARGIN_B 20

static const char *WEBPAGE_STYLE =
  "<style>"
  "body{font-family:sans-serif;max-width:480px;margin:0 auto;padding:16px;"
  "font-size:18px;background:#f4f6f8;color:#222}"
  "h1{font-size:22px;margin-bottom:4px}"
  ".card{background:#fff;border-radius:10px;padding:16px;margin:12px 0;"
  "box-shadow:0 1px 3px rgba(0,0,0,0.15)}"
  ".reading{font-size:32px;font-weight:bold;margin:4px 0}"
  ".sub{font-size:16px;font-weight:normal;color:#666;margin:0 0 8px}"
  ".badge{display:inline-block;padding:4px 14px;border-radius:14px;"
  "font-weight:bold;color:#fff;font-size:16px}"
  ".on{background:#2ecc71}"
  ".off{background:#95a5a6}"
  ".svg-wrap{text-align:center}"
  "label{display:block;margin:12px 0 4px;font-weight:bold}"
  "input{width:100%;box-sizing:border-box;font-size:16px;padding:8px;"
  "border:1px solid #ccc;border-radius:6px}"
  "table{width:100%;border-collapse:collapse;margin:8px 0}"
  "table td{padding:2px}"
  "table th{font-size:14px;color:#666;font-weight:normal}"
  "button{margin-top:16px;width:100%;font-size:18px;padding:10px;"
  "background:#2980b9;color:#fff;border:none;border-radius:6px}"
  "</style>";

/* Appends a formatted fragment to *pxOffset within pcBuf. Truncation-safe: if a fragment would
 * not fit, *pxOffset is clamped to xBufSize so later calls become no-ops instead of underflowing
 * (xBufSize - *pxOffset) and overrunning the buffer - the classic running-snprintf-offset bug. */
static void HtmlAppend(char *pcBuf, size_t xBufSize, size_t *pxOffset, const char *pcFormat, ...)
{
  va_list xArgs;
  int iLen;

  if (*pxOffset >= xBufSize)
  {
    return;
  }

  va_start(xArgs, pcFormat);
  iLen = vsnprintf(pcBuf + *pxOffset, xBufSize - *pxOffset, pcFormat, xArgs);
  va_end(xArgs);

  if (iLen < 0)
  {
    return;
  }

  if ((size_t) iLen >= xBufSize - *pxOffset)
  {
    *pxOffset = xBufSize;
  }
  else
  {
    *pxOffset += (size_t) iLen;
  }
}

static float DigitsToVolts(uint16_t u16Digits)
{
  return ((float) u16Digits * ANA_IN_VMAX_VOLTS) / (float) UINT16_MAX;
}

static int TempToSvgY(float fTempC, float fTempMin, float fTempMax, int iPlotH)
{
  return WEBPAGE_SVG_MARGIN_T + (int) (((fTempMax - fTempC) / (fTempMax - fTempMin)) * iPlotH);
}

static int VoltsToSvgX(float fVolts, int iPlotW)
{
  return WEBPAGE_SVG_MARGIN_L + (int) ((fVolts / ANA_IN_VMAX_VOLTS) * iPlotW);
}

static void AppendCurveSvg(char *pcHtml, size_t xHtmlSize, size_t *pxOffset)
{
  uint8_t u8Count = GetCurvePointCount();
  uint16_t u16CurrentDigits = ReadAnaInFiltered(ANA_IN_0);
  float fCurrentVolts = DigitsToVolts(u16CurrentDigits);
  float fOffC = GetThermostatOffThresholdC();
  float fOnC = GetThermostatOnThresholdC();
  float fTempMin = (fOffC < fOnC) ? fOffC : fOnC;
  float fTempMax = (fOffC < fOnC) ? fOnC : fOffC;
  int iPlotW = WEBPAGE_SVG_WIDTH - WEBPAGE_SVG_MARGIN_L - WEBPAGE_SVG_MARGIN_R;
  int iPlotH = WEBPAGE_SVG_HEIGHT - WEBPAGE_SVG_MARGIN_T - WEBPAGE_SVG_MARGIN_B;
  int iPlotTop = WEBPAGE_SVG_MARGIN_T;
  int iPlotBottom = WEBPAGE_SVG_MARGIN_T + iPlotH;
  int iOnY;
  int iOffY;
  float fPad;
  uint8_t u8Idx;

  for (u8Idx = 0; u8Idx < u8Count; u8Idx++)
  {
    float fT = GetCurvePoint(u8Idx).fTempC;

    if (fT < fTempMin)
    {
      fTempMin = fT;
    }
    if (fT > fTempMax)
    {
      fTempMax = fT;
    }
  }
  if (fTempMax - fTempMin < 1.0f)
  {
    fTempMax = fTempMin + 1.0f; /* avoid a divide-by-zero plot range if off/on/curve all coincide */
  }
  fPad = (fTempMax - fTempMin) * 0.1f;
  fTempMin -= fPad;
  fTempMax += fPad;

  iOnY = TempToSvgY(fOnC, fTempMin, fTempMax, iPlotH);
  iOffY = TempToSvgY(fOffC, fTempMin, fTempMax, iPlotH);

  HtmlAppend(pcHtml, xHtmlSize, pxOffset,
    "<div class=\"svg-wrap\"><svg viewBox=\"0 0 %d %d\" width=\"100%%\" style=\"max-width:320px\">"
    "<rect x=\"0\" y=\"0\" width=\"%d\" height=\"%d\" fill=\"#fff\" stroke=\"#ccc\"/>",
    WEBPAGE_SVG_WIDTH, WEBPAGE_SVG_HEIGHT, WEBPAGE_SVG_WIDTH, WEBPAGE_SVG_HEIGHT);

  /* On/off zones: above the "on" line is where the output turns/stays on, below the "off" line
   * is where it turns/stays off - the red current-reading dot lands visibly in one of them. */
  HtmlAppend(pcHtml, xHtmlSize, pxOffset,
    "<rect x=\"%d\" y=\"%d\" width=\"%d\" height=\"%d\" fill=\"#2ecc71\" fill-opacity=\"0.15\"/>",
    WEBPAGE_SVG_MARGIN_L, iPlotTop, iPlotW, (iOnY > iPlotTop) ? (iOnY - iPlotTop) : 0);
  HtmlAppend(pcHtml, xHtmlSize, pxOffset,
    "<rect x=\"%d\" y=\"%d\" width=\"%d\" height=\"%d\" fill=\"#95a5a6\" fill-opacity=\"0.2\"/>",
    WEBPAGE_SVG_MARGIN_L, iOffY, iPlotW, (iPlotBottom > iOffY) ? (iPlotBottom - iOffY) : 0);
  HtmlAppend(pcHtml, xHtmlSize, pxOffset,
    "<line x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" stroke=\"#27ae60\" stroke-dasharray=\"4,2\"/>"
    "<text x=\"%d\" y=\"%d\" font-size=\"9\" fill=\"#27ae60\">on</text>"
    "<line x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" stroke=\"#7f8c8d\" stroke-dasharray=\"4,2\"/>"
    "<text x=\"%d\" y=\"%d\" font-size=\"9\" fill=\"#7f8c8d\">off</text>",
    WEBPAGE_SVG_MARGIN_L, iOnY, WEBPAGE_SVG_MARGIN_L + iPlotW, iOnY,
    WEBPAGE_SVG_MARGIN_L + iPlotW - 12, iOnY - 3,
    WEBPAGE_SVG_MARGIN_L, iOffY, WEBPAGE_SVG_MARGIN_L + iPlotW, iOffY,
    WEBPAGE_SVG_MARGIN_L + iPlotW - 14, iOffY + 11);

  if (u8Count >= 2)
  {
    HtmlAppend(pcHtml, xHtmlSize, pxOffset,
      "<polyline fill=\"none\" stroke=\"#2980b9\" stroke-width=\"2\" points=\"");

    for (u8Idx = 0; u8Idx < u8Count; u8Idx++)
    {
      CURVE_POINT xPt = GetCurvePoint(u8Idx);

      HtmlAppend(pcHtml, xHtmlSize, pxOffset, "%d,%d ",
        VoltsToSvgX(xPt.fVolts, iPlotW), TempToSvgY(xPt.fTempC, fTempMin, fTempMax, iPlotH));
    }

    HtmlAppend(pcHtml, xHtmlSize, pxOffset, "\"/>");

    for (u8Idx = 0; u8Idx < u8Count; u8Idx++)
    {
      CURVE_POINT xPt = GetCurvePoint(u8Idx);

      HtmlAppend(pcHtml, xHtmlSize, pxOffset,
        "<circle cx=\"%d\" cy=\"%d\" r=\"3\" fill=\"#2980b9\"/>",
        VoltsToSvgX(xPt.fVolts, iPlotW), TempToSvgY(xPt.fTempC, fTempMin, fTempMax, iPlotH));
    }
  }

  HtmlAppend(pcHtml, xHtmlSize, pxOffset,
    "<circle cx=\"%d\" cy=\"%d\" r=\"5\" fill=\"#e74c3c\"/>",
    VoltsToSvgX(fCurrentVolts, iPlotW),
    TempToSvgY(GetCurveTemperature(u16CurrentDigits), fTempMin, fTempMax, iPlotH));

  /* Axis scale labels: just the endpoints, drawn last so they sit on top of the zone shading. */
  HtmlAppend(pcHtml, xHtmlSize, pxOffset,
    "<text x=\"2\" y=\"%d\" font-size=\"8\" fill=\"#888\">%.0f</text>"
    "<text x=\"2\" y=\"%d\" font-size=\"8\" fill=\"#888\">%.0f</text>"
    "<text x=\"%d\" y=\"%d\" font-size=\"8\" fill=\"#888\">0V</text>"
    "<text x=\"%d\" y=\"%d\" font-size=\"8\" fill=\"#888\" text-anchor=\"end\">%.1fV</text>",
    iPlotTop + 8, (double) fTempMax,
    iPlotBottom, (double) fTempMin,
    WEBPAGE_SVG_MARGIN_L, WEBPAGE_SVG_HEIGHT - 4,
    WEBPAGE_SVG_MARGIN_L + iPlotW, WEBPAGE_SVG_HEIGHT - 4, (double) ANA_IN_VMAX_VOLTS);

  HtmlAppend(pcHtml, xHtmlSize, pxOffset, "</svg></div>");
}

/* The content of the /reading iframe: its own tiny standalone page, refreshing itself every 5s
 * independently of the outer page (see WebpageIndexGetHandler) so the settings form never gets
 * interrupted by a reload while the user is typing into it. */
static esp_err_t WebpageReadingGetHandler(httpd_req_t *pxReq)
{
  char acHtml[768];
  size_t xOffset = 0;
  uint16_t u16Digits = ReadAnaInFiltered(ANA_IN_0);
  float fMeasuredVolts = DigitsToVolts(u16Digits);
  float fMeasuredTempC = GetCurveTemperature(u16Digits);
  bool xOutputState = ReadDigOut(DIG_OUT_0);
  const char *pcOutputClass = xOutputState ? "on" : "off";
  const char *pcOutputText = xOutputState ? "ON" : "OFF";

  HtmlAppend(acHtml, sizeof(acHtml), &xOffset,
    "<html><head><meta http-equiv=\"refresh\" content=\"5\">"
    "<style>"
    "body{font-family:sans-serif;margin:0;padding:0;background:#fff;color:#222}"
    ".reading{font-size:32px;font-weight:bold;margin:4px 0}"
    ".sub{font-size:16px;font-weight:normal;color:#666;margin:0 0 8px}"
    ".badge{display:inline-block;padding:4px 14px;border-radius:14px;"
    "font-weight:bold;color:#fff;font-size:16px}"
    ".on{background:#2ecc71}"
    ".off{background:#95a5a6}"
    "</style></head><body>"
    "<div class=\"reading\">%.1f&deg;C</div>"
    "<div class=\"sub\">%.2f V</div>"
    "<div>Output: <span class=\"badge %s\">%s</span></div>"
    "</body></html>",
    (double) fMeasuredTempC, (double) fMeasuredVolts, pcOutputClass, pcOutputText);

  httpd_resp_set_type(pxReq, "text/html");
  httpd_resp_send(pxReq, acHtml, HTTPD_RESP_USE_STRLEN);

  return ESP_OK;
}

/* The content of the /graph iframe: same self-refreshing pattern as /reading above, so the red
 * current-reading dot on the curve plot also stays live without ever reloading the settings form. */
#define WEBPAGE_GRAPH_BUFFER_SIZE 2048

static esp_err_t WebpageGraphGetHandler(httpd_req_t *pxReq)
{
  char *pcHtml = malloc(WEBPAGE_GRAPH_BUFFER_SIZE);
  size_t xOffset = 0;

  if (pcHtml == NULL)
  {
    httpd_resp_send_500(pxReq);
    return ESP_FAIL;
  }

  HtmlAppend(pcHtml, WEBPAGE_GRAPH_BUFFER_SIZE, &xOffset,
    "<html><head><meta http-equiv=\"refresh\" content=\"5\">"
    "<style>body{margin:0;padding:0}.svg-wrap{text-align:center}</style>"
    "</head><body>");

  AppendCurveSvg(pcHtml, WEBPAGE_GRAPH_BUFFER_SIZE, &xOffset);

  HtmlAppend(pcHtml, WEBPAGE_GRAPH_BUFFER_SIZE, &xOffset, "</body></html>");

  httpd_resp_set_type(pxReq, "text/html");
  httpd_resp_send(pxReq, pcHtml, HTTPD_RESP_USE_STRLEN);
  free(pcHtml);

  return ESP_OK;
}

static esp_err_t WebpageIndexGetHandler(httpd_req_t *pxReq)
{
  char *pcHtml = malloc(WEBPAGE_HTML_BUFFER_SIZE);
  char acApName[33];
  size_t xOffset = 0;
  uint8_t u8CurveCount = GetCurvePointCount();
  uint8_t u8Idx;

  if (pcHtml == NULL)
  {
    httpd_resp_send_500(pxReq);
    return ESP_FAIL;
  }

  if (ReadNvs(NVS_PARAM_AP_NAME, acApName, sizeof(acApName)) != ESP_OK)
  {
    strcpy(acApName, "Thermostat");
  }

  HtmlAppend(pcHtml, WEBPAGE_HTML_BUFFER_SIZE, &xOffset,
    "<html><head><title>%s</title>"
    "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
    "%s"
    "</head><body>"
    "<h1>%s</h1>"
    "<div class=\"card\">"
    /* Only this reading auto-refreshes (via its own page at /reading) - the settings form below
     * must never be reloaded out from under the user while they're typing into it. */
    "<iframe src=\"/reading\" title=\"Live reading\" style=\"width:100%%;height:120px;border:none\"></iframe>"
    "</div>"
    "<div class=\"card\">",
    acApName, WEBPAGE_STYLE, acApName);

  HtmlAppend(pcHtml, WEBPAGE_HTML_BUFFER_SIZE, &xOffset,
    "<iframe src=\"/graph\" title=\"Curve\" style=\"width:100%%;height:190px;border:none\"></iframe>");

  HtmlAppend(pcHtml, WEBPAGE_HTML_BUFFER_SIZE, &xOffset, "</div><div class=\"card\">"
    "<form method=\"POST\" action=\"/settings\">"
    "<label>AP name: <input type=\"text\" name=\"ap_name\" value=\"%s\"></label>"
    "<label>Off threshold (&deg;C): <input type=\"number\" step=\"0.1\" name=\"off\" value=\"%.1f\"></label>"
    "<label>On threshold (&deg;C): <input type=\"number\" step=\"0.1\" name=\"on\" value=\"%.1f\"></label>"
    "<label><input type=\"checkbox\" name=\"invert\" style=\"width:auto\" %s> Invert output "
    "(for an active-low relay)</label>"
    "<p>Characteristic curve (Volt to &deg;C), up to %d points - leave a row blank to skip it:</p>"
    "<table><tr><th>Volt</th><th>&deg;C</th></tr>",
    acApName, (double) GetThermostatOffThresholdC(), (double) GetThermostatOnThresholdC(),
    GetDigOutInvert() ? "checked" : "", CURVE_POINT_MAX);

  for (u8Idx = 0; u8Idx < CURVE_POINT_MAX; u8Idx++)
  {
    if (u8Idx < u8CurveCount)
    {
      CURVE_POINT xPt = GetCurvePoint(u8Idx);

      HtmlAppend(pcHtml, WEBPAGE_HTML_BUFFER_SIZE, &xOffset,
        "<tr><td><input type=\"number\" step=\"0.01\" name=\"cv%u\" value=\"%.2f\"></td>"
        "<td><input type=\"number\" step=\"0.1\" name=\"ct%u\" value=\"%.1f\"></td></tr>",
        (unsigned) u8Idx, (double) xPt.fVolts, (unsigned) u8Idx, (double) xPt.fTempC);
    }
    else
    {
      HtmlAppend(pcHtml, WEBPAGE_HTML_BUFFER_SIZE, &xOffset,
        "<tr><td><input type=\"number\" step=\"0.01\" name=\"cv%u\"></td>"
        "<td><input type=\"number\" step=\"0.1\" name=\"ct%u\"></td></tr>",
        (unsigned) u8Idx, (unsigned) u8Idx);
    }
  }

  HtmlAppend(pcHtml, WEBPAGE_HTML_BUFFER_SIZE, &xOffset,
    "</table>"
    "<button type=\"submit\">Save</button>"
    "</form>"
    "</div>"
    "</body></html>");

  httpd_resp_set_type(pxReq, "text/html");
  httpd_resp_send(pxReq, pcHtml, HTTPD_RESP_USE_STRLEN);
  free(pcHtml);

  return ESP_OK;
}

static esp_err_t WebpageSettingsPostHandler(httpd_req_t *pxReq)
{
  char acBody[512];
  char acApName[33];
  char acCurrentApName[33];
  char acOff[16];
  char acOn[16];
  char acInvert[8];
  CURVE_POINT axNewPoints[CURVE_POINT_MAX];
  uint8_t u8NewPointCount = 0;
  uint8_t u8Idx;
  int i32Remaining = (int) pxReq->content_len;
  int i32Received = 0;
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
  if (httpd_query_key_value(acBody, "off", acOff, sizeof(acOff)) != ESP_OK)
  {
    strcpy(acOff, "0");
  }
  if (httpd_query_key_value(acBody, "on", acOn, sizeof(acOn)) != ESP_OK)
  {
    strcpy(acOn, "0");
  }

  SetThermostatThresholds(strtof(acOff, NULL), strtof(acOn, NULL));

  /* Checkboxes are only present in the POST body at all when checked. */
  SetDigOutInvert(httpd_query_key_value(acBody, "invert", acInvert, sizeof(acInvert)) == ESP_OK);

  for (u8Idx = 0; u8Idx < CURVE_POINT_MAX; u8Idx++)
  {
    char acKey[16];
    char acVoltStr[16];
    char acTempStr[16];

    snprintf(acKey, sizeof(acKey), "cv%u", (unsigned) u8Idx);
    if (httpd_query_key_value(acBody, acKey, acVoltStr, sizeof(acVoltStr)) != ESP_OK || acVoltStr[0] == '\0')
    {
      continue;
    }

    snprintf(acKey, sizeof(acKey), "ct%u", (unsigned) u8Idx);
    if (httpd_query_key_value(acBody, acKey, acTempStr, sizeof(acTempStr)) != ESP_OK || acTempStr[0] == '\0')
    {
      continue;
    }

    axNewPoints[u8NewPointCount].fVolts = strtof(acVoltStr, NULL);
    axNewPoints[u8NewPointCount].fTempC = strtof(acTempStr, NULL);
    u8NewPointCount++;
  }
  SetCurvePoints(axNewPoints, u8NewPointCount);

  if (ReadNvs(NVS_PARAM_AP_NAME, acCurrentApName, sizeof(acCurrentApName)) != ESP_OK)
  {
    acCurrentApName[0] = '\0';
  }

  if (acApName[0] != '\0' && strcmp(acApName, acCurrentApName) != 0)
  {
    WriteNvs(NVS_PARAM_AP_NAME, acApName, (uint32_t) (strlen(acApName) + 1));
    xApNameChanged = true;
  }

  if (xApNameChanged)
  {
    httpd_resp_set_type(pxReq, "text/html");
    httpd_resp_send(pxReq,
      "<html><body>Saved. The device is restarting with the new AP name - "
      "reconnect your WiFi and browse to 192.168.4.1 again.</body></html>",
      HTTPD_RESP_USE_STRLEN);
    esp_restart();
  }
  else
  {
    httpd_resp_set_status(pxReq, "303 See Other");
    httpd_resp_set_hdr(pxReq, "Location", "/");
    httpd_resp_send(pxReq, NULL, 0);
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
  static const httpd_uri_t xReadingUri =
  {
    .uri = "/reading",
    .method = HTTP_GET,
    .handler = WebpageReadingGetHandler,
    .user_ctx = NULL
  };
  static const httpd_uri_t xGraphUri =
  {
    .uri = "/graph",
    .method = HTTP_GET,
    .handler = WebpageGraphGetHandler,
    .user_ctx = NULL
  };
  static const httpd_uri_t xSettingsUri =
  {
    .uri = "/settings",
    .method = HTTP_POST,
    .handler = WebpageSettingsPostHandler,
    .user_ctx = NULL
  };

  xConfig.lru_purge_enable = true;
  xConfig.keep_alive_enable = true;

  ESP_ERROR_CHECK(httpd_start(&xServer, &xConfig));
  ESP_ERROR_CHECK(httpd_register_uri_handler(xServer, &xIndexUri));
  ESP_ERROR_CHECK(httpd_register_uri_handler(xServer, &xReadingUri));
  ESP_ERROR_CHECK(httpd_register_uri_handler(xServer, &xGraphUri));
  ESP_ERROR_CHECK(httpd_register_uri_handler(xServer, &xSettingsUri));
}
