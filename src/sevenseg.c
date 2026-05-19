#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>

#include "sevenseg.h"

#define SEGMENT_SELECT_PIN PD2
#define SEGMENT_DP         0x80
#define SEGMENT_DASH       0x40
#define SEGMENT_BLANK      0x00

static const uint8_t hex_segment_patterns[16] =
{
    0x3F, 0x06, 0x5B, 0x4F,
    0x66, 0x6D, 0x7D, 0x07,
    0x7F, 0x6F, 0x77, 0x7C,
    0x39, 0x5E, 0x79, 0x71
};

static volatile uint8_t left_digit_pattern;
static volatile uint8_t right_digit_pattern;
static volatile uint8_t showing_left_digit;

void sevenseg_init(void)
{
    // PC0-PC7 drive A,B,C,D,E,F,G,DP and PD2 selects the active digit.
    DDRC = 0xFF;
    DDRD |= (1 << SEGMENT_SELECT_PIN);
    PORTC = 0;
    PORTD &= ~(1 << SEGMENT_SELECT_PIN);
    sevenseg_set_values(0, 0);

    // Timer2 refreshes the multiplexed display every 1ms.
    TCCR2A = (1 << WGM21);
    TCCR2B = (1 << CS22);
    OCR2A = 124;
    TIMSK2 = (1 << OCIE2A);
}

void sevenseg_set_values(uint8_t submitted_count, uint8_t current_marks)
{
    uint8_t right_pattern = SEGMENT_BLANK;
    uint8_t right_is_on = current_marks > 0;

    if (current_marks > 0 && current_marks <= 9)
    {
        right_pattern = hex_segment_patterns[current_marks];
    }
    else if (current_marks > 9)
    {
        right_pattern = SEGMENT_DASH;
    }

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        left_digit_pattern = hex_segment_patterns[submitted_count & 0x0F];
        if (right_is_on)
        {
            left_digit_pattern |= SEGMENT_DP;
        }
        right_digit_pattern = right_pattern;
    }
}

ISR(TIMER2_COMPA_vect)
{
    // Blank before switching digits to avoid ghosting.
    PORTC = SEGMENT_BLANK;

    if (showing_left_digit)
    {
        showing_left_digit = 0;
        PORTD &= ~(1 << SEGMENT_SELECT_PIN);
        PORTC = right_digit_pattern;
    }
    else
    {
        showing_left_digit = 1;
        PORTD |= (1 << SEGMENT_SELECT_PIN);
        PORTC = left_digit_pattern;
    }
}
