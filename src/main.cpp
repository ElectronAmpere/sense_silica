/**
 * Simple Blink Example for RIoS
 * https://www.cs.ucr.edu/~vahid/pubs/wese12_rios.pdf
 * https://www.cs.ucr.edu/~vahid/rios/
 * https://www.cs.ucr.edu/~vahid/pes/RITools/ 
 */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>

#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#define TIMER1_PRESCALER       1024UL

// Prescaler bit mask for CS12:CS10 = 1 0 1 (1024)
#define TIMER1_CS_BITS        (_BV(CS12) | _BV(CS10))

// Atomic helpers for 16-bit Timer1 registers (datasheet requires byte-ordering)
static inline void timer1_write_ocr1a_atomic(uint16_t value)
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        OCR1AH = (uint8_t)(value >> 8);
        OCR1AL = (uint8_t)(value & 0xFF);
    }
}

static inline void timer1_write_tcnt1_atomic(uint16_t value)
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        TCNT1H = (uint8_t)(value >> 8);
        TCNT1L = (uint8_t)(value & 0xFF);
    }
}

static inline uint16_t timer1_read_tcnt1_atomic(void)
{
    uint8_t low, high;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        // Read low first to latch high byte per datasheet
        low = TCNT1L;
        high = TCNT1H;
    }
    return (uint16_t)((high << 8) | low);
}

ISR(TIMER1_COMPA_vect) {
    // Toggle LED on PB5 at each compare match; no reload needed
    PORTB ^= _BV(PORTB5);
}

void timer1_set_period_ms(uint16_t period_ms)
{
    // Stop timer: clear prescaler bits
    TCCR1B &= ~(_BV(CS10) | _BV(CS11) | _BV(CS12));

    // Configure CTC mode (WGM12 = 1, others 0)
    TCCR1A &= ~(_BV(WGM10) | _BV(WGM11));
    TCCR1B = (TCCR1B & ~(_BV(WGM13) | _BV(WGM12))) | _BV(WGM12);

    // Compute compare value for requested period
    // ticks = (F_CPU / prescaler) * (period_ms / 1000)
    uint32_t ticks = ((uint32_t)F_CPU / (uint32_t)TIMER1_PRESCALER) * (uint32_t)period_ms / 1000UL;
    if (ticks == 0) {
        ticks = 1; // minimum
    }
    if (ticks > 65536UL) {
        ticks = 65536UL; // clamp to 16-bit timer range
    }
    // Atomic write to 16-bit OCR1A (high then low)
    timer1_write_ocr1a_atomic((uint16_t)(ticks - 1UL));

    // Clear pending compare flag and counter
    TIFR1 |= _BV(OCF1A);
    // Atomic clear of 16-bit counter
    timer1_write_tcnt1_atomic(0);

    // Enable compare A interrupt only
    TIMSK1 &= ~(_BV(TOIE1));
    TIMSK1 |= _BV(OCIE1A);

    // Start Timer1 with prescaler = 1024
    TCCR1B |= TIMER1_CS_BITS;
}

void TimerSet(void)
{
    // Auto-reload via CTC at 100 ms
    timer1_set_period_ms(1000);
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