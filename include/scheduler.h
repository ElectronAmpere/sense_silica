#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdint.h>

typedef struct task {
    unsigned char running;        // 1 indicates task is running
    int state;                    // Current state of state machine
    unsigned long period;         // Rate at which the task should tick
    unsigned long elapsedTime;    // Time since task's last tick
    int (*TickFct)(int);          // Function to call for task's tick
} task;

// Initialize scheduler with the task table
void scheduler_init(task* tasks, unsigned char tasks_num);

// Scheduler tick; called from the Timer1 compare ISR
void scheduler_tick(void);

#endif // SCHEDULER_H