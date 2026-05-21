#include <stdint.h>

#include "sync_mode.h"
#include "buttons.h"
#include "timer.h"

#define DOT_DASH_THRESHOLD_MS 200
#define FIRST_SUBMIT_MS      1000
#define SECOND_SUBMIT_MS     2000

typedef enum
{
    SYNC_STATE_RELEASED = 0,
    SYNC_STATE_PRESSED
} sync_state_t;

static sync_state_t sync_state;
static uint32_t press_start_ms;
static uint32_t release_start_ms;
static uint8_t release_submits_sent;

void sync_mode_init(void)
{
    // Start the synchronous FSM from the current B0 level.
    sync_mode_reset();
}

void sync_mode_reset(void)
{
    // Forget any partial press/release timing when modes change.
    if (buttons_b0_held())
    {
        sync_state = SYNC_STATE_PRESSED;
        press_start_ms = timer_millis();
    }
    else
    {
        sync_state = SYNC_STATE_RELEASED;
        release_start_ms = timer_millis();
    }

    // No automatic submit has been sent for the current release period.
    release_submits_sent = 0;
}

sync_event_t sync_mode_poll(void)
{
    uint32_t now_ms = timer_millis();
    uint8_t b0_is_held = buttons_b0_held();

    // A new press starts timing a dot/dash and cancels release submits.
    if (sync_state == SYNC_STATE_RELEASED && b0_is_held)
    {
        sync_state = SYNC_STATE_PRESSED;
        press_start_ms = now_ms;
        release_submits_sent = 0;
        return SYNC_EVENT_NONE;
    }

    // Releasing B0 converts the measured hold time into dot or dash.
    if (sync_state == SYNC_STATE_PRESSED && !b0_is_held)
    {
        sync_state = SYNC_STATE_RELEASED;
        release_start_ms = now_ms;
        release_submits_sent = 0;

        if ((now_ms - press_start_ms) < DOT_DASH_THRESHOLD_MS)
        {
            return SYNC_EVENT_DOT;
        }
        return SYNC_EVENT_DASH;
    }

    // While released, emit at most two automatic submits at 1s and 2s.
    if (sync_state == SYNC_STATE_RELEASED)
    {
        uint32_t released_ms = now_ms - release_start_ms;

        if (release_submits_sent == 0 && released_ms >= FIRST_SUBMIT_MS)
        {
            release_submits_sent = 1;
            return SYNC_EVENT_SUBMIT;
        }

        if (release_submits_sent == 1 && released_ms >= SECOND_SUBMIT_MS)
        {
            release_submits_sent = 2;
            return SYNC_EVENT_SUBMIT;
        }
    }

    // No synchronous input event is ready this loop.
    return SYNC_EVENT_NONE;
}
