/*
  Foils_hid, HID device client for Foils

  This file is part of FOILS, the Freebox Open Interface
  Libraries. This file is distributed under a 2-clause BSD license,
  see LICENSE.TXT for details.

  Copyright (c) 2011, Freebox SAS
  See AUTHORS for details
 */

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <assert.h>
#include <ela/ela.h>
#include <sys/unistd.h>

#include <foils/hid.h>
#include <foils/hid_device.h>

#include "term_input.h"
#include "mapping.h"

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
    0x27, 0xFF, 0xFF, 0xFF,  /*      Logical Maximum (2**24-1), */
    0x81, 0x62,         /*      Input (Variable, No pref state, No Null Pos),  */
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
    0x80,               /*      Input,                          */
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

    0x05, 0x01,         /* Usage Page (Desktop),                */
    0x0a, 0x80, 0x00,   /* Usage (System Control),              */
    0xA1, 0x01,         /* Collection (Application),            */
    0x85, 0x04,         /*  Report ID (4),                      */
    0x75, 0x01,         /*  Report Size (1),                    */
    0x95, 0x04,         /*  Report Count (4),                   */
    0x1a, 0x81, 0x00,   /*  Usage Minimum (System Power Down),  */
    0x2a, 0x84, 0x00,   /*  Usage Maximum (System Context menu),*/
    0x81, 0x02,         /*  Input (Variable),                   */
    0x75, 0x01,         /*  Report Size (1),                    */
    0x95, 0x04,         /*  Report Count (4),                   */
    0x81, 0x01,         /*  Input (Constant),                   */
    0xC0,               /* End Collection,                      */
};

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

struct unicode_state;

typedef void code_sender_t(struct unicode_state *ks, uint32_t code);

struct unicode_state {
    struct term_input_state input_state;
    struct ela_el *el;
    struct foils_hid *client;
    struct ela_event_source *release;
    code_sender_t *release_sender;
    uint32_t current_code;
    uint32_t release_code;
};

static void release_send(struct unicode_state *ks)
{
    if (ks->release_sender) {
        ks->release_sender(ks, ks->release_code);
        ks->release_sender = NULL;
    }
}

static
void release_cb(
    struct ela_event_source *source, int fd,
    uint32_t mask, void *data)
{
    struct unicode_state *ks = data;
    release_send(ks);
}

static void release_post(
    struct unicode_state *ks,
    code_sender_t *release_sender,
    uint32_t release_code)
{
    if (ks->release_sender) {
        release_send(ks);
        ela_remove(ks->el, ks->release);
    }

    ks->release_sender = release_sender;
    ks->release_code = release_code;

    struct timeval tv = {0, 100000};
    ela_set_timeout(ks->el, ks->release, &tv, ELA_EVENT_ONCE);
    ela_add(ks->el, ks->release);
}

static void send_unicode(struct unicode_state *ks, uint32_t code)
{
    foils_hid_input_report_send(ks->client, 0, 1, 1, &code, sizeof(code));
}

static void send_kbd(struct unicode_state *ks, uint32_t _code)
{
    uint8_t code = _code;
    foils_hid_input_report_send(ks->client, 0, 2, 1, &code, sizeof(code));
}

static void send_cons(struct unicode_state *ks, uint32_t _code)
{
    uint16_t code = _code;
    foils_hid_input_report_send(ks->client, 0, 3, 1, &code, sizeof(code));
}

static void send_sysctl(struct unicode_state *ks, uint32_t _code)
{
    uint8_t code = _code;
    foils_hid_input_report_send(ks->client, 0, 4, 1, &code, sizeof(code));
}

static
void input_handler(
    struct term_input_state *input,
    uint8_t is_unicode,
    uint32_t code)
{
    struct unicode_state *ks = (void*)input;

    if (is_unicode)
        return send_unicode(ks, code);

    mapping_dump_target(code);

    const struct target_code *target = mapping_get(code);
    code_sender_t *sender;
    uint8_t repeatable = 0;

    switch (target->report) {
    case TARGET_UNICODE:
        sender = send_unicode;
        break;
    case TARGET_KEYBOARD:
        sender = send_kbd;
        repeatable = 1;
        break;
    case TARGET_CONSUMER:
        sender = send_cons;
        break;
    case TARGET_DESKTOP:
        sender = send_sysctl;
        break;
    default:
        return;
    }

    if (repeatable
        && target->usage == ks->current_code
        && sender == ks->release_sender) {
        ela_remove(ks->el, ks->release);
        ela_add(ks->el, ks->release);
    } else {
        sender(ks, target->usage);
        release_post(ks, sender, 0);
        ks->current_code = target->usage;
    }
}

int main(int argc, char **argv)
{
    struct ela_el *el = ela_create(NULL);
    assert(el && "Event loop creation failed");

    struct foils_hid client;

    if ( argc < 3 ) {
        fprintf(stderr, "Usage: %s ip port\n", argv[0]);
        return 1;
    }

    int err = foils_hid_init(&client, el, &handler, descriptors, 1);
    if (err) {
        fprintf(stderr, "Error creating client: %s\n", strerror(err));
        return 1;
    }

    struct unicode_state ks = {
        .client = &client,
        .el = el,
    };

    mapping_dump();

    term_input_init(&ks.input_state, input_handler, el);
    foils_hid_client_connect_hostname(&client, argv[1], atoi(argv[2]), 0);
    foils_hid_device_enable(&client, 0);

    ela_source_alloc(el, release_cb, &ks, &ks.release);

    ela_run(el);

    term_input_deinit(&ks.input_state);
    foils_hid_deinit(&client);

    ela_close(el);
    return 0;
}
