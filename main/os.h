/**
 * @file os.h
 * @brief Main application: dispatches Init/Timer/Cycle calls to the other modules.
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize every module, in dependency order.
 */
void InitOs(void);

/**
 * @brief Dispatch the 1ms timer functions. Must be called every 1ms by the caller's scheduling loop.
 */
void TimerOs(void);

/**
 * @brief Dispatch the 20ms cycle functions. Must be called every 20ms by the caller's scheduling loop.
 */
void CycleOs(void);

#ifdef __cplusplus
}
#endif
