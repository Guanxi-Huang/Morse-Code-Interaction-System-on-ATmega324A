#include <stdint.h>
#include <avr/io.h>

#include "io_led.h"

#define LED_LOW_MASK       0xFC
#define LED_HIGH_MASK      ((1 << PD6) | (1 << PD7))
#define LED_QUEUE_LEN      32

static uint8_t led_history;
static uint8_t led_queue[LED_QUEUE_LEN];
static uint8_t queue_head;
static uint8_t queue_tail;
static uint8_t queued_beats;
static uint8_t mark_gap_pending;

static void enqueue_beat(uint8_t bit);
static uint8_t dequeue_beat(uint8_t *bit);
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
}

void io_led_on_dot(void)
{
    if (mark_gap_pending)
    {
        enqueue_beat(0);
    }
    enqueue_beat(1);
    mark_gap_pending = 1;
}

void io_led_on_dash(void)
{
    if (mark_gap_pending)
    {
        enqueue_beat(0);
    }
    enqueue_beat(1);
    enqueue_beat(1);
    enqueue_beat(1);
    mark_gap_pending = 1;
}

void io_led_on_submit_character(void)
{
    // Character gaps are three low beats after the previous mark.
    enqueue_beat(0);
    enqueue_beat(0);
    enqueue_beat(0);
    mark_gap_pending = 0;
}

void io_led_on_submit_word(void)
{
    // A word gap is five low beats; extend the previous character gap by two.
    enqueue_beat(0);
    enqueue_beat(0);
    mark_gap_pending = 0;
}

void io_led_tick(void)
{
    uint8_t bit;

    if (dequeue_beat(&bit))
    {
        push_led_history(bit);
        write_led_outputs();
    }
}

static void enqueue_beat(uint8_t bit)
{
    if (queued_beats >= LED_QUEUE_LEN)
    {
        return;
    }

    led_queue[queue_tail] = bit & 0x01;
    queue_tail = (queue_tail + 1) % LED_QUEUE_LEN;
    queued_beats++;
}

static uint8_t dequeue_beat(uint8_t *bit)
{
    if (queued_beats == 0)
    {
        return 0;
    }

    *bit = led_queue[queue_head];
    queue_head = (queue_head + 1) % LED_QUEUE_LEN;
    queued_beats--;
    return 1;
}

static void push_led_history(uint8_t bit)
{
    led_history = (led_history << 1) | (bit & 0x01);
}

static void write_led_outputs(void)
{
    PORTA = (PORTA & ~LED_LOW_MASK) | ((led_history & 0x3F) << 2);
    PORTD = (PORTD & ~LED_HIGH_MASK) | (led_history & 0xC0);
}
