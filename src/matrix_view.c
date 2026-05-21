#include <stdint.h>

#include "matrix_view.h"
#include "display.h"
#include "ledmatrix.h"

#define RIGHT_EDGE_X 13
#define SMALL_CHAR_COLUMNS 3
#define CHAR_STEP_COLUMNS 4

static uint8_t submitted_chars_on_matrix;

static void shift_left_one_character(void);

void matrix_view_init(void)
{
    submitted_chars_on_matrix = 0;
    ledmatrix_clear();
}

void matrix_view_on_mark(char preview_char)
{
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

    // With incomplete-character preview, move the completed glyph left now
    // so the right edge is ready for the next red preview.
    shift_left_one_character();
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
