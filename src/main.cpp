#include <Arduino.h>
// Keep main minimal; setup APIs moved to setup.cpp
#include "setup.h"

int main(void) {
    // Manually call the Arduino core init function.
    init();

    setupHardware();
    setupScheduler();
    
    // Enable global interrupts
    sei();

    // The scheduler takes over from here.
    while (true) {
        // This loop will be preempted by the timer interrupt for task scheduling.
        // It can be used for low-priority background processing or power-saving modes.
    }

    return 0; // This line is unreachable.
}