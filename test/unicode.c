/*
  Foils_hid, HID device client for Foils

  This file is part of FOILS, the Freebox Open Interface
  Libraries. This file is distributed under a 2-clause BSD license,
  see LICENSE.TXT for details.

  Copyright (c) 2011, Freebox SAS
  See AUTHORS for details
 */

#include <stdio.h>
#include <sys/unistd.h>
#include <sys/fcntl.h>
#include <math.h>
#include <string.h>
#if defined(__APPLE__)
# include <term.h>
#else
# include <termio.h>
#endif
#include <assert.h>
#include <ela/ela.h>
#include <foils/hid.h>
#include <foils/hid_device.h>

static const uint8_t unicode_report_descriptor[] = {
    0x05, 0x01,         /*  Usage Page (Desktop),               */
    0x09, 0x06,         /*  Usage (Keyboard),                   */

    0xA1, 0x01,         /*  Collection (Application),           */
    0x85, 0x01,         /*      Report ID (1),                  */
    0x05, 0x10,         /*      Usage Page (Unicode),           */
    0x08,               /*      Usage (00h),                    */
    0x95, 0x01,         /*      Report Count (1),               */
    0x75, 0x20,         /*      Report Size (32),               */
    0x14,               /*      Logical Minimum (0),            */
    0x25, 0xFF,         /*      Logical Maximum (-1),           */
    0x81, 0x06,         /*      Input (Variable, Relative),     */
    0xC0,               /*  End Collection                      */

    0xA1, 0x01,         /*  Collection (Application),           */
    0x85, 0x02,         /*      Report ID (2),                  */
    0x95, 0x01,         /*      Report Count (1),               */
    0x75, 0x08,         /*      Report Size (8),                */
    0x15, 0x00,         /*      Logical Minimum (0),            */
    0x26, 0xFF, 0x00,   /*      Logical Maximum (255),          */
    0x05, 0x07,         /*      Usage Page (Keyboard),          */
    0x19, 0x00,         /*      Usage Minimum (None),           */
    0x2A, 0xFF, 0x00,   /*      Usage Maximum (FFh),            */
    0x81, 0x00,         /*      Input,                          */
    0xC0,               /*  End Collection                      */

    0x05, 0x0C,         /* Usage Page (Consumer),               */
    0x09, 0x01,         /* Usage (Consumer Control),            */
    0xA1, 0x01,         /* Collection (Application),            */
    0x85, 0x03,         /*  Report ID (3),                      */
    0x95, 0x01,         /*  Report Count (1),                   */
    0x75, 0x10,         /*  Report Size (16),                   */
    0x19, 0x00,         /*  Usage Minimum (Consumer Control),   */
    0x2A, 0x8C, 0x02,   /*  Usage Maximum (AC Send),            */
    0x15, 0x00,         /*  Logical Minimum (0),                */
    0x26, 0x8C, 0x02,   /*  Logical Maximum (652),              */
    0x80,               /*  Input,                              */
    0xC0,               /* End Collection,                      */
};

#define HID_KEYBOARD_A 0x04
#define HID_KEYBOARD_ENTER 0x28
#define HID_KEYBOARD_BACKSPACE 0x2A
#define HID_KEYBOARD_TAB 0x2B
#define HID_KEYBOARD_RIGHTARROW 0x4F
#define HID_KEYBOARD_LEFTARROW 0x50
#define HID_KEYBOARD_DOWNARROW 0x51
#define HID_KEYBOARD_UPARROW 0x52
#define HID_KEYBOARD_HOME 0x4A
#define HID_KEYBOARD_POWER 0x66
#define HID_KEYBOARD_F1 0x3A

#define HID_CONSUMER_SUB_CHANNEL_INCREMENT 0x171
#define HID_CONSUMER_ALTERNATE_AUDIO_INCREMENT 0x173
#define HID_CONSUMER_ALTERNATE_SUBTITLE_INCREMENT 0x175
#define HID_CONSUMER_CHANNEL_INCREMENT 0x9c
#define HID_CONSUMER_CHANNEL_DECREMENT 0x9d
#define HID_CONSUMER_PLAY 0xb0
#define HID_CONSUMER_PAUSE 0xb1
#define HID_CONSUMER_RECORD 0xb2
#define HID_CONSUMER_FAST_FORWARD 0xb3
#define HID_CONSUMER_REWIND 0xb4
#define HID_CONSUMER_SCAN_NEXT_TRACK 0xb5
#define HID_CONSUMER_SCAN_PREVIOUS_TRACK 0xb6
#define HID_CONSUMER_STOP 0xb7
#define HID_CONSUMER_EJECT 0xb8
#define HID_CONSUMER_MUTE 0xe2
#define HID_CONSUMER_VOLUME_INCREMENT 0xe9
#define HID_CONSUMER_VOLUME_DECREMENT 0xea


