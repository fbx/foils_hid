/*
  Foils_hid, HID device client for Foils

  This file is part of FOILS, the Freebox Open Interface
  Libraries. This file is distributed under a 2-clause BSD license,
  see LICENSE.TXT for details.

  Copyright (c) 2011, Freebox SAS
  See AUTHORS for details
 */

#ifndef MAPPING_H_
#define MAPPING_H_

#include <stdbool.h>
#include <inttypes.h>
#include "term_input.h"

enum target_code_type
{
    TARGET_NONE,
    TARGET_UNICODE,
    TARGET_KEYBOARD,
    TARGET_CONSUMER,
    TARGET_DESKTOP,
};

struct target_code
{
    enum target_code_type report;
    uint32_t usage;
    char name[32];
};

bool mapping_get(bool is_unicode, uint32_t code,struct target_code *out_target);
void mapping_dump_target(const struct target_code *target);
void mapping_dump(void);

#endif
