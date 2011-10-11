/*
  Foils_hid, HID device client for Foils

  This file is part of FOILS, the Freebox Open Interface
  Libraries. This file is distributed under a 2-clause BSD license,
  see LICENSE.TXT for details.

  Copyright (c) 2011, Freebox SAS
  See AUTHORS for details
 */

#ifndef FOILS_HID_DEVIVCE_H_
#define FOILS_HID_DEVIVCE_H_

/**
   @file
   @module {HID device descriptor}
   @short Device internal definition
*/

#include <stdint.h>
#include <sys/types.h>

#define DEVICE_NAME_LEN 64

/**
   @this describes a device to the library context.
 */
struct foils_hid_device_descriptor
{
    /** Device product name */
    char name[DEVICE_NAME_LEN];
    /** Device version, in 2-byte hex format (e.g. 0x100 is 1.00 */
    uint16_t version;

    /** Device report descriptor */
    void *descriptor;
    /** Device report descriptor size */
    size_t descriptor_size;

    /** Device physical descriptor */
    void *physical;
    /** Device physical descriptor size */
    size_t physical_size;

    /** Device strings descriptor */
    void *strings;
    /** Device strings descriptor size */
    size_t strings_size;
};

#endif
