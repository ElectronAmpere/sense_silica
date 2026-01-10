/**
 * Simple Blink Example for RIoS
 * https://www.cs.ucr.edu/~vahid/pubs/wese12_rios.pdf
 * https://www.cs.ucr.edu/~vahid/rios/
 * https://www.cs.ucr.edu/~vahid/pes/RITools/ 
 */
/** Generic library */
#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/delay.h>
#include <math.h>
#include <util/atomic.h>

typedef struct task {
    unsigned char running; // 1 indicates task is running
    int state;         // Current state of state machine
    unsigned long period;    // Rate at which the task should tick
    unsigned long elapsedTime; // Time since task's last tick
    int (*TickFct)(int); // Function to call for task's tick with state information
} task;

/** Custom library */
#include "timer.h"

#define TASK_TICKS_GCD_IN_MS (25) // Timer ticks for scheduler
#define TOTAL_TASKS_NUM (1) // Number of non-idle tasks
#define TOTAL_TASKS_RUNNING_NUM (TOTAL_TASKS_NUM + 1) // +1 for idle task
#define IDLE_TASK_RUNNING_INDICATOR (255) 

unsigned char runningTasks[TOTAL_TASKS_RUNNING_NUM] = {IDLE_TASK_RUNNING_INDICATOR}; // Array to track running tasks
unsigned char currentTask = 0; // Index of the current task

int Task_ToggleLED(int state);
void TimerISR(void);

ISR(TIMER1_COMPA_vect) {
    // loaded on compare match no reload needed
    TimerISR();
}

void TimerSet(void)
{
    // Auto-reload via CTC at TASK_TICKS_GCD_IN_MS
    timer1_set_period_ms(TASK_TICKS_GCD_IN_MS);
    sei(); // Enable interrupt
}

static task tasks[TOTAL_TASKS_NUM] = {
    {0, 0, 100, 0, &Task_ToggleLED} // Toggle LED every 100 ms
};

void TimerISR(void)
{
    unsigned char index;
    for (index = 0; index < TOTAL_TASKS_NUM; ++index) { // Heart of scheduler code
        if (  (tasks[index].elapsedTime >= tasks[index].period)     // Task ready
           && (runningTasks[currentTask] > index)               // Task priority > current task priority
           && (!tasks[index].running)                           // Task not already running (no self-preemption)
        ) {
            // Protect scheduler bookkeeping with atomic block
            ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
                tasks[index].elapsedTime = 0;          // Reset time since last tick
                tasks[index].running = 1;              // Mark as running
                currentTask += 1;
                runningTasks[currentTask] = index;     // Add to runningTasks
            }

            // Allow nested interrupts while task tick executes (optional, mirrors prior SREG |= 0x80)
            sei();
            tasks[index].state = tasks[index].TickFct(tasks[index].state); // Execute tick
            cli(); // Return to atomic context before cleanup (mirrors prior SREG &= 0x7F)

            // Cleanup bookkeeping atomically
            ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
                tasks[index].running = 0;                    // Mark as not running
                runningTasks[currentTask] = IDLE_TASK_RUNNING_INDICATOR;    // Remove from runningTasks
                currentTask -= 1;
            }
        }

        // Elapsed time update; interrupts are disabled unless nested were enabled during TickFct
        tasks[index].elapsedTime += TASK_TICKS_GCD_IN_MS;
    }
}

int Task_ToggleLED(int state) {
    switch(state) {
        case 0:
            // Toggle LED on PB5
            PORTB ^= _BV(PORTB5);
            state = 0;
            break;
        default:
            state = 0;
            break;
    }
    return state;
}

int main() {

    // Set PB5 as output without clobbering other pins
    DDRB |= _BV(DDB5);

    TimerSet();

    while(1)
    {
        // Do anything, this timer is non-blocking. It will interrupt the CPU only when needed
    }
    
    // Add return so old compilers don't cry about it being missing. Under normal circumstances this will never be hit
    return 0;
}