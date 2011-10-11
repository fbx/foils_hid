/*
  Foils_hid, HID device client for Foils

  This file is part of FOILS, the Freebox Open Interface
  Libraries. This file is distributed under a 2-clause BSD license,
  see LICENSE.TXT for details.

  Copyright (c) 2011, Freebox SAS
  See AUTHORS for details
 */

#include <string.h>
#include <foils/rudp_hid_client.h>

enum foils_hid_command
{
    FOILS_HID_DEVICE_NEW = 0, // client to server
    FOILS_HID_DEVICE_DROPPED = 1, // client to server

    FOILS_HID_DEVICE_CREATED = 2, // server to client
    FOILS_HID_DEVICE_CLOSE = 3, // server to client

    FOILS_HID_FEATURE = 4, // bidir
    FOILS_HID_DATA = 5, // bidir
    FOILS_HID_GRAB = 6, // server to client
    FOILS_HID_RELEASE = 7, // server to client

    FOILS_HID_FEATURE_SOLLICIT = 8, // server to client
};

/**
   When unused, fields are 0.

   All fields are big-endian on the wire.
 */
struct foils_hid_header
{
    uint32_t device_id;
    uint32_t report_id;
};

#define DEVICE_NAME_LEN 64
#define DEVICE_SERIAL_LEN 32

/**
   All fields are big-endian on the wire.
 */
struct foils_hid_device_new
{
    char name[DEVICE_NAME_LEN];
    char serial[DEVICE_SERIAL_LEN];
    uint16_t zero; // keep this to 0
    uint16_t version; // 1.00 is 0x0100
    uint16_t descriptor_offset; // from start of struct foils_hid_device_new
    uint16_t descriptor_size; // bytes
    uint16_t physical_offset; // from start of struct foils_hid_device_new
    uint16_t physical_size; // bytes
    uint16_t strings_offset; // from start of struct foils_hid_device_new
    uint16_t strings_size; // bytes
};


static const struct rudp_client_handler _handler;

int rudp_hid_client_init(
    struct rudp_hid_client *client,
    struct rudp *rudp,
    const struct rudp_hid_client_handler *handler)
{
    client->handler = handler;
    return rudp_client_init(&client->base, rudp, &_handler);
}

void rudp_hid_client_deinit(
    struct rudp_hid_client *client)
{
    rudp_client_deinit(&client->base);
}

static inline size_t round_up(size_t x)
{
    return (x + 3) & ~3;
}

int rudp_hid_device_new(
    struct rudp_hid_client *client,
    const struct foils_hid_device_descriptor *desc,
    uint32_t device_id)
{
    size_t blob_size = desc->descriptor_size
        + desc->physical_size
        + desc->strings_size + 8;

    struct packet {
        struct foils_hid_header header[1];
        struct foils_hid_device_new dev[1];
        uint8_t data[blob_size];
    } packet[1];

    memset(packet, 0, sizeof(*packet));

    packet->header->device_id = htonl(device_id);

    strncpy(packet->dev->name, desc->name, DEVICE_NAME_LEN);
//    strncpy(packet->dev->serial, desc->serial, DEVICE_SERIAL_LEN);
    packet->dev->version = htons(desc->version);

    size_t descriptor_size = round_up(desc->descriptor_size);
    size_t physical_size = round_up(desc->physical_size);

    packet->dev->descriptor_size = htons(desc->descriptor_size);
    packet->dev->physical_size = htons(desc->physical_size);
    packet->dev->strings_size = htons(desc->strings_size);

    packet->dev->descriptor_offset = htons(
        sizeof(struct foils_hid_device_new));
    packet->dev->physical_offset = htons(
        sizeof(struct foils_hid_device_new)
        + descriptor_size);
    packet->dev->strings_offset = htons(
        sizeof(struct foils_hid_device_new)
        + descriptor_size
        + physical_size);

    memcpy(packet->data, desc->descriptor, desc->descriptor_size);
    memcpy(packet->data + descriptor_size,
           desc->physical, desc->physical_size);
    memcpy(packet->data + descriptor_size + physical_size,
           desc->strings, desc->strings_size);

    return rudp_client_send(
        &client->base, 1, FOILS_HID_DEVICE_NEW,
        packet, sizeof(struct foils_hid_header)
        + sizeof(struct foils_hid_device_new)
        + blob_size);
}


