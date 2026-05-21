#include <stdint.h>

#include "matrix_view.h"
#include "display.h"
#include "ledmatrix.h"

#define RIGHT_EDGE_X 13
#define CHAR_STEP_COLUMNS 4

static uint8_t shift_frames_remaining;

static void matrix_view_snap_animation(void);

void matrix_view_init(void)
{
    // Reset the animation state before clearing the physical matrix.
    shift_frames_remaining = 0;
    ledmatrix_clear();
}

void matrix_view_on_mark(char preview_char)
{
    // Finish any previous submit animation before drawing the new preview.
    matrix_view_snap_animation();

    // Show the current in-progress decoding in red at the right edge.
    draw_small_char(preview_char, RIGHT_EDGE_X, COLOUR_RED);
}

void matrix_view_on_submit(char submitted_char)
{
    // Button-submitted characters are completed in green.
    matrix_view_on_submit_colour(submitted_char, COLOUR_GREEN);
}

void matrix_view_on_submit_colour(char submitted_char, uint8_t colour)
{
    // Finish any previous submit animation before placing this character.
    matrix_view_snap_animation();

    // Draw the completed glyph at the right edge without moving it yet.
    draw_small_char(submitted_char, RIGHT_EDGE_X, colour);

    // Schedule four 100ms frames so a new small glyph can fit on the right.
    shift_frames_remaining = CHAR_STEP_COLUMNS;
}

void matrix_view_clear_in_progress(void)
{
    // Finish submitted glyph movement before erasing the preview area.
    matrix_view_snap_animation();

    // Draw a blank glyph where the red in-progress character was displayed.
    draw_small_char(' ', RIGHT_EDGE_X, COLOUR_BLACK);
}

void matrix_view_tick(void)
{
    // Move one animation frame per 100ms tick if a submit is in progress.
    if (shift_frames_remaining > 0)
    {
        ledmatrix_shift_left();
        shift_frames_remaining--;
    }
}

static void matrix_view_snap_animation(void)
{
    // Push any remaining frames immediately so new input starts from rest.
    while (shift_frames_remaining > 0)
    {
        ledmatrix_shift_left();
        shift_frames_remaining--;
    }
}
