#ifndef JOYSTICK_H_
#define JOYSTICK_H_

#include <stdint.h>

void joystick_init(void);
uint16_t joystick_read_x(void);
uint16_t joystick_read_y(void);
int8_t joystick_axis_direction(uint16_t value, int8_t previous_direction);
uint16_t joystick_scroll_delay_ms(uint16_t x_value);

#endif /* JOYSTICK_H_ */
