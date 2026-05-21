#include <stdint.h>
#include <avr/io.h>

#include "buzzer.h"

#define BUZZER_PIN         PD5
#define DOT_TOP_VALUE      7999
#define DASH_TOP_VALUE     5713

static void buzzer_connect_output(void);

void buzzer_init(void)
{
    // Configure PD5/OC1A as the buzzer output and keep it silent at boot.
    DDRD |= (1 << BUZZER_PIN);
    PORTD &= ~(1 << BUZZER_PIN);

    // Reset Timer1 control registers before selecting the PWM mode.
    TCCR1A = 0;
    TCCR1B = 0;

    // Use Timer1 Fast PWM mode 14 with ICR1 as TOP and no prescaler.
    ICR1 = DOT_TOP_VALUE;
    OCR1A = 0;
    TCCR1A = (1 << WGM11);
    TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS10);
    buzzer_silence();
}

void buzzer_silence(void)
{
    // Disconnect OC1A and drive the pin low to avoid narrow PWM clicks.
    TCCR1A &= ~((1 << COM1A1) | (1 << COM1A0));
    PORTD &= ~(1 << BUZZER_PIN);
}

void buzzer_on_beat(const io_led_beat_t *beat)
{
    uint16_t top_value;

    // Gaps and button-generated beats do not produce a tone.
    // If beat does not exist, stop the buzzer and exit the function
    if (beat == 0)
    {
    buzzer_silence();
    return;
    }
    
    // If beat exists but is not lit, stop the buzzer and exit the function
    if (beat->lit == 0)
    {
        buzzer_silence();
        return;
    }

    // If beat exists and is lit, but mark does not exist, stop the buzzer and exit the function
    if (beat->mark_type == IO_LED_MARK_NONE)
    {
        buzzer_silence();
        return;
    }

    // Dashes use a higher frequency than dots by about 1.4x.
    top_value = (beat->mark_type == IO_LED_MARK_DASH)
        ? DASH_TOP_VALUE
        : DOT_TOP_VALUE;

    // Apply the selected frequency and final-mark duty-cycle rule.
    ICR1 = top_value;
    if (beat->is_final)
    {
        OCR1A = (uint16_t)((top_value + 1) / 10);
    }
    else
    {
        OCR1A = (uint16_t)(top_value / 2);
    }
    buzzer_connect_output();
}

static void buzzer_connect_output(void)
{
    // Connect OC1A in non-inverting PWM mode when a tone is required.
    TCCR1A = (TCCR1A & ~(1 << COM1A0)) | (1 << COM1A1) | (1 << WGM11);
}
