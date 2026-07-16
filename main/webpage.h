/**
 * @file webpage.h
 * @brief Web UI: shows the measured value/output state and lets the user view/edit settings.
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Start the HTTP server and register the page/settings handlers.
 */
void InitWebpage(void);

#ifdef __cplusplus
}
#endif
