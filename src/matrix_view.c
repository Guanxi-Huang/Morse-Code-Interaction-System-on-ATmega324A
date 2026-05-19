#include <stdint.h>

#include "matrix_view.h"
#include "display.h"
#include "ledmatrix.h"

#define RIGHT_EDGE_X 13
#define SMALL_CHAR_COLUMNS 3
#define CHAR_STEP_COLUMNS 4

static uint8_t submitted_chars_on_matrix;
static uint8_t shift_pending;

static void shift_left_one_character(void);

void matrix_view_init(void)
{
    submitted_chars_on_matrix = 0;
    shift_pending = 0;
    ledmatrix_clear();
}

void matrix_view_on_mark(char preview_char)
{
    if (shift_pending)
    {
        shift_left_one_character();
        shift_pending = 0;
    }

    // Show the current in-progress decoding in red at the right edge.
    draw_small_char(preview_char, RIGHT_EDGE_X, COLOUR_RED);
}

void matrix_view_on_submit(char submitted_char)
{
    // Spaces naturally draw as an empty three-column glyph.
    draw_small_char(submitted_char, RIGHT_EDGE_X, COLOUR_GREEN);

    if (submitted_chars_on_matrix < 4)
    {
        submitted_chars_on_matrix++;
    }
    shift_pending = 1;
}

void matrix_view_tick(void)
{
    // Tier A does not animate the LED matrix over time.
}

static void shift_left_one_character(void)
{
    for (uint8_t i = 0; i < CHAR_STEP_COLUMNS; i++)
    {
        ledmatrix_shift_left();
    }
}
