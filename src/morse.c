/*
 * morse.c
 *
 * Main file
 *
 * Authors: Peter Sutton, Bradley Stone, Ryan Wang
 * Modified by <YOUR NAME HERE>, <YOUR STUDENT ID HERE>
 */ 


/* Definitions */
#include <stdint.h>
#include <stdio.h>
#include <avr/io.h>
#include <avr/interrupt.h>

/* Internal Library Includes */
#include "serialio.h"
#include "terminalio.h"
#include "ledmatrix.h"
#include "display.h"
#include "encoding.h"
#include "timer.h"
#include "buttons.h"
#include "input_state.h"
#include "matrix_view.h"
#include "io_led.h"
#include "sevenseg.h"
#include "serial_view.h"
#include "sync_mode.h"
#include "buzzer.h"
#include "joystick.h"

#define SYNC_SWITCH_PIN PD3
#define FONT_SWITCH_PIN PD4


/* Internal Function Declarations */
void initialise_hardware(void);
void start_morse(void);
void start_splash_screen(void);
void handle_inputs(void);
void handle_button_event(button_event_t event);
void handle_sync_event(sync_event_t event);
void handle_serial_input(void);
void handle_mode_transition(uint8_t sync_enabled);
void handle_font_selection(void);
void handle_joystick(void);
void refresh_progress_outputs(void);
uint8_t sync_mode_enabled(void);
uint8_t large_font_enabled(void);

static uint8_t previous_sync_mode;
static uint8_t previous_large_font;
static int8_t previous_scroll_direction;
static int8_t previous_brightness_direction;
static uint32_t next_scroll_ms;
static uint32_t next_brightness_ms;


int main(void)
{
    initialise_hardware();
    start_splash_screen();
    start_morse();
}

void initialise_hardware(void)
{
    spi_setup_master(128); // init LED matrix
    // Setup serial port for 19200 baud communication
    init_serial_stdio(19200);
    buttons_init();
    input_state_init();
    buzzer_init();
    io_led_init();
    sevenseg_init();
    joystick_init();
    DDRD &= ~(1 << SYNC_SWITCH_PIN); // use S0 on PD3 as an active-high input
    PORTD &= ~(1 << SYNC_SWITCH_PIN); // leave the switch line externally driven
    DDRD &= ~(1 << FONT_SWITCH_PIN); // use S1 on PD4 as an active-high input
    PORTD &= ~(1 << FONT_SWITCH_PIN); // leave the font switch externally driven
    timer_init();
    sync_mode_init();
    sei(); // enable global interrupts
}

void start_splash_screen(void)
{
    // draw sigil on LED matrix
    start_splash_display();
    move_terminal_cursor(10, 6);
    printf("CSSE%d AVR Project", 2010); // change if masters student
    move_terminal_cursor(10, 8);
    printf("\"Morse Code Emulator\"");
    move_terminal_cursor(10, 10);
    printf("%d, Semester %s", 2026, "One");
    move_terminal_cursor(10, 12);
    // "%ld" is "long decimal", since a student number is bigger than 2**16
    printf("By %s (%ld)", "Student Name", 48000000);
    
    // Wait until a button is pressed
    while(!(PINB & 0x07))
    {
        ; // do nothing til button press
    }
    ledmatrix_clear();
    
}

void start_morse(void)
{
    // Clear the serial terminal
    clear_terminal();
    buttons_sync(); // consume the splash-screen button press
    matrix_view_init();
    serial_view_init();
    previous_sync_mode = sync_mode_enabled();
    previous_large_font = large_font_enabled();
    previous_scroll_direction = 0;
    previous_brightness_direction = 0;
    next_scroll_ms = timer_millis();
    next_brightness_ms = timer_millis();
    sync_mode_reset();
    matrix_view_set_large_font(previous_large_font);
    sevenseg_set_values(input_chars_submitted(), input_marks_count());

    while(1)
    {
        // Handle any button or key inputs
        handle_inputs();
    }
    // should never reach
}

void handle_inputs(void)
{
    button_event_t event;
    uint8_t sync_now;

    // Advance periodic animations before handling new input events.
    if (timer_tick_consume())
    {
        io_led_tick();
        matrix_view_tick();
    }

    // Always poll buttons so edge state stays current across mode changes.
    event = buttons_poll();
    sync_now = sync_mode_enabled();

    // Apply Tier C controls that affect only the LED matrix view.
    handle_font_selection();
    handle_joystick();

    // Clear unfinished input when S0 changes mode.
    if (sync_now != previous_sync_mode)
    {
        handle_mode_transition(sync_now);
        previous_sync_mode = sync_now;
    }

    // In synchronous mode only B0 timing is used; B1/B2 edge events are ignored.
    if (sync_now)
    {
        sync_event_t sync_event = sync_mode_poll();
        if (sync_event != SYNC_EVENT_NONE)
        {
            handle_sync_event(sync_event);
        }
        handle_serial_input();
        return;
    }

    // In asynchronous mode, the button rising edges directly create inputs.
    if (event != BTN_NONE)
    {
        handle_button_event(event);
    }
    handle_serial_input();
}