static const
struct foils_hid_device_descriptor descriptors[] =
{
    { "Unicode", 0x0100,
      (void*)unicode_report_descriptor, sizeof(unicode_report_descriptor),
      NULL, 0, NULL, 0
    },
};

static
void status(
    struct foils_hid *client,
    enum foils_hid_state state)
{
    const char *st = NULL;
    switch (state) {
    case FOILS_HID_IDLE:
        st = "idle";
        break;
    case FOILS_HID_CONNECTING:
        st = "connecting";
        break;
    case FOILS_HID_CONNECTED:
        st = "connected";
        break;
    case FOILS_HID_RESOLVE_FAILED:
        st = "resolve failed";
        break;
    case FOILS_HID_DROPPED:
        st = "dropped";
        break;
    }

    printf("Status: %s\n", st);
}

static
void feature_report(
    struct foils_hid *client,
    uint32_t device_id, uint8_t report_id,
    const void *data, size_t datalen)
{
}

static
void output_report(
    struct foils_hid *client,
    uint32_t device_id, uint8_t report_id,
    const void *data, size_t datalen)
{
}

static
void feature_report_sollicit(
    struct foils_hid *client,
    uint32_t device_id, uint8_t report_id)
{
}

static const struct foils_hid_handler handler =
{
    .status = status,
    .feature_report = feature_report,
    .output_report = output_report,
    .feature_report_sollicit = feature_report_sollicit,
};

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
}

enum state
{
    IDLE, ESCAPE, CONTROL, FUNC,
};

struct unicode_state {
    struct foils_hid *client;
    uint32_t code;
    enum state state;
    int left;
    int control_arg;
};

static void send_unicode(struct unicode_state *ks, uint32_t code)
{
    foils_hid_input_report_send(ks->client, 0, 1, 1, &code, sizeof(code));
    code = 0;
    foils_hid_input_report_send(ks->client, 0, 1, 1, &code, sizeof(code));
}

static void send_kbd(struct unicode_state *ks, int8_t code)
{
    foils_hid_input_report_send(ks->client, 0, 2, 1, &code, sizeof(code));
    code = 0;
    foils_hid_input_report_send(ks->client, 0, 2, 1, &code, sizeof(code));
}

static void send_cons(struct unicode_state *ks, int16_t code)
{
    foils_hid_input_report_send(ks->client, 0, 3, 1, &code, sizeof(code));
    code = 0;
    foils_hid_input_report_send(ks->client, 0, 3, 1, &code, sizeof(code));
}

#define CTRL(x) (x-'a'+1)

static void handle_codepoint(struct unicode_state *ks, uint32_t unicode)
{
    switch (unicode) {
    default:
        return send_unicode(ks, unicode);

    case CTRL('d'):
        ela_exit(ks->client->el);
        break;

    case CTRL('a'):
        printf("Enable\n");
        foils_hid_device_enable(ks->client, 0);
        break;

    case CTRL('e'):
        printf("Disable\n");
        foils_hid_device_disable(ks->client, 0);
        break;

    case 0x7f: // Backspace
        send_kbd(ks, HID_KEYBOARD_BACKSPACE);
        break;

    case 0x0a:
    case 0x0d: // Return
        send_kbd(ks, HID_KEYBOARD_ENTER);
        break;

    case CTRL('i'): // Tab
        send_kbd(ks, HID_KEYBOARD_TAB);
        break;

    case CTRL('t'): // sub
        send_cons(ks, HID_CONSUMER_ALTERNATE_SUBTITLE_INCREMENT);
        break;

    case CTRL('u'): // audio
        send_cons(ks, HID_CONSUMER_ALTERNATE_AUDIO_INCREMENT);
        break;

    case CTRL('v'): // video
        send_cons(ks, HID_CONSUMER_SUB_CHANNEL_INCREMENT);
        break;
    }
}

