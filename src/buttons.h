#ifndef BUTTONS_H_
#define BUTTONS_H_

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

#endif /* BUTTONS_H_ */
