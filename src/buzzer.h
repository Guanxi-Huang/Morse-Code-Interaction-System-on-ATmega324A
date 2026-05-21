#ifndef BUZZER_H_
#define BUZZER_H_

#include "io_led.h"

void buzzer_init(void);
void buzzer_silence(void);
void buzzer_on_beat(const io_led_beat_t *beat);

#endif /* BUZZER_H_ */
