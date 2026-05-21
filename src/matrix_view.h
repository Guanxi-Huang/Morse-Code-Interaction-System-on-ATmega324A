#ifndef MATRIX_VIEW_H_
#define MATRIX_VIEW_H_

#include <stdint.h>

void matrix_view_init(void);
void matrix_view_on_mark(char preview_char);
void matrix_view_on_submit(char submitted_char);
void matrix_view_on_submit_colour(char submitted_char, uint8_t colour);
void matrix_view_clear_in_progress(void);
void matrix_view_tick(void);
void matrix_view_set_large_font(uint8_t enabled);
void matrix_view_scroll(int8_t direction);
void matrix_view_adjust_brightness(int8_t direction);

#endif /* MATRIX_VIEW_H_ */
