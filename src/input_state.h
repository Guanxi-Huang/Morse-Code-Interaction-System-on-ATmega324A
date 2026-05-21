#ifndef INPUT_STATE_H_
#define INPUT_STATE_H_

#include <stdint.h>

#define HISTORY_LEN 4

typedef enum
{
    SUBMIT_IGNORED = 0,
    SUBMIT_CHARACTER,
    SUBMIT_SPACE
} submit_result_t;

void input_state_init(void);
void input_add_dot(void);
void input_add_dash(void);
void input_clear_in_progress(void);
void input_record_external_char(char submitted_char);
submit_result_t input_submit(void);
uint8_t input_marks_count(void);
uint8_t input_chars_submitted(void);
uint8_t input_current_code(void);
char input_current_char(void);
char input_last_submitted_char(void);

#endif /* INPUT_STATE_H_ */
