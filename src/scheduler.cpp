#include <avr/interrupt.h>
#include <util/atomic.h>

#include "config.h"
#include "scheduler.h"

static task* g_tasks = nullptr;
static unsigned char g_tasks_num = 0;

static unsigned char runningTasks[TOTAL_TASKS_RUNNING_NUM] = {IDLE_TASK_RUNNING_INDICATOR};
static unsigned char currentTask = 0;

void scheduler_init(task* tasks, unsigned char tasks_num)
{
    g_tasks = tasks;
    g_tasks_num = tasks_num;
    runningTasks[0] = IDLE_TASK_RUNNING_INDICATOR;
    currentTask = 0;
}

void scheduler_tick(void)
{
    if (!g_tasks || g_tasks_num == 0) {
        return;
    }

    for (unsigned char index = 0; index < g_tasks_num; ++index) {
        task& t = g_tasks[index];
        if (  (t.elapsedTime >= t.period)
           && (runningTasks[currentTask] > index)
           && (!t.running)
        ) {
            // Protect scheduler bookkeeping with atomic block
            ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
                t.elapsedTime = 0;
                t.running = 1;
                currentTask += 1;
                runningTasks[currentTask] = index;
            }

            // Allow nested interrupts while task tick executes
            sei();
            t.state = t.TickFct(t.state);
            cli();

            // Cleanup bookkeeping atomically
            ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
                t.running = 0;
                runningTasks[currentTask] = IDLE_TASK_RUNNING_INDICATOR;
                currentTask -= 1;
            }
        }

        // Elapsed time update
        t.elapsedTime += TASK_TICKS_GCD_IN_MS;
    }
}
