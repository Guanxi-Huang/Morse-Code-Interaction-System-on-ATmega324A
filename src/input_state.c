#include <stdint.h>

#include "input_state.h"
#include "encoding.h"

static uint8_t current_code;
static uint8_t marks_count;
static uint8_t consecutive_submits;
static uint8_t chars_submitted;
static char history_chars[HISTORY_LEN];
static uint8_t history_count;
static char last_submitted_char;

static void input_add_mark(uint8_t mark_bit);
static void history_push(char submitted_char);

void input_state_init(void)
{
    current_code = 1;
    marks_count = 0;
    consecutive_submits = 0;
    chars_submitted = 0;
    history_count = 0;
    last_submitted_char = ' ';
}

void input_add_dot(void)
{
    input_add_mark(0);
}

void input_add_dash(void)
{
    input_add_mark(1);
}

submit_result_t input_submit(void)
{
    if (marks_count > 0)
    {
        char submitted_char = morse_to_char(current_code);
        history_push(submitted_char);
        last_submitted_char = submitted_char;
        chars_submitted++;
        current_code = 1;
        marks_count = 0;
        consecutive_submits = 1;
        return SUBMIT_CHARACTER;
    }

    if (consecutive_submits == 1)
    {
        history_push(' ');
        last_submitted_char = ' ';
        chars_submitted++;
        consecutive_submits = 2;
        return SUBMIT_SPACE;
    }

    return SUBMIT_IGNORED;
}

uint8_t input_marks_count(void)
{
    return marks_count;
}

uint8_t input_chars_submitted(void)
{
    return chars_submitted;
}

uint8_t input_current_code(void)
{
    return current_code;
}

char input_current_char(void)
{
    if (marks_count == 0)
    {
        return ' ';
    }
    return morse_to_char(current_code);
}

char input_last_submitted_char(void)
{
    return last_submitted_char;
}

static void input_add_mark(uint8_t mark_bit)
{
    // Keep counting long invalid patterns for the seven-segment display.
    if (marks_count < 7)
    {
        current_code = (current_code << 1) | (mark_bit & 0x01);
    }
    else
    {
        current_code = 0;
    }

    marks_count++;
    consecutive_submits = 0;
}

static void history_push(char submitted_char)
{
    if (history_count < HISTORY_LEN)
    {
        history_chars[history_count++] = submitted_char;
        return;
    }

    for (uint8_t i = 1; i < HISTORY_LEN; i++)
    {
        history_chars[i - 1] = history_chars[i];
    }
    history_chars[HISTORY_LEN - 1] = submitted_char;
}
