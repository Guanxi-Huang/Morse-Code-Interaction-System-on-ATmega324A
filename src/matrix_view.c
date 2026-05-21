#include <stdint.h>
#include <avr/eeprom.h>

#include "matrix_view.h"
#include "display.h"
#include "ledmatrix.h"

#define MATRIX_HISTORY_LEN       50
#define EEPROM_SCROLLBACK_SLOTS  64
#define EEPROM_VALID_MARK        0xA5

typedef struct
{
    char character;
    uint8_t colour;
} matrix_entry_t;

typedef struct
{
    uint32_t sequence;
    char character;
    uint8_t colour;
    uint8_t valid;
} matrix_eeprom_record_t;

static matrix_eeprom_record_t EEMEM eeprom_scrollback[EEPROM_SCROLLBACK_SLOTS];

static matrix_entry_t history[MATRIX_HISTORY_LEN];
static uint8_t history_count;
static uint8_t large_font_enabled;
static uint8_t brightness_level;
static uint8_t shift_frames_remaining;
static uint8_t right_margin_columns;
static uint8_t preview_active;
static char preview_char;
static uint16_t scroll_columns;
static uint8_t latest_eeprom_slot;
static uint32_t latest_eeprom_sequence;

static void matrix_view_render(void);
static void matrix_view_update_physical_column(uint8_t x,
        int16_t virtual_column);
static int16_t matrix_view_rightmost_virtual_column(void);
static void matrix_view_snap_animation(void);
static void matrix_view_push_history(char submitted_char, uint8_t colour);
static void matrix_view_load_eeprom_history(void);
static void matrix_view_store_eeprom_entry(char submitted_char, uint8_t colour);
static uint8_t matrix_view_glyph_width(void);
static uint8_t matrix_view_step_width(void);
static uint16_t matrix_view_content_width(void);
static uint16_t matrix_view_max_scroll(void);
static uint8_t matrix_view_column_colour(int16_t virtual_column, uint8_t row);
static uint8_t scale_colour(uint8_t colour);

void matrix_view_init(void)
{
    // Reset runtime view state, then restore saved submitted characters.
    history_count = 0;
    large_font_enabled = 0;
    brightness_level = 15;
    shift_frames_remaining = 0;
    right_margin_columns = matrix_view_step_width();
    preview_active = 0;
    preview_char = ' ';
    scroll_columns = 0;
    latest_eeprom_slot = EEPROM_SCROLLBACK_SLOTS - 1;
    latest_eeprom_sequence = 0;

    // EEPROM restoration gives Tier C scrollback without affecting input state.
    matrix_view_load_eeprom_history();
    ledmatrix_clear();
    matrix_view_render();
}

void matrix_view_on_mark(char preview_character)
{
    // New input returns from scrollback to the present before drawing preview.
    matrix_view_snap_animation();
    scroll_columns = 0;
    preview_active = 1;
    preview_char = preview_character;
    right_margin_columns = 0;

    // Redraw so the preview font, colour and brightness are all current.
    matrix_view_render();
}

void matrix_view_on_submit(char submitted_char)
{
    // Button-submitted characters are completed in green.
    matrix_view_on_submit_colour(submitted_char, COLOUR_GREEN);
}

void matrix_view_on_submit_colour(char submitted_char, uint8_t colour)
{
    // Finish the previous movement before appending the newly submitted glyph.
    matrix_view_snap_animation();
    scroll_columns = 0;
    preview_active = 0;
    right_margin_columns = 0;

    // Store this completed character for display, scrollback and EEPROM.
    matrix_view_push_history(submitted_char, colour);
    matrix_view_store_eeprom_entry(submitted_char, colour);

    // Place the new glyph on the right edge and animate it left one character.
    shift_frames_remaining = matrix_view_step_width();
    matrix_view_render();
}

void matrix_view_clear_in_progress(void)
{
    // Remove the red preview while preserving all submitted history.
    matrix_view_snap_animation();
    preview_active = 0;
    preview_char = ' ';
    scroll_columns = 0;
    right_margin_columns = (history_count > 0) ? matrix_view_step_width() : 0;
    matrix_view_render();
}

void matrix_view_tick(void)
{
    // Move one animation frame per 100ms tick if a submit is in progress.
    if (shift_frames_remaining > 0)
    {
        ledmatrix_shift_left();
        shift_frames_remaining--;
        right_margin_columns++;
    }
}

void matrix_view_set_large_font(uint8_t enabled)
{
    // Switch S1 updates all visible columns immediately when the font changes.
    enabled = enabled ? 1 : 0;
    if (large_font_enabled == enabled)
    {
        return;
    }

    // Complete any old-font animation before redrawing with the new font.
    matrix_view_snap_animation();
    large_font_enabled = enabled;
    right_margin_columns = preview_active ? 0 : matrix_view_step_width();
    if (scroll_columns > matrix_view_max_scroll())
    {
        scroll_columns = matrix_view_max_scroll();
    }
    matrix_view_render();
}

