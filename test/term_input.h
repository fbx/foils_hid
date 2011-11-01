#ifndef TERM_INPUT_H_
#define TERM_INPUT_H_

#include <ela/ela.h>

enum term_input_key
{
    TERM_INPUT_NONE,
    TERM_INPUT_CTRL_KEYS = 1,
#define TERM_INPUT_CTRL(x) (TERM_INPUT_CTRL_KEYS + (x) - 'a')
    TERM_INPUT_FUNCTION_KEYS = TERM_INPUT_CTRL('z'+1),
#define TERM_INPUT_F(x) (TERM_INPUT_FUNCTION_KEYS - 1 + (x))
    TERM_INPUT_HOME = TERM_INPUT_F(25),
    TERM_INPUT_END,
    TERM_INPUT_PAGE_UP,
    TERM_INPUT_PAGE_DOWN,
    TERM_INPUT_UP,
    TERM_INPUT_DOWN,
    TERM_INPUT_LEFT,
    TERM_INPUT_RIGHT,
    TERM_INPUT_BACKSPACE,
    TERM_INPUT_COUNT,
};

struct term_input_state;

enum state
{
    IDLE, ESCAPE, CONTROL, CONTROL2,
};

typedef void input_handler_func_t(struct term_input_state *input, uint8_t is_unicode, uint32_t code);

struct term_input_state
{
    struct ela_el *el;
    struct ela_event_source *source;
    input_handler_func_t *handler;
    uint32_t code;
    enum state state;
    int left;
    int control_arg;
    int control_arg2;
};

int term_input_init(
    struct term_input_state *state,
    input_handler_func_t *handler,
    struct ela_el *el);

void term_input_deinit(
    struct term_input_state *state);

#endif
