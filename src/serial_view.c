#include <stdint.h>
#include <stdio.h>

#include "serial_view.h"
#include "terminalio.h"
#include "ledmatrix.h"

#define TERMINAL_COLUMNS 80
#define TERMINAL_ROWS    24

static uint8_t cursor_col;
static uint8_t cursor_row;

static void print_at_cursor(char character, DisplayParameter colour);
static void advance_cursor(void);
static DisplayParameter colour_to_terminal_display(uint8_t colour);

void serial_view_init(void)
{
    cursor_col = 1;
    cursor_row = 1;
    move_terminal_cursor(cursor_col, cursor_row);
}

void serial_view_on_mark(char preview_char)
{
    // Overwrite the in-progress character without advancing the cursor.
    print_at_cursor(preview_char, FG_RED);
}

void serial_view_on_submit(char submitted_char)
{
    // Button submissions are fixed in green to match the LED matrix.
    serial_view_on_submit_colour(submitted_char, COLOUR_GREEN);
}

void serial_view_on_submit_colour(char submitted_char, uint8_t colour)
{
    // Print a completed character in the colour used by the LED matrix.
    print_at_cursor(submitted_char, colour_to_terminal_display(colour));
    advance_cursor();
    move_terminal_cursor(cursor_col, cursor_row);
}

void serial_view_clear_in_progress(void)
{
    // Erase the preview character and keep the cursor at the same slot.
    print_at_cursor(' ', FG_WHITE);
}

static void print_at_cursor(char character, DisplayParameter colour)
{
    // Move, colour, print, reset and restore the cursor to the same cell.
    move_terminal_cursor(cursor_col, cursor_row);
    set_terminal_display(colour);
    printf("%c", character);
    reset_terminal_display();
    move_terminal_cursor(cursor_col, cursor_row);
}

static void advance_cursor(void)
{
    // Advance across an 80x24 terminal and wrap for at least 200 characters.
    cursor_col++;
    if (cursor_col > TERMINAL_COLUMNS)
    {
        cursor_col = 1;
        cursor_row++;
        if (cursor_row > TERMINAL_ROWS)
        {
            cursor_row = 1;
        }
    }
}

static DisplayParameter colour_to_terminal_display(uint8_t colour)
{
    // Yellow serial input uses both red and green LEDs on the matrix.
    if (colour == COLOUR_YELLOW)
    {
        return FG_YELLOW;
    }

    // Red previews and green submissions keep the terminal colour matched.
    if (colour == COLOUR_RED)
    {
        return FG_RED;
    }
    return FG_GREEN;
}
