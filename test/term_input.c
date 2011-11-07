/*
  Foils_hid, HID device client for Foils

  This file is part of FOILS, the Freebox Open Interface
  Libraries. This file is distributed under a 2-clause BSD license,
  see LICENSE.TXT for details.

  Copyright (c) 2011, Freebox SAS
  See AUTHORS for details
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/fcntl.h>
#if defined(__APPLE__)
# include <term.h>
#else
# include <termio.h>
#endif

#include "term_input.h"

static
int tty_break(void)
{
#if defined(__APPLE__)
    struct termios ttystate;

    tcgetattr(fileno(stdin), &ttystate);
    ttystate.c_lflag &= ~(ICANON|ECHO);
    ttystate.c_cc[VMIN] = 1;
    tcsetattr(fileno(stdin), TCSANOW, &ttystate);
#else
    struct termio modmodes;
    if(ioctl(fileno(stdin), TCGETA, &modmodes) < 0)
        return -1;
    modmodes.c_lflag &= ~(ICANON|ECHO);
    modmodes.c_cc[VMIN] = 1;
    modmodes.c_cc[VTIME] = 0;
    return ioctl(fileno(stdin), TCSETAW, &modmodes);
#endif
    return 0;
}

#define KEY_CTRL(x) (x-'a'+1)
#define SHIFT(x) (ks->control_arg2 == 2 ? x : 0)

static void handle_control(struct term_input_state *ks, uint8_t code)
{
    switch (code) {
    case '~':
        switch (ks->control_arg) {
        case 5: // Page up
            return ks->handler(ks, 0, TERM_INPUT_PAGE_UP);
        case 6: // Page down
            return ks->handler(ks, 0, TERM_INPUT_PAGE_DOWN);
        case 15: // F5
            return ks->handler(ks, 0, TERM_INPUT_F(5));
        case (17) ... (21): // F6-F10
            return ks->handler(ks, 0, TERM_INPUT_F(
                                   ks->control_arg - 17 + 6
                                   + SHIFT(12)));
        case (23) ... (24): // F11-F12
            return ks->handler(ks, 0, TERM_INPUT_F(
                                   ks->control_arg - 23 + 11
                                   + SHIFT(12)));
        }
    case 'A': // UP
        return ks->handler(ks, 0, TERM_INPUT_UP);
    case 'B': // Down
        return ks->handler(ks, 0, TERM_INPUT_DOWN);
    case 'C': // Right
        return ks->handler(ks, 0, TERM_INPUT_RIGHT);
    case 'D': // Left
        return ks->handler(ks, 0, TERM_INPUT_LEFT);
    case 'H': // Home
        return ks->handler(ks, 0, TERM_INPUT_HOME);
    case 'F': // End
        return ks->handler(ks, 0, TERM_INPUT_END);
    case 'P'...'S':
        switch (ks->control_arg) {
        case 'O':
            // F1-F4
            return ks->handler(ks, 0, TERM_INPUT_F(code - 'P' + 1));
        case 1:
            if (ks->control_arg2 == 2)
                // F13-F16
                return ks->handler(ks, 0, TERM_INPUT_F(code - 'P' + 13));
        }
        break;
    }

    if (ks->control_arg2)
        printf("Unknown control '%d;%d%c'\n", ks->control_arg, ks->control_arg2, code);
    else
        printf("Unknown control '%d%c'\n", ks->control_arg, code);
}

static
void term_input_update(
    struct ela_event_source *source,
    int fd, uint32_t mask, void *data)
{
    struct term_input_state *ks = data;

    int c;

    for (c = getchar(); c >= 0; c = getchar()) {
        if (ks->left == 0) {
            if (c & 0x80) {
                for (ks->left=1; ks->left<=5; ks->left++)
                    if (!(c & (1<<(6 - ks->left))))
                        break;
                ks->code = c & ((1<<(6 - ks->left)) - 1);
            } else {
                ks->code = c;
            }
        } else {
            ks->code <<= 6;
            ks->code |= c & 0x3f;
            ks->left--;
        }

        if (ks->left)
            continue;

        /* printf("Handling... code: %02x, state: %02x, ca: %02x, ca2: %02x\n", */
        /*        ks->code, ks->state, ks->control_arg, ks->control_arg2); */

        switch (ks->state) {
        case IDLE:
            switch (ks->code) {
            default:
                ks->handler(ks, 1, ks->code);
                break;

            case 0x1b: // esc
                ks->state = ESCAPE;
                ks->control_arg = 0;
                ks->control_arg2 = 0;
                break;

            case 0x7f: // Backspace
                ks->handler(ks, 0, TERM_INPUT_BACKSPACE);
                break;

            case KEY_CTRL('a')...KEY_CTRL('z'):
                ks->handler(
                    ks, 0,
                    TERM_INPUT_CTRL(ks->code - KEY_CTRL('a') + 'a'));
                break;
            }
            break;

        case ESCAPE:
            switch (ks->code) {
            case 0x1b: // esc
                ks->state = ESCAPE;
                ks->control_arg = 0;
                ks->control_arg2 = 0;
                break;
            case '[':
                ks->state = CONTROL;
                break;
            case 'O':
                ks->control_arg = 'O';
                ks->state = CONTROL;
                break;
            default:
                ks->state = IDLE;
                break;
            }
            break;

        case CONTROL:
            switch (ks->code) {
            case ';':
                ks->state = CONTROL2;
                break;
            case '0'...'9':
                ks->control_arg *= 10;
                ks->control_arg += ks->code - '0';
                break;
            default:
                handle_control(ks, ks->code);
                ks->state = IDLE;
                break;
            }
            break;

        case CONTROL2:
            switch (ks->code) {
            case '0'...'9':
                ks->control_arg2 *= 10;
                ks->control_arg2 += ks->code - '0';
                break;
            default:
                handle_control(ks, ks->code);
                ks->state = IDLE;
                break;
            }
            break;
        }
        ks->code = 0;
    }
}

int term_input_init(
    struct term_input_state *state,
    input_handler_func_t *handler,
    struct ela_el *el)
{
    memset(state, 0, sizeof(*state));

    state->el = el;
    state->handler = handler;

    tty_break();
    fcntl(0, F_SETFL, O_NONBLOCK);

    ela_source_alloc(el, term_input_update, state, &state->source);
    ela_set_fd(el, state->source, 0, ELA_EVENT_READABLE);
    ela_add(el, state->source);

    return 0;
}

void term_input_deinit(
    struct term_input_state *state)
{
    ela_remove(state->el, state->source);
    ela_source_free(state->el, state->source);
}
