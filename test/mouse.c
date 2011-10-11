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
#include <foils/hid.h>
#include <foils/hid_device.h>

/* anchor report_definition */
static const uint8_t mouse_report_descriptor[] = {
    0x05, 0x01,                     /*  Usage Page (Desktop),           */
    0x09, 0x02,                     /*  Usage (Mouse),                  */
    0xA1, 0x01,                     /*  Collection (Application),       */
    0x05, 0x09,                     /*      Usage Page (Button),        */
    0x19, 0x01,                     /*      Usage Minimum (01h),        */
    0x29, 0x03,                     /*      Usage Maximum (03h),        */
    0x15, 0x00,                     /*      Logical Minimum (0),        */
    0x25, 0x01,                     /*      Logical Maximum (1),        */
    0x75, 0x01,                     /*      Report Size (1),            */
    0x95, 0x03,                     /*      Report Count (3),           */
    0x81, 0x02,                     /*      Input (Variable),           */
    0x75, 0x05,                     /*      Report Size (5),            */
    0x95, 0x01,                     /*      Report Count (1),           */
    0x81, 0x01,                     /*      Input (Constant),           */
    0x05, 0x01,                     /*      Usage Page (Desktop),       */
    0x09, 0x30,                     /*      Usage (X),                  */
    0x09, 0x31,                     /*      Usage (Y),                  */
    0x09, 0x38,                     /*      Usage (Wheel),              */
    0x15, 0x81,                     /*      Logical Minimum (-127),     */
    0x26, 0x80, 0x00,               /*      Logical Maximum (128),      */
    0x75, 0x08,                     /*      Report Size (8),            */
    0x95, 0x03,                     /*      Report Count (3),           */
    0x81, 0x06,                     /*      Input (Variable, Relative), */
    0xC0,                           /*  End Collection,                 */
};

struct mouse_report
{
    uint8_t buttons;
    int8_t x;
    int8_t y;
    int8_t wheel;
};
/* anchor end */

/* anchor device_definition */
static const
struct foils_hid_device_descriptor descriptors[] =
{
    { "Mouse", 0x0100,
      (void*)mouse_report_descriptor, sizeof(mouse_report_descriptor),
      NULL, 0, NULL, 0
    },
};
/* anchor end */

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

struct mouse_state
{
    struct foils_hid *client;
    float theta;
};

static
void do_mouse_update(
    struct ela_event_source *source, int fd, uint32_t mask, void *data)
{
    struct mouse_state *ms = data;

    /* anchor sending */
    float angle;
    /* anchor end */

    ms->theta += M_PI/16.f;
    angle = ms->theta;

    /* anchor sending */
    struct mouse_report report = {
        .x = cosf(angle) * 32.,
        .y = sinf(angle) * 16.,
    };

    foils_hid_input_report_send(ms->client, 0, 0, 0, &report, sizeof(report));
    /* anchor end */
}

int main(int argc, char **argv)
{
    /* anchor event_loop */
    struct ela_el *el = ela_create(NULL);
    /* anchor end */
    assert(el && "Event loop creation failed");

    if ( argc < 2 ) {
        fprintf(stderr, "Usage: %s ip\n", argv[0]);
        return 1;
    }

    /* anchor client */
    struct foils_hid client;

    int err = foils_hid_init(&client, el, &handler, descriptors, 1);
    if (err) {
        fprintf(stderr, "Error creating client: %s\n", strerror(err));
        return 1;
    }
    /* anchor end */

    struct mouse_state ms = {
        .client = &client,
        .theta = 0.f,
    };

    struct ela_event_source *source;

    ela_source_alloc(el, do_mouse_update, &ms, &source);
    const struct timeval tv = {0, 20000};
    ela_set_timeout(el, source, &tv, 0);
    ela_add(el, source);

    /* anchor client_connect */
    foils_hid_client_connect_hostname(&client, argv[1], 904, 0);
    /* anchor end */

    /* anchor device_enable */
    foils_hid_device_enable(&client, 0);
    /* anchor end */

    /* anchor run */
    ela_run(el);
    /* anchor end */

    ela_remove(el, source);
    ela_source_free(el, source);

    /* anchor cleanup */
    foils_hid_deinit(&client);
    ela_close(el);
    /* anchor end */

    return 0;
}
