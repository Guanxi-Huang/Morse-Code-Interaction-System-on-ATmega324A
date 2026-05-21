#ifndef IO_LED_H_
#define IO_LED_H_

#include <stdint.h>

typedef enum
{
    IO_LED_MARK_NONE = 0,
    IO_LED_MARK_DOT,
    IO_LED_MARK_DASH
} io_led_mark_t;

typedef struct
{
    uint8_t lit;
    io_led_mark_t mark_type;
    uint8_t is_final;
} io_led_beat_t;

void io_led_init(void);
void io_led_on_dot(void);
void io_led_on_dash(void);
void io_led_on_submit_character(void);
void io_led_on_submit_word(void);
void io_led_clear_queue(void);
void io_led_enqueue_silence_beat(void);
void io_led_enqueue_morse_code(uint8_t code, uint8_t enable_buzzer_marks);
void io_led_tick(void);

#endif /* IO_LED_H_ */
