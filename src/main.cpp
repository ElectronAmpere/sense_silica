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

// Task type and scheduler API
#include "config.h"
#include "scheduler.h"

/** Custom library */
#include "timer.h"

int Task_ToggleLED(int state);

void TimerSet(void)
{
    // Auto-reload via CTC at TASK_TICKS_GCD_IN_MS
    timer1_set_period_ms(TASK_TICKS_GCD_IN_MS);
    sei(); // Enable interrupt
}

static task tasks[TOTAL_TASKS_NUM] = {
    {0, 0, 1000, 0, &Task_ToggleLED} // Toggle LED every 100 ms
};

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
    
    // Initialize scheduler with tasks
    scheduler_init(tasks, TOTAL_TASKS_NUM);

    TimerSet();

    while(1)
    {
        // Do anything, this timer is non-blocking. It will interrupt the CPU only when needed
    }
    
    // Add return so old compilers don't cry about it being missing. Under normal circumstances this will never be hit
    return 0;
}