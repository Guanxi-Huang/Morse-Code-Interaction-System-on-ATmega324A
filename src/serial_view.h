#ifndef SERIAL_VIEW_H_
#define SERIAL_VIEW_H_

#include <stdint.h>

void serial_view_init(void);
void serial_view_on_mark(char preview_char);
void serial_view_on_submit(char submitted_char);
void serial_view_on_submit_colour(char submitted_char, uint8_t colour);
void serial_view_clear_in_progress(void);

#endif /* SERIAL_VIEW_H_ */
