/* This file is a part of 'hydroponics' project.
 * Copyright (c) 2012-2015, Artyom Lebedev <artyom.lebedev@gmail.com>
 * All rights reserved.
 * See LICENSE file for copyright details.
 */

/** @file strings.cpp */

#include "cpu.h"

using namespace adk;

void
Strings::StrClockNum(u8 value, char *buf)
{
    u8 dec = value / 10;
    value -= dec * 10;
    if (dec > 9) {
        dec = 9;
    }
    buf[0] = '0' + dec;
    buf[1] = '0' + value;
    buf[2] = 0;
}
