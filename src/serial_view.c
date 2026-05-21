#include <stdint.h>
#include <stdio.h>

#include "serial_view.h"
#include "terminalio.h"

#define TERMINAL_COLUMNS 80
#define TERMINAL_ROWS    24

static uint8_t cursor_col;
static uint8_t cursor_row;

static void print_at_cursor(char character);
static void advance_cursor(void);

void serial_view_init(void)
{
    cursor_col = 1;
    cursor_row = 1;
    move_terminal_cursor(cursor_col, cursor_row);
}

void serial_view_on_mark(char preview_char)
{
    // Overwrite the in-progress character without advancing the cursor.
    print_at_cursor(preview_char);
}

void serial_view_on_submit(char submitted_char)
{
    print_at_cursor(submitted_char);
    advance_cursor();
    move_terminal_cursor(cursor_col, cursor_row);
}

void serial_view_clear_in_progress(void)
{
    // Erase the preview character and keep the cursor at the same slot.
    print_at_cursor(' ');
}

static void print_at_cursor(char character)
{
    move_terminal_cursor(cursor_col, cursor_row);
    printf("%c", character);
    move_terminal_cursor(cursor_col, cursor_row);
}

static void advance_cursor(void)
{
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
