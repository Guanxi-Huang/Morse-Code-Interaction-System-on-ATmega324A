#ifndef TIMER_H_
#define TIMER_H_

#include <stdint.h>

void timer_init(void);
uint8_t timer_tick_consume(void);
uint32_t timer_millis(void);

#endif /* TIMER_H_ */