void handle_button_event(button_event_t event)
{
    submit_result_t submit_result;

    switch (event)
    {
        case BTN_DOT:
            input_add_dot();
            io_led_on_dot();
            io_led_tick(); // Show the first beat immediately.
            refresh_progress_outputs();
            break;

        case BTN_DASH:
            input_add_dash();
            io_led_on_dash();
            io_led_tick(); // Show the first beat immediately.
            refresh_progress_outputs();
            break;

        case BTN_SUBMIT:
            submit_result = input_submit();
            if (submit_result == SUBMIT_CHARACTER)
            {
                io_led_on_submit_character();
                io_led_tick();
                matrix_view_on_submit(input_last_submitted_char());
                serial_view_on_submit(input_last_submitted_char());
            }
            else if (submit_result == SUBMIT_SPACE)
            {
                io_led_on_submit_word();
                io_led_tick();
                matrix_view_on_submit(' ');
                serial_view_on_submit(' ');
            }
            sevenseg_set_values(input_chars_submitted(), input_marks_count());
            break;

        case BTN_NONE:
        default:
            break;
    }
}

void handle_sync_event(sync_event_t event)
{
    // Reuse the asynchronous handlers so every Tier A output stays consistent.
    switch (event)
    {
        case SYNC_EVENT_DOT:
            handle_button_event(BTN_DOT);
            break;

        case SYNC_EVENT_DASH:
            handle_button_event(BTN_DASH);
            break;

        case SYNC_EVENT_SUBMIT:
            handle_button_event(BTN_SUBMIT);
            break;

        case SYNC_EVENT_NONE:
        default:
            break;
    }
}

void handle_serial_input(void)
{
    char received_char;
    uint8_t morse_code;
    char display_char;

    // Only read stdin when the interrupt buffer says a byte is ready.
    if (!serial_input_available())
    {
        return;
    }

    // Convert the typed character to Morse and ignore unsupported input.
    received_char = (char)fgetc(stdin);
    morse_code = char_to_morse(received_char);
    if (morse_code == 0)
    {
        return;
    }

    // Use the decoded character so terminal output is always uppercase.
    display_char = morse_to_char(morse_code);

    // A valid serial character replaces any unfinished button character.
    input_record_external_char(display_char);
    matrix_view_clear_in_progress();
    serial_view_on_submit_colour(display_char, COLOUR_YELLOW);
    sevenseg_set_values(input_chars_submitted(), input_marks_count());

    // Show the serial character in yellow and animate it left over time.
    matrix_view_on_submit_colour(display_char, COLOUR_YELLOW);

    // Snap old playback, then queue the Morse pattern from its first beat.
    io_led_clear_queue();
    io_led_enqueue_morse_code(morse_code, 1);
    io_led_tick();
}

void handle_mode_transition(uint8_t sync_enabled)
{
    // Discard the unfinished character when switching between input modes.
    (void)sync_enabled;
    input_clear_in_progress();
    matrix_view_clear_in_progress();
    serial_view_clear_in_progress();
    sync_mode_reset();
    sevenseg_set_values(input_chars_submitted(), input_marks_count());
}

void handle_font_selection(void)
{
    uint8_t font_now = large_font_enabled();

    // Redraw immediately if S1 changes between the small and large fonts.
    if (font_now != previous_large_font)
    {
        previous_large_font = font_now;
        matrix_view_set_large_font(font_now);
    }
}

void handle_joystick(void)
{
    uint32_t now_ms = timer_millis();
    uint16_t x_value = joystick_read_x();
    uint16_t y_value = joystick_read_y();
    int8_t scroll_direction;
    int8_t brightness_direction;
    uint16_t scroll_delay;

    // X axis: right scrolls into past, left returns toward the present.
    scroll_direction = joystick_axis_direction(x_value,
            previous_scroll_direction);
    if (scroll_direction == 0)
    {
        next_scroll_ms = now_ms;
    }
    else
    {
        scroll_delay = joystick_scroll_delay_ms(x_value);
        if (scroll_direction != previous_scroll_direction
                || now_ms >= next_scroll_ms)
        {
            matrix_view_scroll(scroll_direction);
            next_scroll_ms = now_ms + scroll_delay;
        }
    }
    previous_scroll_direction = scroll_direction;

    // Y axis: high brightens, low darkens, with a one-second hold repeat.
    brightness_direction = joystick_axis_direction(y_value,
            previous_brightness_direction);
    if (brightness_direction == 0)
    {
        next_brightness_ms = now_ms;
    }
    else if (brightness_direction != previous_brightness_direction
            || now_ms >= next_brightness_ms)
    {
        matrix_view_adjust_brightness(brightness_direction);
        next_brightness_ms = now_ms + 1000;
    }
    previous_brightness_direction = brightness_direction;
}

void refresh_progress_outputs(void)
{
    char preview_char = input_current_char();

    matrix_view_on_mark(preview_char);
    serial_view_on_mark(preview_char);
    sevenseg_set_values(input_chars_submitted(), input_marks_count());
}

uint8_t sync_mode_enabled(void)
{
    // S0 high selects synchronous mode; S0 low keeps Tier A asynchronous mode.
    return (PIND & (1 << SYNC_SWITCH_PIN)) ? 1 : 0;
}

uint8_t large_font_enabled(void)
{
    // S1 high selects the five-column font; S1 low selects the three-column font.
    return (PIND & (1 << FONT_SWITCH_PIN)) ? 1 : 0;
}