void matrix_view_scroll(int8_t direction)
{
    uint16_t max_scroll;
    uint16_t old_scroll = scroll_columns;
    int16_t rightmost_virtual_column;

    // Scrollback starts from a stable display, not a half-finished animation.
    matrix_view_snap_animation();
    max_scroll = matrix_view_max_scroll();

    // Positive direction moves into the past; negative returns to the present.
    if (direction > 0 && scroll_columns < max_scroll)
    {
        scroll_columns++;
    }
    else if (direction < 0 && scroll_columns > 0)
    {
        scroll_columns--;
    }

    // Redraw only when the scroll position actually changed.
    if (scroll_columns != old_scroll)
    {
        rightmost_virtual_column = matrix_view_rightmost_virtual_column();
        if (direction > 0)
        {
            // Moving into the past shifts the visible image right by one column.
            ledmatrix_shift_right();
            matrix_view_update_physical_column(0,
                    rightmost_virtual_column - (MATRIX_NUM_COLUMNS - 1));
        }
        else
        {
            // Returning to the present shifts the visible image left by one column.
            ledmatrix_shift_left();
            matrix_view_update_physical_column(MATRIX_NUM_COLUMNS - 1,
                    rightmost_virtual_column);
        }
    }
}

void matrix_view_adjust_brightness(int8_t direction)
{
    uint8_t old_brightness = brightness_level;

    // Keep brightness in the requested 15 discernible levels.
    if (direction > 0 && brightness_level < 15)
    {
        brightness_level++;
    }
    else if (direction < 0 && brightness_level > 1)
    {
        brightness_level--;
    }

    // Redraw visible columns when the selected brightness changed.
    if (brightness_level != old_brightness)
    {
        matrix_view_render();
    }
}

static void matrix_view_render(void)
{
    uint16_t content_width = matrix_view_content_width();
    int16_t rightmost_virtual_column;

    // An empty history with no preview is most efficiently represented by clear.
    if (content_width == 0)
    {
        ledmatrix_clear();
        return;
    }

    // Clamp scrollback so the view never moves past the oldest/newest content.
    if (scroll_columns > matrix_view_max_scroll())
    {
        scroll_columns = matrix_view_max_scroll();
    }

    // Convert physical columns into virtual scrollback columns.
    rightmost_virtual_column = matrix_view_rightmost_virtual_column();
    for (uint8_t x = 0; x < MATRIX_NUM_COLUMNS; x++)
    {
        int16_t virtual_column = rightmost_virtual_column
                - (MATRIX_NUM_COLUMNS - 1 - x);

        // Draw this physical column from the matching virtual scrollback column.
        matrix_view_update_physical_column(x, virtual_column);
    }
}

static void matrix_view_update_physical_column(uint8_t x,
        int16_t virtual_column)
{
    uint8_t pixels[MATRIX_NUM_ROWS];

    // Build one physical LED matrix column from the virtual history.
    for (uint8_t row = 0; row < MATRIX_NUM_ROWS; row++)
    {
        pixels[row] = matrix_view_column_colour(virtual_column, row);
    }
    ledmatrix_update_column(x, pixels);
}

static int16_t matrix_view_rightmost_virtual_column(void)
{
    // The right edge tracks the newest content, right margin, and scrollback.
    return (int16_t)(matrix_view_content_width() - 1
            + right_margin_columns - scroll_columns);
}

static void matrix_view_snap_animation(void)
{
    // Push any remaining frames immediately so new input starts from rest.
    while (shift_frames_remaining > 0)
    {
        ledmatrix_shift_left();
        shift_frames_remaining--;
        right_margin_columns++;
    }
}

static void matrix_view_push_history(char submitted_char, uint8_t colour)
{
    // Keep at least 50 completed characters for Tier C joystick scrollback.
    if (history_count < MATRIX_HISTORY_LEN)
    {
        history[history_count].character = submitted_char;
        history[history_count].colour = colour;
        history_count++;
        return;
    }

    // Drop the oldest entry when the fixed scrollback buffer is full.
    for (uint8_t i = 1; i < MATRIX_HISTORY_LEN; i++)
    {
        history[i - 1] = history[i];
    }
    history[MATRIX_HISTORY_LEN - 1].character = submitted_char;
    history[MATRIX_HISTORY_LEN - 1].colour = colour;
}

