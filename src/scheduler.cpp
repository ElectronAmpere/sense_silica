#include "scheduler.h"
#include "config.h"
#include <avr/interrupt.h>
#include <util/atomic.h>
#include <stddef.h> // For nullptr

// JSF AV C++ Rule 12: Use file scope for objects not visible externally.
namespace {
    // Global pointer to the scheduler instance
    avr_embedded::Scheduler* g_scheduler_instance = nullptr;
}

// C-style wrapper implementations
void scheduler_init(avr_embedded::Task* tasks, uint8_t tasks_num) noexcept {
    // JSF AV C++ Rule 18: All variables shall be initialized before use.
    // This static instance is initialized on first use.
    static avr_embedded::Scheduler scheduler(tasks, tasks_num);
    g_scheduler_instance = &scheduler;
}

void scheduler_tick() noexcept {
    // JSF AV C++ Rule 58: All if, else if, else, while, do, and for statements shall be compound statements.
    if (g_scheduler_instance != nullptr) {
        g_scheduler_instance->tick();
    }
}

namespace avr_embedded {

// JSF AV C++ Rule 39: All constructors shall be declared explicit.
Task::Task(uint32_t period, TickFunction tick_fct) noexcept
    : running(false),
      state(0),
      period(period),
      elapsedTime(0),
      tickFct(tick_fct) {
    // JSF AV C++ Rule 43: The body of a constructor shall not be empty.
    // Initialization is done in the member initializer list.
}

// JSF AV C++ Rule 39: All constructors shall be declared explicit.
Scheduler::Scheduler(Task* tasks, uint8_t num_tasks) noexcept
    : g_tasks(tasks),
      g_tasks_num(num_tasks),
      currentTask(0) {
    // JSF AV C++ Rule 18: All variables shall be initialized before use.
    for (uint8_t i = 0; i < TOTAL_TASKS_RUNNING_NUM; ++i) {
        runningTasks[i] = IDLE_TASK_RUNNING_INDICATOR;
    }
}

void Scheduler::tick() noexcept {
    // JSF AV C++ Rule 58: All if, else if, else, while, do, and for statements shall be compound statements.
    if (g_tasks == nullptr || g_tasks_num == 0) {
        return;
    }

    // JSF AV C++ Rule 81: Unsigned integers shall be used for indices.
    for (uint8_t index = 0; index < g_tasks_num; ++index) {
        Task& t = g_tasks[index];
        
        // JSF AV C++ Rule 68: A for loop shall contain a single iterator.
        // This loop follows that rule.

        // JSF AV C++ Rule 60: All if statements shall be compound statements.
        if ((t.getElapsedTime() >= t.getPeriod()) &&
            (runningTasks[currentTask] > index) &&
            (!t.isRunning())) {
            
            // JSF AV C++ Rule 70: The volatile keyword shall not be used.
            // ATOMIC_BLOCK is used for ensuring atomicity of critical sections.
            ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
                t.resetElapsedTime();
                t.setRunning(true);
                currentTask++;
                runningTasks[currentTask] = index;
            }

            // JSF AV C++ Rule 164: Long or complex processing in an ISR shall be avoided.
            // We allow nested interrupts here so higher priority interrupts can fire,
            // but this task must complete swiftly.
            sei();
            t.setState(t.getTickFunction()(t.getState()));
            cli();

            ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
                t.setRunning(false);
                runningTasks[currentTask] = IDLE_TASK_RUNNING_INDICATOR;
                currentTask--;
            }
        }

        t.incrementElapsedTime(TASK_TICKS_GCD_IN_MS);
    }
}

} // namespace avr_embedded
