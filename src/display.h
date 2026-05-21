/*
 * display.h
 *
 * Author: Ryan Wang
 */ 

#ifndef DISPLAY_H_
#define DISPLAY_H_

#include <stdint.h>

/*
 * display a start screen
 */
void start_splash_display(void);

/*
 * draws a small char glyph on the LED matrix starting at x_position (columnn number)
 */
void draw_small_char(char character, uint8_t x_position, uint8_t colour);
void draw_char_with_font(char character, uint8_t x_position, uint8_t colour,
        uint8_t large_font);
uint8_t get_char_glyph_column(char character, uint8_t col, uint8_t large_font);
uint8_t get_char_glyph_width(uint8_t large_font);

#endif 
