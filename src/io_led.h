#ifndef IO_LED_H_
#define IO_LED_H_

void io_led_init(void);
void io_led_on_dot(void);
void io_led_on_dash(void);
void io_led_on_submit_character(void);
void io_led_on_submit_word(void);
void io_led_tick(void);

#endif /* IO_LED_H_ */
