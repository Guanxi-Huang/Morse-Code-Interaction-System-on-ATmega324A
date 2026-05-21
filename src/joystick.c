#include <stdint.h>
#include <avr/io.h>

#include "joystick.h"

#define JOYSTICK_X_CHANNEL          0
#define JOYSTICK_Y_CHANNEL          1
#define ADC_CENTER_VALUE            512
#define JOYSTICK_START_DEADZONE     170
#define JOYSTICK_STOP_DEADZONE      90
#define SCROLL_DELAY_FAST_MS        100
#define SCROLL_DELAY_SLOW_MS        550

static uint16_t adc_read_channel(uint8_t channel);
static uint16_t abs_from_center(uint16_t value);

void joystick_init(void)
{
    // PA0 and PA1 are analogue joystick inputs, so leave them as inputs.
    DDRA &= ~((1 << PA0) | (1 << PA1));
    PORTA &= ~((1 << PA0) | (1 << PA1));

    // Use AVCC as reference and enable ADC with a /64 prescaler.
    ADMUX = (1 << REFS0);
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1);
}

uint16_t joystick_read_x(void)
{
    // Read the horizontal axis from ADC0.
    return adc_read_channel(JOYSTICK_X_CHANNEL);
}

uint16_t joystick_read_y(void)
{
    // Read the vertical axis from ADC1.
    return adc_read_channel(JOYSTICK_Y_CHANNEL);
}

int8_t joystick_axis_direction(uint16_t value, int8_t previous_direction)
{
    // Hysteresis prevents borderline analogue noise from causing fast repeats.
    if (previous_direction > 0)
    {
        return (value > ADC_CENTER_VALUE + JOYSTICK_STOP_DEADZONE) ? 1 : 0;
    }
    if (previous_direction < 0)
    {
        return (value < ADC_CENTER_VALUE - JOYSTICK_STOP_DEADZONE) ? -1 : 0;
    }

    // A neutral axis must move past the larger deadzone to start an action.
    if (value > ADC_CENTER_VALUE + JOYSTICK_START_DEADZONE)
    {
        return 1;
    }
    if (value < ADC_CENTER_VALUE - JOYSTICK_START_DEADZONE)
    {
        return -1;
    }
    return 0;
}

uint16_t joystick_scroll_delay_ms(uint16_t x_value)
{
    uint16_t magnitude = abs_from_center(x_value);
    uint16_t usable_range = ADC_CENTER_VALUE - JOYSTICK_START_DEADZONE;

    // Full tilt scrolls quickly; slight tilt scrolls slowly but controllably.
    if (magnitude <= JOYSTICK_START_DEADZONE)
    {
        return SCROLL_DELAY_SLOW_MS;
    }
    magnitude -= JOYSTICK_START_DEADZONE;
    return SCROLL_DELAY_SLOW_MS
            - ((SCROLL_DELAY_SLOW_MS - SCROLL_DELAY_FAST_MS)
                    * magnitude / usable_range);
}

static uint16_t adc_read_channel(uint8_t channel)
{
    // Select one ADC channel without disturbing the AVCC reference setting.
    ADMUX = (ADMUX & 0xF0) | (channel & 0x0F);

    // Start a conversion and wait for the short hardware conversion to finish.
    ADCSRA |= (1 << ADSC);
    while (ADCSRA & (1 << ADSC))
    {
        ; // wait for ADC hardware to complete this sample
    }
    return ADC;
}

static uint16_t abs_from_center(uint16_t value)
{
    // Return the unsigned distance from the joystick's nominal centre value.
    if (value >= ADC_CENTER_VALUE)
    {
        return value - ADC_CENTER_VALUE;
    }
    return ADC_CENTER_VALUE - value;
}