static void handle_control(struct unicode_state *ks, uint8_t code)
{
    switch (code) {
    case 'A': // UP
        send_kbd(ks, HID_KEYBOARD_UPARROW);
        break;

    case 'B': // Down
        send_kbd(ks, HID_KEYBOARD_DOWNARROW);
        break;

    case 'C': // Right
        send_kbd(ks, HID_KEYBOARD_RIGHTARROW);
        break;

    case 'D': // Left
        send_kbd(ks, HID_KEYBOARD_LEFTARROW);
        break;

    case 'H': // Home
        send_kbd(ks, HID_KEYBOARD_HOME);
        break;

    case '~': // F5-F8
        switch (ks->control_arg) {
        case 5: // Page up: chan+
            send_cons(ks, HID_CONSUMER_CHANNEL_INCREMENT);
            break;
        case 6: // Page down: chan-
            send_cons(ks, HID_CONSUMER_CHANNEL_DECREMENT);
            break;
        case 15: // F5: power
            send_kbd(ks, HID_KEYBOARD_POWER);
            break;
        case 17: // F6: Rew
            send_cons(ks, HID_CONSUMER_REWIND);
            break;
        case 18: // F7: Play
            send_cons(ks, HID_CONSUMER_PLAY);
            break;
        case 19: // F8: FF
            send_cons(ks, HID_CONSUMER_FAST_FORWARD);
            break;

        case 20: // F9: eject
            send_cons(ks, HID_CONSUMER_EJECT);
            break;
        case 21: // F10: vol-
            send_cons(ks, HID_CONSUMER_VOLUME_DECREMENT);
            break;
        case 23: // F11: vol+
            send_cons(ks, HID_CONSUMER_VOLUME_INCREMENT);
            break;
        case 24: // F12: 
//            send_cons(ks, HID_CONSUMER_FAST_FORWARD);
            break;
        }
    }
}

static void handle_func(struct unicode_state *ks, uint8_t code)
{
    switch (code) {
    case 'P'...'S': // F1-F4
        send_kbd(ks, code-'P'+HID_KEYBOARD_F1);
        break;
    }
}

static
void do_unicode_update(
    struct ela_event_source *source, int fd, uint32_t mask, void *data)
{
    struct unicode_state *ks = data;

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

        if (ks->left == 0) {
            switch (ks->state) {
            case IDLE:
                if (ks->code == 0x1b)
                    ks->state = ESCAPE;
                else
                    handle_codepoint(ks, ks->code);
                break;

            case ESCAPE:
                switch (ks->code) {
                case '[':
                    ks->control_arg = 0;
                    ks->state = CONTROL;
                    break;
                case 'O':
                    ks->state = FUNC;
                    break;
                default:
                    ks->state = IDLE;
                    break;
                }
                break;

            case CONTROL:
                switch (ks->code) {
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

            case FUNC:
                handle_func(ks, ks->code);
                ks->state = IDLE;
                break;
            }
        }
    }
}

int main(int argc, char **argv)
{
    struct ela_el *el = ela_create(NULL);
    assert(el && "Event loop creation failed");

    struct foils_hid client;

    if ( argc < 2 ) {
        fprintf(stderr, "Usage: %s ip\n", argv[0]);
        return 1;
    }

    int err = foils_hid_init(&client, el, &handler, descriptors, 1);
    if (err) {
        fprintf(stderr, "Error creating client: %s\n", strerror(err));
        return 1;
    }

    struct unicode_state ks = {
        .client = &client,
        .code = 0,
        .left = 0,
    };

    struct ela_event_source *source;

    tty_break();
    fcntl(0, F_SETFL, O_NONBLOCK);

    ela_source_alloc(el, do_unicode_update, &ks, &source);
    ela_set_fd(el, source, 0, ELA_EVENT_READABLE);
    ela_add(el, source);

    foils_hid_client_connect_hostname(&client, argv[1], 904, 0);

    foils_hid_device_enable(&client, 0);

    ela_run(el);

    ela_remove(el, source);
    ela_source_free(el, source);

    foils_hid_deinit(&client);

    ela_close(el);
    return 0;
}
