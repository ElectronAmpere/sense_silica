#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdint.h>
#include <stddef.h>
#include "config.h"

namespace avr_embedded {

/**
 * @brief Function pointer type for a task's tick function.
 * @param state The current state of the task's state machine.
 * @return The next state of the task's state machine.
 */
using TickFunction = int (*)(int);

/**
 * @class Task
 * @brief Represents a single cooperative task in the scheduler.
 * @details This struct holds all metadata for a task, including its state,
 *          period, and a pointer to its execution function. It is designed
 *          to be used in a static array passed to the Scheduler.
 */
class Task {
public:
    // JSF AV C++ Rule 39: All constructors shall be declared explicit.
    explicit Task(uint32_t period, TickFunction tick_fct) noexcept;

    // JSF AV C++ Rule 30: A class that has a destructor shall also have a copy constructor and an assignment operator.
    // Default copy constructor, assignment operator, and destructor are sufficient here.
    Task(const Task&) = default;
    Task& operator=(const Task&) = default;
    ~Task() = default;

    // Public member functions to access and manipulate task properties
    bool isRunning() const noexcept { return running; }
    void setRunning(bool is_running) noexcept { running = is_running; }

    int getState() const noexcept { return state; }
    void setState(int new_state) noexcept { state = new_state; }

    uint32_t getPeriod() const noexcept { return period; }
    uint32_t getElapsedTime() const noexcept { return elapsedTime; }
    void resetElapsedTime() noexcept { elapsedTime = 0; }
    void incrementElapsedTime(uint32_t time) noexcept { elapsedTime += time; }

    TickFunction getTickFunction() const noexcept { return tickFct; }

private:
    // JSF AV C++ Rule 23: All data members shall be private.
    bool running;           ///< True if the task is currently executing.
    int state;              ///< The current state of the task's state machine.
    const uint32_t period;  ///< The rate at which the task should tick, in milliseconds.
    uint32_t elapsedTime;   ///< Time elapsed since the task's last tick.
    TickFunction tickFct;   ///< Pointer to the function to call for this task's tick.
};

/**
 * @class Scheduler
 * @brief A cooperative, non-preemptive task scheduler.
 * @details This class manages a fixed-size array of tasks, executing them
 *          based on a periodic timer tick. It is designed for systems where
 *          deterministic, non-preemptive multitasking is required.
 *          It follows many of the JSF Air Vehicle C++ Coding Standards.
 */
class Scheduler {
public:
    // JSF AV C++ Rule 39: All constructors shall be declared explicit.
    explicit Scheduler(Task* tasks, uint8_t num_tasks) noexcept;

    // JSF AV C++ Rule 30, 32: Prohibit copy construction and assignment.
    Scheduler(const Scheduler&) = delete;
    Scheduler& operator=(const Scheduler&) = delete;
    ~Scheduler() = default;

    /**
     * @brief Executes one tick of the scheduler.
     * @details This method should be called from a periodic timer interrupt.
     *          It iterates through the tasks, and for each task whose period
     *          has elapsed, it executes the task's tick function.
     */
    void tick() noexcept;

private:
    // JSF AV C++ Rule 23: All data members shall be private.
    Task* const g_tasks;         ///< Pointer to the array of tasks.
    const uint8_t g_tasks_num;   ///< The total number of tasks in the array.
    
    // JSF AV C++ Rule 70: The volatile keyword shall not be used.
    // Instead, atomicity is handled by the caller (e.g., in the ISR).
    uint8_t runningTasks[avr_embedded::TOTAL_TASKS_RUNNING_NUM]; ///< Array to track running task indices.
    uint8_t currentTask;                           ///< Index of the currently executing task.
};

} // namespace avr_embedded

// C-style wrapper functions for compatibility with existing C code (e.g., ISRs)
void scheduler_init(avr_embedded::Task* tasks, uint8_t tasks_num) noexcept;
void scheduler_tick() noexcept;

#endif // SCHEDULER_H