#ifndef SYNC_MODE_H_
#define SYNC_MODE_H_

typedef enum
{
    SYNC_EVENT_NONE = 0,
    SYNC_EVENT_DOT,
    SYNC_EVENT_DASH,
    SYNC_EVENT_SUBMIT
} sync_event_t;

void sync_mode_init(void);
void sync_mode_reset(void);
sync_event_t sync_mode_poll(void);

#endif /* SYNC_MODE_H_ */
