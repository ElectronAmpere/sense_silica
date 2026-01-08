#include <Arduino.h>

/**
 * Simple Blink Example for RIoS
 * https://www.cs.ucr.edu/~vahid/pubs/wese12_rios.pdf
 * www.cs.ucr.edu/~vahid/rios/
 */
#include "../include/rios.h"
#include "../include/rios_timer.h"

// Example tick functions (must match signature)
int TickFct_1(int state);
int TickFct_2(int state);
int TickFct_3(int state);

int main(void)
{
    // Initialize kernel with 25 ms base tick (GCD)
    rios_init(25);

    // Register tasks: (function, init_state, period_ms)
    rios_add_task(TickFct_1, -1, 25);
    rios_add_task(TickFct_2, -1, 50);
    rios_add_task(TickFct_3, -1, 100);

    // Start kernel (starts timer)
    rios_start();

    // Main can sleep; scheduler runs in ISR
    while (1) {
        // Optionally enter low-power mode or handle background housekeeping
        ;
    }
}

