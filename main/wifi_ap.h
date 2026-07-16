/**
 * @file wifi_ap.h
 * @brief Open (no password) WiFi access point.
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the network/event/WiFi driver stack. Must be called before StartWifiAp,
 *        and after InitNvs.
 */
void InitWifiAp(void);

/**
 * @brief Start broadcasting the access point, using the name persisted in NVS
 *        (falling back to, and persisting, a default name on first boot).
 */
void StartWifiAp(void);

#ifdef __cplusplus
}
#endif
