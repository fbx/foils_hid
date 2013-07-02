/*
  Foils_hid, HID device client for Foils

  This file is part of FOILS, the Freebox Open Interface
  Libraries. This file is distributed under a 2-clause BSD license,
  see LICENSE.TXT for details.

  Copyright (c) 2011, Freebox SAS
  See AUTHORS for details
 */

#ifndef RUDP_HID_CLIENT_H_
#define RUDP_HID_CLIENT_H_

/**
   @file
   @module {HID rudp client}
   @short Human interface device low-level client

   This defines a wrapped Librudp client handling all the low-level
   protocol of HID devices transport over Librudp.
*/

#include <foils/hid_device.h>
#include <rudp/client.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

struct rudp_hid_client;

/**
   @this defines the possible client callbacks.
 */
struct rudp_hid_client_handler
{
    /**
       @this is called when the client connects to the server

       @param client The client context
     */
    void (*connected)(
        struct rudp_hid_client *client);

    /**
       @this is called when the client gets dropped from the server

       @param client The client context
     */
    void (*server_lost)(
        struct rudp_hid_client *client);

    /**
       @this is called when the server asks for device grab

       @param client The client context
       @param device_id Grabbed device id
       @param report_id The report id in the device
     */
    void (*device_grab)(
        struct rudp_hid_client *client,
        uint32_t device_id, uint8_t report_id);

    /**
       @this is called when the server asks for device release

       @param client The client context
       @param device_id Released device id
       @param report_id The report id in the device
     */
    void (*device_release)(
        struct rudp_hid_client *client,
        uint32_t device_id, uint8_t report_id);

    /**
       @this is called when the server asks for device open

       @param client The client context
       @param device_id Opened device id
     */
    void (*device_open)(
        struct rudp_hid_client *client,
        uint32_t device_id);

    /**
       @this is called when the server asks for device closure

       @param client The client context
       @param device_id Closed device id
     */
    void (*device_close)(
        struct rudp_hid_client *client,
        uint32_t device_id);

    /**
       @this is called when the server sends a feature report to the
             client

       @param client The client context
       @param device_id Closed device id
       @param report_id The report id in the device
       @param data Report blob
       @param datalen Received blob size
     */
    void (*feature_report)(
        struct rudp_hid_client *client,
        uint32_t device_id, uint8_t report_id,
        const void *data, size_t datalen);

    /**
       @this is called when the server sends an output report to the
             client

       @param client The client context
       @param device_id Closed device id
       @param report_id The report id in the device
       @param data Report blob
       @param datalen Received blob size
     */
    void (*output_report)(
        struct rudp_hid_client *client,
        uint32_t device_id, uint8_t report_id,
        const void *data, size_t datalen);

    /**
       @this is called when the server needs the device to send a
       feature report

       @param client The client context
       @param device_id The device index in the declared array
       @param report_id The needed report id in the device
     */
    void (*feature_report_sollicit)(
        struct rudp_hid_client *client,
        uint32_t device_id, uint8_t report_id);
};

/**
   Wrapped rudp HID client state

   @hidecontent
 */
struct rudp_hid_client
{
    struct rudp_client base;
    const struct rudp_hid_client_handler *handler;
};

/**
   @mgroup {Client context management}

   @this initializes a client structure in the given librudp context

   @param client Client structure to initialize
   @param rudp Valid rudp context
   @param handler Callback functions

   @returns 0 when done, or an error taken from errno(7)
 */
int rudp_hid_client_init(
    struct rudp_hid_client *client,
    struct rudp *rudp,
    const struct rudp_hid_client_handler *handler);

/**
   @mgroup {Connection management}

   @this sets the target hostname which to connect to

   @param client Client context
   @param hostname Host name to connect to. This is resolved through
          standard libc resolution.
   @param port Port to connect to
   @param ip_flags Librudp connection flags. See librudp documentation

   @returns 0 if OK, or an error taken from errno(7)
 */
static inline
rudp_error_t rudp_hid_client_set_hostname(
    struct rudp_hid_client *client,
    const char *hostname,
    const uint16_t port,
    uint32_t ip_flags)
{
    return rudp_client_set_hostname(
        &client->base, hostname, port, ip_flags);
}

