/**
 * Simple Blink Example for RIoS
 * https://www.cs.ucr.edu/~vahid/pubs/wese12_rios.pdf
 * https://www.cs.ucr.edu/~vahid/rios/
 * https://www.cs.ucr.edu/~vahid/pes/RITools/ 
 */
#include <avr/io.h>
#include <avr/interrupt.h>

ISR(TIMER1_OVF_vect) {

    // Toggle the 5th data register of PORTB
    PORTB ^= (1 << PORTB5);

    // 100ms for 16MHz clock
    TCNT1 = 63974;
}

int main() {
    
    DDRB = (1 << DDB5); // Set 5th data direction register of PORTB. A set value means output
    TCNT1 = 63974; // 100 ms for 16MHz clock
    TCCR1A = 0x00; // Set normal counter mode
    TCCR1B = (1<<CS10) | (1<<CS12); // Set 1024 pre-scaler
    TIMSK1 = (1 << TOIE1); // Set overflow interrupt enable bit
    sei(); // Enable interrupts globally
    
    while(1)
    {
        // Do anything, this timer is non-blocking. It will interrupt the CPU only when needed
    }
    
    // Add return so old compilers don't cry about it being missing. Under normal circumstances this will never be hit
    return 0;
}