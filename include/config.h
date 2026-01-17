#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

// JSF AV C++ Rule 1: All code shall conform to ISO/IEC 14882:2003 standard.
// Using fixed-width integers for clarity and portability.

// JSF AV C++ Rule 10: The #define directive shall not be used to create constants.
// Use const or constexpr instead.
namespace avr_embedded {

/**
 * @brief The greatest common divisor (GCD) of all task periods, in milliseconds.
 * @details This value determines the fundamental tick rate of the scheduler.
 *          All task periods must be a multiple of this value.
 */
constexpr uint32_t TASK_TICKS_GCD_IN_MS = 25;

/**
 * @brief The total number of non-idle tasks configured in the application.
 */
constexpr uint8_t TOTAL_TASKS_NUM = 2;

/**
 * @brief The maximum number of tasks that can be in a 'running' state simultaneously.
 * @details This is used for bookkeeping in the scheduler. It's the total number of
 *          tasks plus one slot for the implicit idle task.
 */
constexpr uint8_t TOTAL_TASKS_RUNNING_NUM = TOTAL_TASKS_NUM + 1;

/**
 * @brief A special value used to indicate that a slot in the runningTasks array is idle.
 * @details This value should be higher than any valid task index.
 */
constexpr uint8_t IDLE_TASK_RUNNING_INDICATOR = 255;

} // namespace avr_embedded

#endif // CONFIG_H