/**
   @mgroup {Connection management}

   @this sets the IPv4 to connect to

   @param client Client context
   @param address IPv4 address, in @tt in_addr usual order
   @param port Port to connect to
*/
static inline
void rudp_hid_client_set_ipv4(
    struct rudp_hid_client *client,
    const struct in_addr *address,
    const uint16_t port)
{
    return rudp_client_set_ipv4(
        &client->base, address, port);
}

/**
   @mgroup {Connection management}

   @this sets the IPv6 to connect to

   @deprecated
   This function should not be used anymore, since it does not allow
   to set the IPv6 scope. Use @ref foils_hid_client_connect instead.

   @param client Client context
   @param address IPv6 address, in @tt in6_addr usual order
   @param port Port to connect to
*/
static inline
void rudp_hid_client_set_ipv6(
    struct rudp_hid_client *client,
    const struct in6_addr *address,
    const uint16_t port)
{
    struct sockaddr_in6 addr6;

    memset(&addr6, 0, sizeof (addr6));
    addr6.sin6_family = AF_INET6;
    addr6.sin6_addr = *address;
    addr6.sin6_port = ntohs(port);

    rudp_client_set_addr(
        &client->base, (struct sockaddr *)&addr6, sizeof (addr6));
}

/**
   @mgroup {Connection management}

   @this sets the IPv6 to connect to

   @param client Client context
   @param address IPv4 or IPv6 address
   @param addrlen Size of the address structure
*/
static inline
void rudp_hid_client_set_addr(
    struct rudp_hid_client *client,
    const struct sockaddr *address,
    socklen_t addrlen)
{
    rudp_client_set_addr(&client->base, address, addrlen);
}

/**
   @mgroup {Connection management}

   @this makes the client attempt to connect to the server.  The
   target address must be populated before this call.

   @param client Client context
   @returns 0 when done, or an error from errno(7)
 */
static inline
int rudp_hid_client_connect(
    struct rudp_hid_client *client)
{
    return rudp_client_connect(&client->base);
}

/**
   @mgroup {Connection management}

   @this makes the client disconnect its server, or cease trying to
   connect.  @ref rudp_hid_client_connect must have been called before
   this call.

   @param client Client context
   @returns 0 when done, or an error from errno(7)
 */
static inline
int rudp_hid_client_close(
    struct rudp_hid_client *client)
{
    return rudp_client_close(&client->base);
}


/**
   @mgroup {Client context management}

   @this releases all context of the client.

   Client must not be connecting nor connected when calling this function.

   @param rlh Client context
 */
void rudp_hid_client_deinit(
    struct rudp_hid_client *client);

/**
   @mgroup {Protocol handlers}

   @this sends a message to the server signalling presence for a new
   device

   @param rlh Client context
   @param desc Device descriptor
   @param device_id Device index
 */
int rudp_hid_device_new(
    struct rudp_hid_client *client,
    const struct foils_hid_device_descriptor *desc,
    uint32_t device_id);

/**
   @mgroup {Protocol handlers}

   @this sends a message to the server signalling disapperance of a
   device.

   @param rlh Client context
   @param device_id Device index
 */
int rudp_hid_device_dropped(
    struct rudp_hid_client *client,
    uint32_t device_id);

/**
   @mgroup {Protocol handlers}

   @this sends a feature report from a given device.

   User must ensure the payload correspond to the device's report
   descriptor.

   @param client Client state
   @param device_id Device index
   @param report_id Report index
   @param reliable Whether this report may be lost in transport
   @param data Report data
   @param datalen Report data size
 */
int rudp_hid_feature_report_send(
    struct rudp_hid_client *client,
    uint32_t device_id,
    uint8_t report_id,
    int reliable,
    const void *data, size_t datalen);

/**
   @mgroup {Protocol handlers}

   @this sends an input report from a given device.

   User must ensure the payload correspond to the device's report
   descriptor.

   @param client Client state
   @param device_id Device index
   @param report_id Report index
   @param reliable Whether this report may be lost in transport
   @param data Report data
   @param datalen Report data size
 */
int rudp_hid_input_report_send(
    struct rudp_hid_client *client,
    uint32_t device_id,
    uint8_t report_id,
    int reliable,
    const void *data, size_t datalen);

#endif
