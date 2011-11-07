/*
  Foils_hid, HID device client for Foils

  This file is part of FOILS, the Freebox Open Interface
  Libraries. This file is distributed under a 2-clause BSD license,
  see LICENSE.TXT for details.

  Copyright (c) 2011, Freebox SAS
  See AUTHORS for details
 */

#include <stdlib.h>
#include <stdio.h>
#include "mapping.h"
#include "term_input.h"

#define DC_SYSTEM_POWER_DOWN 0x01
#define DC_SYSTEM_POWER_SLEEP 0x02
#define DC_SYSTEM_POWER_WAKEUP 0x04
#define DC_SYSTEM_CONTEXT_MENU 0x08

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
#define HID_CONSUMER_RANDOM_PLAY 0xb9

#define HID_CONSUMER_AC_ZOOM_IN 0x22d
#define HID_CONSUMER_AC_ZOOM_OUT 0x22e

#define HID_CONSUMER_AC_SEARCH 0x221
#define HID_CONSUMER_AC_PROPERTIES 0x209
#define HID_CONSUMER_AC_EXIT 0x204

#define HID_CONSUMER_AL_TASK_MANAGER 0x18f
#define HID_CONSUMER_AL_INTERNET_BROWSER 0x196
#define HID_CONSUMER_AL_AUDIO_BROWSER 0x1b7

static const
char * const target_name[TERM_INPUT_COUNT] =
{
    [TERM_INPUT_CTRL('a')] = "Ctrl+a",
    [TERM_INPUT_CTRL('b')] = "Ctrl+b",
    [TERM_INPUT_CTRL('c')] = "Ctrl+c",
    [TERM_INPUT_CTRL('d')] = "Ctrl+d",
    [TERM_INPUT_CTRL('e')] = "Ctrl+e",
    [TERM_INPUT_CTRL('f')] = "Ctrl+f",
    [TERM_INPUT_CTRL('g')] = "Ctrl+g",
    [TERM_INPUT_CTRL('h')] = "Ctrl+h",
    [TERM_INPUT_CTRL('i')] = "Ctrl+i",
    [TERM_INPUT_CTRL('j')] = "Ctrl+j",
    [TERM_INPUT_CTRL('k')] = "Ctrl+k",
    [TERM_INPUT_CTRL('l')] = "Ctrl+l",
    [TERM_INPUT_CTRL('m')] = "Ctrl+m",
    [TERM_INPUT_CTRL('n')] = "Ctrl+n",
    [TERM_INPUT_CTRL('o')] = "Ctrl+o",
    [TERM_INPUT_CTRL('p')] = "Ctrl+p",
    [TERM_INPUT_CTRL('q')] = "Ctrl+q",
    [TERM_INPUT_CTRL('r')] = "Ctrl+r",
    [TERM_INPUT_CTRL('s')] = "Ctrl+s",
    [TERM_INPUT_CTRL('t')] = "Ctrl+t",
    [TERM_INPUT_CTRL('u')] = "Ctrl+u",
    [TERM_INPUT_CTRL('v')] = "Ctrl+v",
    [TERM_INPUT_CTRL('w')] = "Ctrl+w",
    [TERM_INPUT_CTRL('x')] = "Ctrl+x",
    [TERM_INPUT_CTRL('y')] = "Ctrl+y",
    [TERM_INPUT_CTRL('z')] = "Ctrl+z",
    [TERM_INPUT_BACKSPACE] = "Backspace",
    [TERM_INPUT_UP] = "Up",
    [TERM_INPUT_DOWN] = "Down",
    [TERM_INPUT_LEFT] = "Left",
    [TERM_INPUT_RIGHT] = "Right",
    [TERM_INPUT_HOME] = "Home",
    [TERM_INPUT_END] = "End",
    [TERM_INPUT_F(1)] = "F1",
    [TERM_INPUT_F(2)] = "F2",
    [TERM_INPUT_F(3)] = "F3",
    [TERM_INPUT_F(4)] = "F4",
    [TERM_INPUT_F(5)] = "F5",
    [TERM_INPUT_F(6)] = "F6",
    [TERM_INPUT_F(7)] = "F7",
    [TERM_INPUT_F(8)] = "F8",
    [TERM_INPUT_F(9)] = "F9",
    [TERM_INPUT_F(10)] = "F10",
    [TERM_INPUT_F(11)] = "F11",
    [TERM_INPUT_F(12)] = "F12",
    [TERM_INPUT_F(13)] = "F13",
    [TERM_INPUT_F(14)] = "F14",
    [TERM_INPUT_F(15)] = "F15",
    [TERM_INPUT_F(16)] = "F16",
    [TERM_INPUT_F(17)] = "F17",
    [TERM_INPUT_F(18)] = "F18",
    [TERM_INPUT_F(19)] = "F19",
    [TERM_INPUT_F(20)] = "F20",
    [TERM_INPUT_F(21)] = "F21",
    [TERM_INPUT_F(22)] = "F22",
    [TERM_INPUT_F(23)] = "F23",
    [TERM_INPUT_F(24)] = "F24",
    [TERM_INPUT_PAGE_UP] = "PageUp",
    [TERM_INPUT_PAGE_DOWN] = "PageDown",
};

