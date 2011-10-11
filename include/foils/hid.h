/*
  Foils_hid, HID device client for Foils

  This file is part of FOILS, the Freebox Open Interface
  Libraries. This file is distributed under a 2-clause BSD license,
  see LICENSE.TXT for details.

  Copyright (c) 2011, Freebox SAS
  See AUTHORS for details
 */

#ifndef FOILS_HID_H
#define FOILS_HID_H

/**
   @file
   @module {HID device client}
   @short Human interface device high-level client
*/

#include <rudp/rudp.h>
#include <rudp/client.h>
#include <foils/rudp_hid_client.h>

struct foils_hid;

/**
   @this is the client state.  User code gets notified of state
   evolution through the @ref foils_hid_handler::status.
 */
enum foils_hid_state
{
    FOILS_HID_IDLE,
    FOILS_HID_CONNECTING,
    FOILS_HID_CONNECTED,
    FOILS_HID_RESOLVE_FAILED,
    FOILS_HID_DROPPED,
};

/**
   @this is a set of callbacks from the client state to user code.
 */
struct foils_hid_handler
{
    /**
       @this notifies the user code of current client state.
       @param client The client context
       @param state The current state
     */
    void (*status)(
        struct foils_hid *client,
        enum foils_hid_state state);

    /**
       @this is called when a device receives a feature report
       @param client The client context
       @param device_id The device index in the declared array
       @param report_id The report id in the device
       @param data Report blob
       @param datalen Received blob size
     */
    void (*feature_report)(
        struct foils_hid *client,
        uint32_t device_id, uint8_t report_id,
        const void *data, size_t datalen);

    /**
       @this is called when a device receives an output report
       @param client The client context
       @param device_id The device index in the declared array
       @param report_id The report id in the device
       @param data Report blob
       @param datalen Received blob size
     */
    void (*output_report)(
        struct foils_hid *client,
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
        struct foils_hid *client,
        uint32_t device_id, uint8_t report_id);
};

/**
   @this is the foils HID device client state.

   @hidecontent
 */
struct foils_hid
{
    struct rudp_hid_client client;
    const struct foils_hid_handler *handler;
    struct rudp rudp;
    struct ela_el *el;
    uint32_t enable;
    struct foils_grab_accountant *ga;
    int state;
    const struct foils_hid_device_descriptor *descriptor;
    size_t descriptor_count;
};

/**
   @this initializes the foils HID state from a set of devices.

   @param rlh The client state
   @param el A valid event loop abstraction handle
   @param handler The user-provided handler function structure
   @param device A device array
   @param device_count Count of devices in the array, limited to 32.

   @returns 0 when done, or an error taken from errno(7)
 */
int foils_hid_init(
    struct foils_hid *rlh,
    struct ela_el *el,
    const struct foils_hid_handler *handler,
    const struct foils_hid_device_descriptor *device,
    size_t device_count);

/**
   @this enables a device. An enabled device becomes visible to the
   server.

   @param rlh The client state
   @param index Device index in the array
 */
void foils_hid_device_enable(struct foils_hid *rlh, size_t index);

/**
   @this disables a device. Disabling a device makes it appear
   disconnected.

   @param rlh The client state
   @param index Device index in the array
 */
void foils_hid_device_disable(struct foils_hid *rlh, size_t index);

/**
   @this sends an input report from a given device.

   User must ensure the payload correspond to the device's report
   descriptor.

   @param rlh The client state
   @param device_index Device index in the array
   @param report_id Report index
   @param reliable Whether this report may be lost in transport
   @param data Report data
   @param datalen Report data size
 */
void foils_hid_input_report_send(
    struct foils_hid *rlh,
    size_t device_index, uint8_t report_id,
    int reliable,
    const void *data, size_t datalen);

/**
   @this sends a feature report from a given device.

   User must ensure the payload correspond to the device's report
   descriptor.

   @param rlh The client state
   @param device_index Device index in the array
   @param report_id Report index
   @param reliable Whether this report may be lost in transport
   @param data Report data
   @param datalen Report data size
 */
void foils_hid_feature_report_send(
    struct foils_hid *rlh,
    size_t device_index, uint8_t report_id,
    int reliable,
    const void *data, size_t datalen);

/**
   @this releases all context of the client.

   This disconnects from the server and makes all devices unavailable.

   @param rlh Client context
 */
void foils_hid_deinit(struct foils_hid *rlh);


/**
   @this tries to connect to the given hostname and port

   @param client Client context
   @param hostname Host name to connect to. This is resolved through
          standard libc resolution.
   @param port Port to connect to
   @param ip_flags Librudp connection flags. See librudp documentation

   @returns 0 if OK, or an error taken from errno(7)
 */
int foils_hid_client_connect_hostname(
    struct foils_hid *client,
    const char *hostname,
    const uint16_t port,
    uint32_t ip_flags);

/**
   @this tries to connect to the given IPv4 address and port

   @param client Client context
   @param address IPv4 address, in @tt in_addr usual order
   @param port Port to connect to
*/
void foils_hid_client_connect_ipv4(
    struct foils_hid *client,
    const struct in_addr *address,
    const uint16_t port);

/**
   @this tries to connect to the given IPv6 address and port

   @param client Client context
   @param address IPv6 address, in @tt in6_addr usual order
   @param port Port to connect to
*/
void foils_hid_client_connect_ipv6(
    struct foils_hid *client,
    const struct in6_addr *address,
    const uint16_t port);

#endif
