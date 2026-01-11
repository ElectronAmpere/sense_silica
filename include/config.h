#ifndef CONFIG_H
#define CONFIG_H

// Scheduler tick period in milliseconds (tasksPeriodGCD)
#define TASK_TICKS_GCD_IN_MS (25)

// Number of non-idle tasks configured in the application
#define TOTAL_TASKS_NUM (2)

// Running tasks bookkeeping: +1 slot for implicit idle task
#define TOTAL_TASKS_RUNNING_NUM (TOTAL_TASKS_NUM + 1)

// Idle task indicator (lowest priority)
#define IDLE_TASK_RUNNING_INDICATOR (255)

#endif // CONFIG_H