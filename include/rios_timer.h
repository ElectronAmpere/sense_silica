#ifndef __RIOS_TIMER_H__
#define __RIOS_TIMER_H__

/**
 * Simple Blink Example for RIoS
 * https://www.cs.ucr.edu/~vahid/pubs/wese12_rios.pdf
 * www.cs.ucr.edu/~vahid/rios/
 */
void TimerSet(int milliseconds);
void TimerOn(void);
void TimerOff(void);

#endif // __RIOS_TIMER_H__