int rudp_hid_device_dropped(
    struct rudp_hid_client *client,
    uint32_t device_id)
{
    struct foils_hid_header packet[1];

    memset(packet, 0, sizeof(packet));

    packet->device_id = htonl(device_id);

    return rudp_client_send(
        &client->base, 1, FOILS_HID_DEVICE_DROPPED,
        packet, sizeof(*packet));
}


static
void do_handle_packet(
    struct rudp_client *_client,
    int command, const void *data, size_t len)
{
    struct rudp_hid_client *client = (struct rudp_hid_client *)_client;
    const struct foils_hid_header *header = data;

    if (len < sizeof(*header))
        return;

    switch (command) {
    case FOILS_HID_DEVICE_NEW:
    case FOILS_HID_DEVICE_DROPPED:
    default:
        return;

    case FOILS_HID_DEVICE_CREATED:
        client->handler->device_open(
            client, ntohl(header->device_id));
        break;

    case FOILS_HID_DEVICE_CLOSE:
        client->handler->device_close(
            client, ntohl(header->device_id));
        break;

    case FOILS_HID_FEATURE_SOLLICIT:
        client->handler->feature_report_sollicit(
            client, ntohl(header->device_id),
            ntohl(header->report_id));
        break;

    case FOILS_HID_FEATURE:
        client->handler->feature_report(
            client, ntohl(header->device_id),
            ntohl(header->report_id),
            (const void*)(header+1), len - sizeof(*header));
        break;

    case FOILS_HID_DATA:
        client->handler->output_report(
            client, ntohl(header->device_id),
            ntohl(header->report_id),
            (const void*)(header+1), len - sizeof(*header));
        break;

    case FOILS_HID_RELEASE:
        client->handler->device_release(
            client, ntohl(header->device_id),
            ntohl(header->report_id));
        break;

    case FOILS_HID_GRAB:
        client->handler->device_grab(
            client, ntohl(header->device_id),
            ntohl(header->report_id));
        break;
    }
}

static
void do_link_info(struct rudp_client *_client, struct rudp_link_info *info)
{
    struct rudp_hid_client *client = (struct rudp_hid_client *)_client;
    (void)client;
}

static
void do_connected(struct rudp_client *_client)
{
    struct rudp_hid_client *client = (struct rudp_hid_client *)_client;

    client->handler->connected(client);
}

static
void do_server_lost(struct rudp_client *_client)
{
    struct rudp_hid_client *client = (struct rudp_hid_client *)_client;

    client->handler->server_lost(client);
}


int rudp_hid_feature_report_send(
    struct rudp_hid_client *client,
    uint32_t device_id,
    uint8_t report_id,
    int reliable,
    const void *data, size_t datalen)
{
    uint8_t blob[datalen + 8];
    struct foils_hid_header *header = (struct foils_hid_header *)blob;

    header->device_id = htonl(device_id);
    header->report_id = htonl(report_id);
    memcpy(header+1, data, datalen);
    return rudp_client_send(&client->base, reliable,
                            FOILS_HID_FEATURE, header,
                            datalen + 8);
}


int rudp_hid_input_report_send(
    struct rudp_hid_client *client,
    uint32_t device_id,
    uint8_t report_id,
    int reliable,
    const void *data, size_t datalen)
{
    uint8_t blob[datalen + 8];
    struct foils_hid_header *header = (struct foils_hid_header *)blob;

    header->device_id = htonl(device_id);
    header->report_id = htonl(report_id);
    memcpy(header+1, data, datalen);
    return rudp_client_send(&client->base, reliable,
                            FOILS_HID_DATA, header,
                            datalen + 8);
}


static const struct rudp_client_handler _handler =
{
    .handle_packet = do_handle_packet,
    .link_info = do_link_info,
    .connected = do_connected,
    .server_lost = do_server_lost,
};
