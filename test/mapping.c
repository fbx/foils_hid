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
#include <string.h>
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
#define HID_KEYBOARD_INSERT 0x49
#define HID_KEYBOARD_HOME 0x4A
#define HID_KEYBOARD_DELETE 0x4C
#define HID_KEYBOARD_END 0x4D
#define HID_KEYBOARD_POWER 0x66
#define HID_KEYBOARD_F1 0x3A

#define HID_CONSUMER_SUB_CHANNEL_INCREMENT 0x171
#define HID_CONSUMER_ALTERNATE_AUDIO_INCREMENT 0x173
#define HID_CONSUMER_ALTERNATE_SUBTITLE_INCREMENT 0x175
#define HID_CONSUMER_DATA_ON_SCREEN 0x60
#define HID_CONSUMER_CHANNEL_INCREMENT 0x9c
#define HID_CONSUMER_CHANNEL_DECREMENT 0x9d
#define HID_CONSUMER_PLAY 0xb0
#define HID_CONSUMER_PAUSE 0xb1
#define HID_CONSUMER_PLAY_PAUSE 0xcd
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
#define HID_CONSUMER_VOICE_COMMAND 0xcf
#define HID_CONSUMER_MENU 0x40

#define HID_CONSUMER_AC_ZOOM_IN 0x22d
#define HID_CONSUMER_AC_ZOOM_OUT 0x22e

#define HID_CONSUMER_AC_SEARCH 0x221
#define HID_CONSUMER_AC_PROPERTIES 0x209
#define HID_CONSUMER_AC_EXIT 0x204

#define HID_CONSUMER_AL_TASK_MANAGER 0x18f
#define HID_CONSUMER_AL_INTERNET_BROWSER 0x196
#define HID_CONSUMER_AL_AUDIO_BROWSER 0x1b7

