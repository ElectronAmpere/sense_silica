#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include "scheduler.h" // Use C-style wrapper
#include "timer.h"

// JSF AV C++ Rule 12: The static keyword shall be used for functions and objects with file scope.
namespace {

// JSF AV C++ Rule 10: Use const/constexpr for constants.
#ifndef F_CPU
constexpr uint32_t F_CPU = 16000000UL;
#endif
constexpr uint16_t TIMER1_PRESCALER = 1024UL;

/**
 * @brief Atomically writes a 16-bit value to the OCR1A register.
 * @details Follows AVR datasheet recommendation for 16-bit register access.
 *          This function is marked inline to encourage the compiler to place it
 *          directly where it's called, avoiding function call overhead.
 * @param value The 16-bit value to write.
 */
inline void timer1_write_ocr1a_atomic(uint16_t value) noexcept {
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        OCR1AH = static_cast<uint8_t>(value >> 8);
        OCR1AL = static_cast<uint8_t>(value & 0xFF);
    }
}

/**
 * @brief Atomically writes a 16-bit value to the TCNT1 register.
 * @details Follows AVR datasheet recommendation for 16-bit register access.
 * @param value The 16-bit value to write.
 */
inline void timer1_write_tcnt1_atomic(uint16_t value) noexcept {
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        TCNT1H = static_cast<uint8_t>(value >> 8);
        TCNT1L = static_cast<uint8_t>(value & 0xFF);
    }
}

} // anonymous namespace

/**
 * @brief Configures and starts Timer1 to generate interrupts at a specified period.
 * @details This function sets up Timer1 in Clear Timer on Compare Match (CTC) mode.
 *          It calculates the required compare value based on the system clock,
 *          prescaler, and desired period. It enables the compare match interrupt
 *          and starts the timer.
 * @param period_ms The desired interrupt period in milliseconds.
 */
void timer1_set_period_ms(uint16_t period_ms) noexcept {
    // JSF AV C++ Rule 13: All declarations should have file scope.
    // Stop timer: clear prescaler bits
    TCCR1B &= ~(_BV(CS10) | _BV(CS11) | _BV(CS12));

    // Configure CTC mode (WGM12 = 1, others 0)
    TCCR1A &= ~(_BV(WGM10) | _BV(WGM11));
    TCCR1B = (TCCR1B & ~(_BV(WGM13))) | _BV(WGM12);

    // JSF AV C++ Rule 177: The comma operator shall not be used.
    // JSF AV C++ Rule 18: All variables shall be initialized before use.
    // Compute compare value for requested period
    // ticks = (F_CPU / prescaler) * (period_ms / 1000)
    uint32_t ticks = (static_cast<uint32_t>(F_CPU) / TIMER1_PRESCALER * period_ms) / 1000UL;
    
    // JSF AV C++ Rule 60: All if, else if, else, while, do, and for statements shall be compound statements.
    if (ticks == 0) {
        ticks = 1; // minimum
    }
    // JSF AV C++ Rule 20: All floating-point constants shall be written with a decimal point and at least one digit.
    // Not applicable here, but good practice.
    if (ticks > 65535UL) {
        ticks = 65535UL; // clamp to 16-bit timer range
    }
    
    timer1_write_ocr1a_atomic(static_cast<uint16_t>(ticks - 1UL));

    // Clear pending compare flag and counter
    TIFR1 |= _BV(OCF1A);
    timer1_write_tcnt1_atomic(0);

    // Enable compare A interrupt only
    TIMSK1 &= ~(_BV(TOIE1));
    TIMSK1 |= _BV(OCIE1A);

    // Start Timer1 with prescaler = 1024
    TCCR1B |= (_BV(CS12) | _BV(CS10));
}

/**
 * @brief Interrupt Service Routine for Timer1 Compare Match A.
 * @details This ISR is automatically called by the hardware when Timer1's counter
 *          matches the value in OCR1A. It serves as the main timing source
 *          for the application by dispatching to the scheduler's tick function.
 */
ISR(TIMER1_COMPA_vect) {
    // JSF AV C++ Rule 164: Long or complex processing in an ISR shall be avoided.
    // The scheduler tick is designed to be brief.
    scheduler_tick();
}
