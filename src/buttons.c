#include <stdint.h>
#include <avr/io.h>

#include "buttons.h"
#include "timer.h"

#define BUTTON_MASK         ((1 << PB0) | (1 << PB1) | (1 << PB2))
#define BUTTON_DEBOUNCE_MS  20

static uint8_t previous_button_state;
static uint8_t pending_button_edges;
static uint32_t last_edge_time_ms[3];

void buttons_init(void)
{
    // Buttons are active-high external inputs on PB0, PB1 and PB2.
    DDRB &= ~BUTTON_MASK;
    PORTB &= ~BUTTON_MASK;
    buttons_sync();
}

void buttons_sync(void)
{
    // Match the stored state to the pins so an existing press is not replayed.
    previous_button_state = PINB & BUTTON_MASK;
    pending_button_edges = 0;
}

button_event_t buttons_poll(void)
{
    uint8_t current_button_state = PINB & BUTTON_MASK;
    uint8_t new_edges = current_button_state & ~previous_button_state;
    uint32_t now_ms = timer_millis();

    previous_button_state = current_button_state;

    for (uint8_t i = 0; i < 3; i++)
    {
        uint8_t bit_mask = (1 << i);
        if ((new_edges & bit_mask)
                && (now_ms - last_edge_time_ms[i] >= BUTTON_DEBOUNCE_MS))
        {
            pending_button_edges |= bit_mask;
            last_edge_time_ms[i] = now_ms;
        }
    }

    if (pending_button_edges & (1 << PB0))
    {
        pending_button_edges &= ~(1 << PB0);
        return BTN_DOT;
    }
    if (pending_button_edges & (1 << PB1))
    {
        pending_button_edges &= ~(1 << PB1);
        return BTN_DASH;
    }
    if (pending_button_edges & (1 << PB2))
    {
        pending_button_edges &= ~(1 << PB2);
        return BTN_SUBMIT;
    }
    return BTN_NONE;
}
