#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>

#include "timer.h"

static volatile uint32_t elapsed_ms;
static volatile uint8_t hundred_ms_count;
static volatile uint8_t pending_100ms_ticks;

void timer_init(void)
{
    // Timer0 CTC mode gives one interrupt every 1ms at 8MHz with /64.
    TCCR0A = (1 << WGM01);
    TCCR0B = (1 << CS01) | (1 << CS00);
    OCR0A = 124;
    TIMSK0 = (1 << OCIE0A);
}

uint8_t timer_tick_consume(void)
{
    uint8_t tick_available = 0;

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        if (pending_100ms_ticks > 0)
        {
            pending_100ms_ticks--;
            tick_available = 1;
        }
    }
    return tick_available;
}

uint32_t timer_millis(void)
{
    uint32_t snapshot;

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        snapshot = elapsed_ms;
    }
    return snapshot;
}

ISR(TIMER0_COMPA_vect)
{
    elapsed_ms++;

    if (++hundred_ms_count >= 100)
    {
        hundred_ms_count = 0;
        if (pending_100ms_ticks < 255)
        {
            pending_100ms_ticks++;
        }
    }
}
