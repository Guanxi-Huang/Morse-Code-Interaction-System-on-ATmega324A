/*
 * terminalio.c
 *
 * Author: Peter Sutton
 */

#include <stdio.h>
#include <stdint.h>

#include "terminalio.h"


void move_terminal_cursor(int x, int y)
{
    printf("\x1b[%d;%dH", y, x);
}

void clear_terminal(void)
{
    printf("\x1b[2J");
}

void set_terminal_display(DisplayParameter display_parameter)
{
    // Select one ANSI display attribute for the next terminal output.
    printf("\x1b[%dm", display_parameter);
}

void reset_terminal_display(void)
{
    // Reset ANSI attributes so splash and later text do not inherit colours.
    printf("\x1b[0m");
}
