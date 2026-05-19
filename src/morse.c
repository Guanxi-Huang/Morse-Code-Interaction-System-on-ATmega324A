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


/* Internal Function Declarations */
void initialise_hardware(void);
void start_morse(void);
void start_splash_screen(void);
void handle_inputs(void);
void handle_button_event(button_event_t event);
void refresh_progress_outputs(void);


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
    io_led_init();
    sevenseg_init();
    timer_init();
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
    button_event_t event = buttons_poll();

    if (event != BTN_NONE)
    {
        handle_button_event(event);
    }

    if (timer_tick_consume())
    {
        io_led_tick();
        matrix_view_tick();
    }
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

void refresh_progress_outputs(void)
{
    char preview_char = input_current_char();

    matrix_view_on_mark(preview_char);
    serial_view_on_mark(preview_char);
    sevenseg_set_values(input_chars_submitted(), input_marks_count());
}