#define HID_CONSUMER_FREEBOX_APP_TV 0xf01
#define HID_CONSUMER_FREEBOX_APP_REPLAY 0xf02
#define HID_CONSUMER_FREEBOX_APP_VIDEOCLUB 0xf03
#define HID_CONSUMER_FREEBOX_APP_WHATSON 0xf04
#define HID_CONSUMER_FREEBOX_APP_RECORDS 0xf05
#define HID_CONSUMER_FREEBOX_APP_MEDIA 0xf06
#define HID_CONSUMER_FREEBOX_APP_YOUTUBE 0xf07
#define HID_CONSUMER_FREEBOX_APP_RADIOS 0xf08
#define HID_CONSUMER_FREEBOX_APP_CANALVOD 0xf09
#define HID_CONSUMER_FREEBOX_APP_PIP 0xf0a
#define HID_CONSUMER_FREEBOX_APP_NETFLIX 0xf0b

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
    [TERM_INPUT_SHIFT_UP] = "Shift+Up",
    [TERM_INPUT_SHIFT_DOWN] = "Shif+Down",
    [TERM_INPUT_SHIFT_LEFT] = "Shif+Left",
    [TERM_INPUT_SHIFT_RIGHT] = "Shif+Right",
    [TERM_INPUT_CTRL_UP] = "Ctrl+Up",
    [TERM_INPUT_CTRL_DOWN] = "Ctrl+Down",
    [TERM_INPUT_CTRL_LEFT] = "Ctrl+Left",
    [TERM_INPUT_CTRL_RIGHT] = "Ctrl+Right",
    [TERM_INPUT_HOME] = "Home",
    [TERM_INPUT_END] = "End",
    [TERM_INPUT_INS] = "Insert",
    [TERM_INPUT_DEL] = "Delete",
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
    [TERM_INPUT_CTRL('r')] = {TARGET_CONSUMER, HID_CONSUMER_RECORD, "Record"},
    [TERM_INPUT_CTRL('p')] = {TARGET_CONSUMER, HID_CONSUMER_PLAY_PAUSE, "Play/Pause"},
    [TERM_INPUT_CTRL('d')] = {TARGET_CONSUMER, HID_CONSUMER_MUTE, "Mute"},
    [TERM_INPUT_CTRL('x')] = {TARGET_CONSUMER, HID_CONSUMER_AC_EXIT, "AC Exit"},
    [TERM_INPUT_BACKSPACE] = {TARGET_KEYBOARD, HID_KEYBOARD_BACKSPACE, "Backspace"},
    [TERM_INPUT_DEL] = {TARGET_KEYBOARD, HID_KEYBOARD_DELETE, "Delete"},
    [TERM_INPUT_INS] = {TARGET_KEYBOARD, HID_KEYBOARD_INSERT, "Insert"},
    [TERM_INPUT_HOME] = {TARGET_KEYBOARD, HID_KEYBOARD_HOME, "Home"},
    [TERM_INPUT_END] = {TARGET_KEYBOARD, HID_KEYBOARD_END, "End"},
    [TERM_INPUT_UP] = {TARGET_KEYBOARD, HID_KEYBOARD_UPARROW, "Up"},
    [TERM_INPUT_DOWN] = {TARGET_KEYBOARD, HID_KEYBOARD_DOWNARROW, "Down"},
    [TERM_INPUT_LEFT] = {TARGET_KEYBOARD, HID_KEYBOARD_LEFTARROW, "Left"},
    [TERM_INPUT_RIGHT] = {TARGET_KEYBOARD, HID_KEYBOARD_RIGHTARROW, "Right"},
    [TERM_INPUT_SHIFT_UP] = {TARGET_CONSUMER, HID_CONSUMER_VOLUME_INCREMENT, "Vol+"},
    [TERM_INPUT_SHIFT_DOWN] = {TARGET_CONSUMER, HID_CONSUMER_VOLUME_DECREMENT, "Vol-"},
    [TERM_INPUT_SHIFT_LEFT] = {TARGET_CONSUMER, HID_CONSUMER_REWIND, "Rewind"},
    [TERM_INPUT_SHIFT_RIGHT] = {TARGET_CONSUMER, HID_CONSUMER_FAST_FORWARD, "FastForward"},
    [TERM_INPUT_CTRL_UP] = {TARGET_CONSUMER, HID_CONSUMER_CHANNEL_INCREMENT, "Chan+"},
    [TERM_INPUT_CTRL_DOWN] = {TARGET_CONSUMER, HID_CONSUMER_CHANNEL_DECREMENT, "Chan-"},
    [TERM_INPUT_CTRL_LEFT] = {TARGET_CONSUMER, HID_CONSUMER_SCAN_PREVIOUS_TRACK, "Previous track"},
    [TERM_INPUT_CTRL_RIGHT] = {TARGET_CONSUMER, HID_CONSUMER_SCAN_NEXT_TRACK, "Next track"},
    [TERM_INPUT_F(1)] = {TARGET_CONSUMER, HID_CONSUMER_AC_SEARCH, "Search"},
    [TERM_INPUT_F(2)] = {TARGET_CONSUMER, HID_CONSUMER_AC_EXIT, "Exit"},
    [TERM_INPUT_F(3)] = {TARGET_CONSUMER, HID_CONSUMER_MENU, "Menu"},
    [TERM_INPUT_F(4)] = {TARGET_CONSUMER, HID_CONSUMER_AC_PROPERTIES, "Props"},
    [TERM_INPUT_F(5)] = {TARGET_CONSUMER, HID_CONSUMER_AL_TASK_MANAGER, "Home Screen"},
    [TERM_INPUT_F(6)] = {TARGET_CONSUMER, HID_CONSUMER_FREEBOX_APP_NETFLIX, "Netflix"},
    [TERM_INPUT_F(7)] = {TARGET_CONSUMER, HID_CONSUMER_FREEBOX_APP_TV, "TV"},
    [TERM_INPUT_F(8)] = {TARGET_CONSUMER, HID_CONSUMER_FREEBOX_APP_YOUTUBE, "YouTube"},
    [TERM_INPUT_F(9)] = {TARGET_CONSUMER, HID_CONSUMER_VOICE_COMMAND, "Voice Command"},
    [TERM_INPUT_F(10)] = {TARGET_CONSUMER, HID_CONSUMER_EJECT, "Eject"},
    [TERM_INPUT_F(11)] = {TARGET_CONSUMER, HID_CONSUMER_DATA_ON_SCREEN, "Debug overlay"},
    [TERM_INPUT_F(12)] = {TARGET_KEYBOARD, HID_KEYBOARD_POWER, "Power/kbd"},
    [TERM_INPUT_PAGE_UP] = {TARGET_CONSUMER, HID_CONSUMER_CHANNEL_INCREMENT, "Chan+"},
    [TERM_INPUT_PAGE_DOWN] = {TARGET_CONSUMER, HID_CONSUMER_CHANNEL_DECREMENT, "Chan-"},
};

bool mapping_get(bool is_unicode, uint32_t code, struct target_code *out_target)
{
    if (!out_target)
        return false;

    memset(out_target, 0, sizeof (*out_target));

    if (is_unicode) {
        /* dumb utf-32 to utf-8 conversion */
        if (code < 0x0080) {
            out_target->name[0] = code;
        } else if (code < 0x0800) {
            out_target->name[0] = 0xc0 | (code >> 6);
            out_target->name[1] = 0x80 | (code & 0x3f);
        } else if (code < 0x10000) {
            out_target->name[0] = 0xe0 | (code >> 12);
            out_target->name[1] = 0x80 | ((code >> 6) & 0x3f);
            out_target->name[2] = 0x80 | ((code >> 0) & 0x3f);
        } else if (code < 0x200000) {
            out_target->name[0] = 0xf0 | (code >> 18);
            out_target->name[1] = 0x80 | ((code >> 12) & 0x3f);
            out_target->name[2] = 0x80 | ((code >> 6) & 0x3f);
            out_target->name[3] = 0x80 | ((code >> 0) & 0x3f);
        } else {
            return false;
        }
        out_target->report = TARGET_UNICODE;
        out_target->usage = code;
    } else {
        if (code >= TERM_INPUT_COUNT)
            return false;
        *out_target = targets[code];
    }

    return true;
}

void mapping_dump()
{
    uint32_t i;

    printf("Input mapping:\n");
    for (i=0; i<TERM_INPUT_COUNT; ++i) {
        const char *name = target_name[i];
        const struct target_code *code = &targets[i];

        if (code->report == TARGET_NONE)
            continue;

        printf("  %s: %s\n", name, code->name);
    }
}

