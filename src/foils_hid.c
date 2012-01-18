/*
  Foils_hid, HID device client for Foils

  This file is part of FOILS, the Freebox Open Interface
  Libraries. This file is distributed under a 2-clause BSD license,
  see LICENSE.TXT for details.

  Copyright (c) 2011, Freebox SAS
  See AUTHORS for details
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <foils/rudp_hid_client.h>
#include <foils/hid.h>

static const struct rudp_hid_client_handler client_handler;

struct foils_grab_accountant
{
    uint32_t grab[256/32];
};

static
int is_grabbed(const struct foils_grab_accountant *ga, uint8_t report_id)
{
    return (ga->grab[report_id/8] >> (report_id % 8)) & 1;
}

static
void grab_reset(struct foils_grab_accountant *ga)
{
    size_t i;
    for (i=0; i<256/32; ++i)
        ga->grab[i] = 0;
}

static
void grab(struct foils_grab_accountant *ga, uint8_t report_id)
{
    ga->grab[report_id/8] |= 1 << (report_id % 8);
}

static
void release(struct foils_grab_accountant *ga, uint8_t report_id)
{
    ga->grab[report_id/8] &= ~(1 << (report_id % 8));
}



int foils_hid_init(
    struct foils_hid *fh,
    struct ela_el *el,
    const struct foils_hid_handler *handler,
    const struct foils_hid_device_descriptor *descriptor,
    size_t descriptor_count)
{
    memset(fh, 0, sizeof(*fh));

    if (descriptor_count > 32)
        return EINVAL;

    fh->ga = malloc(sizeof(*fh->ga) * descriptor_count);
    if (fh->ga == NULL)
        return ENOMEM;

    memset(fh->ga, 0, sizeof(*fh->ga) * descriptor_count);

    rudp_error_t err = rudp_init(&fh->rudp, el,
                                 RUDP_HANDLER_DEFAULT);
    if ( err )
        goto out;

    err = rudp_hid_client_init(&fh->client, &fh->rudp, &client_handler);
    if ( err )
        goto deinit;

    fh->handler = handler;
    fh->el = el;
    fh->enable = 0;
    fh->state = FOILS_HID_IDLE;
    fh->descriptor = descriptor;
    fh->descriptor_count = descriptor_count;

    return 0;
deinit:
    rudp_deinit(&fh->rudp);
out:
    free(fh->ga);
    return err;
}

void foils_hid_deinit(struct foils_hid *fh)
{
    if (fh->state != FOILS_HID_IDLE)
        rudp_hid_client_close(&fh->client);

    rudp_hid_client_deinit(&fh->client);
    rudp_deinit(&fh->rudp);
    free(fh->ga);
}

void foils_hid_device_enable(struct foils_hid *fh, size_t index)
{
    if (fh->enable & (1<<index))
        return;

    fh->enable |= 1<<index;

    if (!(fh->state == FOILS_HID_CONNECTED))
        return;

    rudp_hid_device_new(&fh->client, &fh->descriptor[index], index);
}

void foils_hid_device_disable(struct foils_hid *fh, size_t index)
{
    if (!(fh->enable & (1<<index)))
        return;

    fh->enable &= ~(1<<index);

    if (!(fh->state == FOILS_HID_CONNECTED))
        return;

    rudp_hid_device_dropped(&fh->client, index);
}


void foils_hid_input_report_send(
    struct foils_hid *fh,
    size_t device_index, uint8_t report_id,
    int reliable,
    const void *data, size_t datalen)
{
    if (!(fh->state == FOILS_HID_CONNECTED))
        return;
    if (!is_grabbed(fh->ga + device_index, report_id))
        return;

    rudp_hid_input_report_send(
        &fh->client, device_index, report_id,
        reliable, data, datalen);
}

void foils_hid_feature_report_send(
    struct foils_hid *fh,
    size_t device_index, uint8_t report_id,
    int reliable,
    const void *data, size_t datalen)
{
    if (!(fh->state == FOILS_HID_CONNECTED))
        return;
    if (!is_grabbed(fh->ga + device_index, report_id))
        return;

    rudp_hid_feature_report_send(
        &fh->client, device_index, report_id,
        reliable, data, datalen);
}

int foils_hid_client_connect_hostname(
    struct foils_hid *fh,
    const char *hostname,
    const uint16_t port,
    uint32_t ip_flags)
{
    if (fh->state == FOILS_HID_CONNECTED)
        return EINVAL;

    rudp_hid_client_set_hostname(&fh->client, hostname, port, ip_flags);

    fh->state = FOILS_HID_CONNECTING;
    if (rudp_hid_client_connect(&fh->client)) {
        fh->state = FOILS_HID_IDLE;
        fh->handler->status(fh, FOILS_HID_RESOLVE_FAILED);
    }
    return 0;
}


void foils_hid_client_connect_ipv4(
    struct foils_hid *fh,
    const struct in_addr *address,
    const uint16_t port)
{
    if (fh->state == FOILS_HID_CONNECTED)
        return;

    rudp_hid_client_set_ipv4(&fh->client, address, port);

    fh->state = FOILS_HID_CONNECTING;
    if (rudp_hid_client_connect(&fh->client)) {
        fh->state = FOILS_HID_IDLE;
        fh->handler->status(fh, FOILS_HID_RESOLVE_FAILED);
    }
}


void foils_hid_client_connect_ipv6(
    struct foils_hid *fh,
    const struct in6_addr *address,
    const uint16_t port)
{
    if (fh->state == FOILS_HID_CONNECTED)
        return;

    rudp_hid_client_set_ipv6(&fh->client, address, port);

    fh->state = FOILS_HID_CONNECTING;
    if (rudp_hid_client_connect(&fh->client)) {
        fh->state = FOILS_HID_IDLE;
        fh->handler->status(fh, FOILS_HID_RESOLVE_FAILED);
    }
}

static
void connected(
        struct rudp_hid_client *_client)
{
    struct foils_hid *fh = (struct foils_hid *)_client;

    fh->state = FOILS_HID_CONNECTED;

    fh->handler->status(fh, FOILS_HID_CONNECTED);

    size_t i;
    for (i=0; i<fh->descriptor_count; ++i) {
        if (!(fh->enable & (1<<i)))
            continue;

        rudp_hid_device_new(&fh->client, &fh->descriptor[i], i);
    }
}

static
void server_lost(
        struct rudp_hid_client *_client)
{
    struct foils_hid *fh = (struct foils_hid *)_client;
    size_t i;

    fh->handler->status(fh, FOILS_HID_CONNECTING);
    fh->state = FOILS_HID_CONNECTING;

    for (i=0; i<fh->descriptor_count; ++i)
        grab_reset(fh->ga + i);

    rudp_hid_client_connect(&fh->client);
}

static
void device_grab(
        struct rudp_hid_client *_client,
        uint32_t device_id, uint8_t report_id)
{
    struct foils_hid *fh = (struct foils_hid *)_client;

    grab(fh->ga + device_id, report_id);
}

static
void device_release(
        struct rudp_hid_client *_client,
        uint32_t device_id, uint8_t report_id)
{
    struct foils_hid *fh = (struct foils_hid *)_client;

    release(fh->ga + device_id, report_id);
}

static
void device_open(
        struct rudp_hid_client *_client,
        uint32_t device_id)
{
    struct foils_hid *fh = (struct foils_hid *)_client;
    (void)fh;
}

static
void device_close(
        struct rudp_hid_client *_client,
        uint32_t device_id)
{
    struct foils_hid *fh = (struct foils_hid *)_client;

    grab_reset(fh->ga + device_id);
}

static
void feature_report(
        struct rudp_hid_client *_client,
        uint32_t device_id, uint8_t report_id,
        const void *data, size_t datalen)
{
    struct foils_hid *fh = (struct foils_hid *)_client;

    fh->handler->feature_report(fh, device_id, report_id,
                                data, datalen);
}

static
void output_report(
        struct rudp_hid_client *_client,
        uint32_t device_id, uint8_t report_id,
        const void *data, size_t datalen)
{
    struct foils_hid *fh = (struct foils_hid *)_client;

    fh->handler->output_report(fh, device_id, report_id,
                               data, datalen);
}

static
void feature_report_sollicit(
        struct rudp_hid_client *_client,
        uint32_t device_id, uint8_t report_id)
{
    struct foils_hid *fh = (struct foils_hid *)_client;

    fh->handler->feature_report_sollicit(fh, device_id, report_id);
}

static const
struct rudp_hid_client_handler client_handler =
{
    .connected = connected,
    .server_lost = server_lost,
    .device_grab = device_grab,
    .device_release = device_release,
    .device_open = device_open,
    .device_close = device_close,
    .feature_report = feature_report,
    .output_report = output_report,
    .feature_report_sollicit = feature_report_sollicit,
};
