/* Modular RIoS-like scheduler (cooperative, tick-driven)
 * - rios_init(base_period_ms): Configure hardware timer
 * - rios_add_task(): Register a task (returns index or -1 on error)
 * - rios_start(): Start timer and kernel
 * The kernel runs tasks from the ISR in a non-blocking manner; tasks should avoid long blocking calls.
 */