static void matrix_view_load_eeprom_history(void)
{
    matrix_eeprom_record_t record;
    uint8_t valid_count = 0;
    uint8_t newest_found = 0;

    // Find the newest valid EEPROM slot and count how much history exists.
    for (uint8_t slot = 0; slot < EEPROM_SCROLLBACK_SLOTS; slot++)
    {
        eeprom_read_block(&record, &eeprom_scrollback[slot], sizeof(record));
        if (record.valid == EEPROM_VALID_MARK)
        {
            valid_count++;
            if (!newest_found || record.sequence > latest_eeprom_sequence)
            {
                newest_found = 1;
                latest_eeprom_slot = slot;
                latest_eeprom_sequence = record.sequence;
            }
        }
    }

    // If EEPROM is empty, keep the display history empty as well.
    if (!newest_found)
    {
        return;
    }

    // Load the newest 50 records in chronological order from the ring.
    if (valid_count > MATRIX_HISTORY_LEN)
    {
        valid_count = MATRIX_HISTORY_LEN;
    }
    for (uint8_t i = 0; i < valid_count; i++)
    {
        uint8_t distance_from_newest = valid_count - 1 - i;
        uint8_t slot = (latest_eeprom_slot + EEPROM_SCROLLBACK_SLOTS
                - distance_from_newest) % EEPROM_SCROLLBACK_SLOTS;

        eeprom_read_block(&record, &eeprom_scrollback[slot], sizeof(record));
        if (record.valid == EEPROM_VALID_MARK)
        {
            history[history_count].character = record.character;
            history[history_count].colour = record.colour;
            history_count++;
        }
    }
}

static void matrix_view_store_eeprom_entry(char submitted_char, uint8_t colour)
{
    matrix_eeprom_record_t record;

    // Move to the next ring slot so no EEPROM cell is rewritten too often.
    latest_eeprom_slot = (latest_eeprom_slot + 1) % EEPROM_SCROLLBACK_SLOTS;
    latest_eeprom_sequence++;

    // Store only submitted characters and their matrix colours.
    record.sequence = latest_eeprom_sequence;
    record.character = submitted_char;
    record.colour = colour;
    record.valid = EEPROM_VALID_MARK;
    eeprom_update_block(&record, &eeprom_scrollback[latest_eeprom_slot],
            sizeof(record));
}

static uint8_t matrix_view_glyph_width(void)
{
    // Read the width from display.c so both fonts share one source of truth.
    return get_char_glyph_width(large_font_enabled);
}

static uint8_t matrix_view_step_width(void)
{
    // One blank column separates neighbouring submitted characters.
    return matrix_view_glyph_width() + 1;
}

static uint16_t matrix_view_content_width(void)
{
    uint16_t width;

    // Submitted history uses one spacer column after each completed glyph.
    width = (uint16_t)history_count * matrix_view_step_width();
    if (preview_active)
    {
        width += matrix_view_glyph_width();
    }
    else if (width > 0)
    {
        width--;
    }
    return width;
}

static uint16_t matrix_view_max_scroll(void)
{
    uint16_t content_width = matrix_view_content_width();
    uint16_t present_right_edge;

    // No content means there is nowhere to scroll.
    if (content_width == 0)
    {
        return 0;
    }

    // The maximum scroll places the oldest virtual column at physical x=0.
    present_right_edge = content_width - 1 + right_margin_columns;
    if (present_right_edge < (MATRIX_NUM_COLUMNS - 1))
    {
        return 0;
    }
    return present_right_edge - (MATRIX_NUM_COLUMNS - 1);
}

static uint8_t matrix_view_column_colour(int16_t virtual_column, uint8_t row)
{
    uint8_t glyph_width = matrix_view_glyph_width();
    uint8_t step_width = matrix_view_step_width();
    uint16_t history_width = (uint16_t)history_count * step_width;
    uint8_t glyph_column;
    uint8_t entry_index;
    uint8_t entry_column;
    char character;
    uint8_t colour;

    // Columns outside the virtual content are blank.
    if (virtual_column < 0
            || (uint16_t)virtual_column >= matrix_view_content_width())
    {
        return COLOUR_BLACK;
    }

    // Select either a submitted history character or the active preview glyph.
    if ((uint16_t)virtual_column < history_width)
    {
        entry_index = (uint8_t)((uint16_t)virtual_column / step_width);
        entry_column = (uint8_t)((uint16_t)virtual_column % step_width);
        if (entry_column >= glyph_width)
        {
            return COLOUR_BLACK;
        }
        character = history[entry_index].character;
        colour = history[entry_index].colour;
    }
    else if (preview_active)
    {
        entry_column = (uint8_t)((uint16_t)virtual_column - history_width);
        character = preview_char;
        colour = COLOUR_RED;
    }
    else
    {
        return COLOUR_BLACK;
    }

    // A set glyph bit lights this row at the current brightness.
    glyph_column = get_char_glyph_column(character, entry_column,
            large_font_enabled);
    if (glyph_column & (1 << row))
    {
        return scale_colour(colour);
    }
    return COLOUR_BLACK;
}

static uint8_t scale_colour(uint8_t colour)
{
    uint8_t green = (colour >> 4) & 0x0F;
    uint8_t red = colour & 0x0F;

    // Scale both colour channels while keeping non-zero colours visible.
    green = (uint8_t)((green * brightness_level) / 15);
    red = (uint8_t)((red * brightness_level) / 15);
    if (((colour >> 4) & 0x0F) && green == 0)
    {
        green = 1;
    }
    if ((colour & 0x0F) && red == 0)
    {
        red = 1;
    }
    return (green << 4) | red;
}
