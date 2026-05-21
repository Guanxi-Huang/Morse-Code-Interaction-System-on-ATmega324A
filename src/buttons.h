#ifndef BUTTONS_H_
#define BUTTONS_H_

#include <stdint.h>

typedef enum
{
    BTN_NONE = 0,
    BTN_DOT,
    BTN_DASH,
    BTN_SUBMIT
} button_event_t;

void buttons_init(void);
void buttons_sync(void);
button_event_t buttons_poll(void);
uint8_t buttons_b0_held(void);

#endif /* BUTTONS_H_ */