static const
struct target_code targets[TERM_INPUT_COUNT] =
{
    [TERM_INPUT_CTRL('j')] = {TARGET_KEYBOARD, HID_KEYBOARD_ENTER, "Enter"},
    [TERM_INPUT_CTRL('m')] = {TARGET_KEYBOARD, HID_KEYBOARD_ENTER, "Enter"},
    [TERM_INPUT_CTRL('i')] = {TARGET_KEYBOARD, HID_KEYBOARD_TAB, "Tab"},
    [TERM_INPUT_CTRL('t')] = {TARGET_CONSUMER, HID_CONSUMER_ALTERNATE_SUBTITLE_INCREMENT, "Subtitle Track"},
    [TERM_INPUT_CTRL('u')] = {TARGET_CONSUMER, HID_CONSUMER_ALTERNATE_AUDIO_INCREMENT, "Audio Track"},
    [TERM_INPUT_CTRL('v')] = {TARGET_CONSUMER, HID_CONSUMER_SUB_CHANNEL_INCREMENT, "Video Track"},
    [TERM_INPUT_CTRL('r')] = {TARGET_CONSUMER, HID_CONSUMER_RANDOM_PLAY, "Random Play"},
    [TERM_INPUT_CTRL('w')] = {TARGET_KEYBOARD, HID_KEYBOARD_POWER, "Power/kbd"},
    [TERM_INPUT_CTRL('p')] = {TARGET_CONSUMER, HID_CONSUMER_MUTE, "Mute"},
    [TERM_INPUT_BACKSPACE] = {TARGET_KEYBOARD, HID_KEYBOARD_BACKSPACE, "Backspace"},
    [TERM_INPUT_UP] = {TARGET_KEYBOARD, HID_KEYBOARD_UPARROW, "Up"},
    [TERM_INPUT_DOWN] = {TARGET_KEYBOARD, HID_KEYBOARD_DOWNARROW, "Down"},
    [TERM_INPUT_LEFT] = {TARGET_KEYBOARD, HID_KEYBOARD_LEFTARROW, "Left"},
    [TERM_INPUT_RIGHT] = {TARGET_KEYBOARD, HID_KEYBOARD_RIGHTARROW, "Right"},
    [TERM_INPUT_HOME] = {TARGET_CONSUMER, HID_CONSUMER_AL_TASK_MANAGER, "Task Manager"},
    [TERM_INPUT_F(1)] = {TARGET_CONSUMER, HID_CONSUMER_AC_SEARCH, "AC Search"},
    [TERM_INPUT_F(2)] = {TARGET_CONSUMER, HID_CONSUMER_AC_EXIT, "AC Exit"},
    [TERM_INPUT_F(3)] = {TARGET_DESKTOP, DC_SYSTEM_CONTEXT_MENU, "Context Menu"},
    [TERM_INPUT_F(4)] = {TARGET_CONSUMER, HID_CONSUMER_AC_PROPERTIES, "AC Properties"},
    [TERM_INPUT_F(5)] = {TARGET_CONSUMER, HID_CONSUMER_EJECT, "Eject"},
    [TERM_INPUT_F(6)] = {TARGET_CONSUMER, HID_CONSUMER_AL_INTERNET_BROWSER, "AL Internet Browser"},
    [TERM_INPUT_F(7)] = {TARGET_CONSUMER, HID_CONSUMER_AC_ZOOM_IN, "Zoom +"},
    [TERM_INPUT_F(9)] = {TARGET_CONSUMER, HID_CONSUMER_VOLUME_DECREMENT, "Vol-"},
    [TERM_INPUT_F(10)] = {TARGET_CONSUMER, HID_CONSUMER_VOLUME_INCREMENT, "Vol+"},
    [TERM_INPUT_F(11)] = {TARGET_DESKTOP, DC_SYSTEM_POWER_SLEEP, "System Sleep"},
    [TERM_INPUT_F(12)] = {TARGET_DESKTOP, DC_SYSTEM_POWER_WAKEUP, "System Wakeup"},
    [TERM_INPUT_F(13)] = {TARGET_CONSUMER, HID_CONSUMER_STOP, "Stop"},
    [TERM_INPUT_F(14)] = {TARGET_CONSUMER, HID_CONSUMER_REWIND, "Rewind"},
    [TERM_INPUT_F(15)] = {TARGET_CONSUMER, HID_CONSUMER_PLAY, "Play"},
    [TERM_INPUT_F(16)] = {TARGET_CONSUMER, HID_CONSUMER_FAST_FORWARD, "FastForward"},
    [TERM_INPUT_F(17)] = {TARGET_CONSUMER, HID_CONSUMER_RECORD, "Record"},
    [TERM_INPUT_F(18)] = {TARGET_CONSUMER, HID_CONSUMER_RANDOM_PLAY, "Random Play"},
    [TERM_INPUT_F(19)] = {TARGET_CONSUMER, HID_CONSUMER_SCAN_PREVIOUS_TRACK, "Previous track"},
    [TERM_INPUT_F(20)] = {TARGET_CONSUMER, HID_CONSUMER_SCAN_NEXT_TRACK, "Next track"},
    [TERM_INPUT_F(21)] = {TARGET_CONSUMER, HID_CONSUMER_SUB_CHANNEL_INCREMENT, "Video Track"},
    [TERM_INPUT_F(22)] = {TARGET_CONSUMER, HID_CONSUMER_ALTERNATE_AUDIO_INCREMENT, "Audio Track"},
    [TERM_INPUT_F(23)] = {TARGET_CONSUMER, HID_CONSUMER_ALTERNATE_SUBTITLE_INCREMENT, "Subtitle Track"},
    [TERM_INPUT_F(24)] = {TARGET_CONSUMER, HID_CONSUMER_FAST_FORWARD, "FastForward"},
    [TERM_INPUT_PAGE_UP] = {TARGET_CONSUMER, HID_CONSUMER_CHANNEL_INCREMENT, "Chan+"},
    [TERM_INPUT_PAGE_DOWN] = {TARGET_CONSUMER, HID_CONSUMER_CHANNEL_DECREMENT, "Chan-"},
};

const struct target_code *mapping_get(enum term_input_key key)
{
    if (key >= TERM_INPUT_COUNT)
        return NULL;

    return &targets[key];
}

void mapping_dump_target(enum term_input_key key)
{
    const struct target_code *target = mapping_get(key);

    if (target && target->report)
        printf("%s: %s\n", target_name[key], target->name);
    else
        printf("%s: [none]\n",
               key < TERM_INPUT_COUNT ? target_name[key] : "Unknown");
}

void mapping_dump()
{
    uint32_t i;

    for (i=0; i<TERM_INPUT_COUNT; ++i) {
        const char *name = target_name[i];
        const struct target_code *code = mapping_get(i);

        if (code->report == TARGET_NONE)
            continue;

        printf("%s: %s\n", name, code->name);
    }
}

