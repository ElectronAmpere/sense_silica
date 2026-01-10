/**
 * Simple Blink Example for RIoS
 * https://www.cs.ucr.edu/~vahid/pubs/wese12_rios.pdf
 * https://www.cs.ucr.edu/~vahid/rios/
 * https://www.cs.ucr.edu/~vahid/pes/RITools/ 
 */
#include <avr/interrupt.h>
#include <avr/io.h>
#include "timer.h"

#define TIMER_TICKS_IN_MS (1000)

ISR(TIMER1_COMPA_vect) {
    // Toggle LED on PB5 at each compare match; no reload needed
    PORTB ^= _BV(PORTB5);
}

void TimerSet(void)
{
    // Auto-reload via CTC at TIMER_TICKS_IN_MS
    timer1_set_period_ms(TIMER_TICKS_IN_MS);
    sei(); // Enable interrupt
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