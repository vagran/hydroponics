/* This file is a part of 'hydroponics' project.
 * Copyright (c) 2012-2015, Artyom Lebedev <artyom.lebedev@gmail.com>
 * All rights reserved.
 * See COPYING file for copyright details.
 */

/** @file lighting.cpp */

#include "cpu.h"

using namespace adk;

Light light;

void
Light::OnAdcResult(u8 channel __UNUSED, u16 value __UNUSED)
{
    //XXX
}
