#include <stdint.h>
#include <avr/io.h>

#include "io_led.h"
#include "buzzer.h"

#define LED_LOW_MASK       0xFC
#define LED_HIGH_MASK      ((1 << PD6) | (1 << PD7))
#define LED_QUEUE_LEN      48

static uint8_t led_history;
static io_led_beat_t led_queue[LED_QUEUE_LEN];
static uint8_t queue_head;
static uint8_t queue_tail;
static uint8_t queued_beats;
static uint8_t mark_gap_pending;

static void enqueue_beat(uint8_t bit, io_led_mark_t mark_type, uint8_t is_final);
static uint8_t dequeue_beat(io_led_beat_t *beat);
static void enqueue_mark(uint8_t beats, io_led_mark_t mark_type, uint8_t is_final);
static void push_led_history(uint8_t bit);
static void write_led_outputs(void);

void io_led_init(void)
{
    // L0-L5 are on PA2-PA7, and L6-L7 are on PD6-PD7.
    DDRA |= LED_LOW_MASK;
    DDRD |= LED_HIGH_MASK;
    led_history = 0;
    queue_head = 0;
    queue_tail = 0;
    queued_beats = 0;
    mark_gap_pending = 0;
    write_led_outputs();
    buzzer_silence();
}

void io_led_on_dot(void)
{
    // Add an automatic one-beat gap before a following mark.
    if (mark_gap_pending)
    {
        enqueue_beat(0, IO_LED_MARK_NONE, 0);
    }
    enqueue_beat(1, IO_LED_MARK_NONE, 0);
    mark_gap_pending = 1;
}

void io_led_on_dash(void)
{
    // Add a three-beat dash, with no buzzer metadata for button input.
    if (mark_gap_pending)
    {
        enqueue_beat(0, IO_LED_MARK_NONE, 0);
    }
    enqueue_mark(3, IO_LED_MARK_NONE, 0);
    mark_gap_pending = 1;
}

void io_led_on_submit_character(void)
{
    // Character gaps are three low beats after the previous mark.
    enqueue_beat(0, IO_LED_MARK_NONE, 0);
    enqueue_beat(0, IO_LED_MARK_NONE, 0);
    enqueue_beat(0, IO_LED_MARK_NONE, 0);
    mark_gap_pending = 0;
}

void io_led_on_submit_word(void)
{
    // A word gap is five low beats; extend the previous character gap by two.
    enqueue_beat(0, IO_LED_MARK_NONE, 0);
    enqueue_beat(0, IO_LED_MARK_NONE, 0);
    mark_gap_pending = 0;
}

void io_led_clear_queue(void)
{
    // Drop pending beats when serial input snaps the current playback.
    queue_head = 0;
    queue_tail = 0;
    queued_beats = 0;
    mark_gap_pending = 0;
    buzzer_silence();
}

void io_led_enqueue_silence_beat(void)
{
    // Insert a one-beat quiet separator before a new serial pattern.
    enqueue_beat(0, IO_LED_MARK_NONE, 0);
}

void io_led_enqueue_morse_code(uint8_t code, uint8_t enable_buzzer_marks)
{
    uint8_t prefix_mask = 0x80;
    uint8_t mark_mask;
    uint8_t first_mark = 1;

    // Find the prefix bit that marks the start of the Morse pattern.
    while (prefix_mask > 0 && !(code & prefix_mask))
    {
        prefix_mask >>= 1;
    }

    // Invalid or empty patterns have no serial animation to enqueue.
    if (prefix_mask <= 1)
    {
        return;
    }

    // Walk each mark from left to right, adding gaps between marks.
    for (mark_mask = (prefix_mask >> 1); mark_mask > 0; mark_mask >>= 1)
    {
        io_led_mark_t mark_type;
        uint8_t is_final_mark = (mark_mask == 1) ? 1 : 0;

        if (!first_mark)
        {
            enqueue_beat(0, IO_LED_MARK_NONE, 0);
        }
        first_mark = 0;

        if (code & mark_mask)
        {
            mark_type = enable_buzzer_marks ? IO_LED_MARK_DASH : IO_LED_MARK_NONE;
            enqueue_mark(3, mark_type, is_final_mark);
        }
        else
        {
            mark_type = enable_buzzer_marks ? IO_LED_MARK_DOT : IO_LED_MARK_NONE;
            enqueue_mark(1, mark_type, is_final_mark);
        }
    }

    // Finish the serial pattern with silence after the final mark.
    enqueue_beat(0, IO_LED_MARK_NONE, 0);
    mark_gap_pending = 0;
}

void io_led_tick(void)
{
    io_led_beat_t beat;

    // Pop one queued beat every 100ms and mirror it to LEDs and buzzer.
    if (dequeue_beat(&beat))
    {
        push_led_history(beat.lit);
        write_led_outputs();
        buzzer_on_beat(&beat);
    }
}

static void enqueue_beat(uint8_t bit, io_led_mark_t mark_type, uint8_t is_final)
{
    // Ignore new beats if the fixed queue is full.
    if (queued_beats >= LED_QUEUE_LEN)
    {
        return;
    }

    led_queue[queue_tail].lit = bit & 0x01;
    led_queue[queue_tail].mark_type = mark_type;
    led_queue[queue_tail].is_final = is_final ? 1 : 0;
    queue_tail = (queue_tail + 1) % LED_QUEUE_LEN;
    queued_beats++;
}

static uint8_t dequeue_beat(io_led_beat_t *beat)
{
    // Report empty queue without changing the LED history.
    if (queued_beats == 0)
    {
        return 0;
    }

    *beat = led_queue[queue_head];
    queue_head = (queue_head + 1) % LED_QUEUE_LEN;
    queued_beats--;
    return 1;
}

static void enqueue_mark(uint8_t beats, io_led_mark_t mark_type, uint8_t is_final)
{
    // Add each beat of a dot or dash with shared buzzer metadata.
    for (uint8_t i = 0; i < beats; i++)
    {
        enqueue_beat(1, mark_type, is_final);
    }
}

static void push_led_history(uint8_t bit)
{
    // Shift the newest beat onto L0 at the right edge.
    led_history = (led_history << 1) | (bit & 0x01);
}

static void write_led_outputs(void)
{
    // Split the eight history bits across PA2-PA7 and PD6-PD7 wiring.
    PORTA = (PORTA & ~LED_LOW_MASK) | ((led_history & 0x3F) << 2);
    PORTD = (PORTD & ~LED_HIGH_MASK) | (led_history & 0xC0);
}
