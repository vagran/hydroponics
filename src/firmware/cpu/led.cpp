/* This file is a part of 'hydroponics' project.
 * Copyright (c) 2015, Artyom Lebedev <artyom.lebedev@gmail.com>
 * All rights reserved.
 * See LICENSE file for copyright details.
 */

/** @file led.cpp */

#include "cpu.h"

using namespace adk;

Led led;

Led::Led()
{
    AVR_BIT_SET8(AVR_REG_DDR(LED_PORT), LED_PIN);
    mode = static_cast<u8>(Mode::STANDBY);
    patPos = 0;
    pattern = Pattern::STANDBY;
}

void
Led::Initialize()
{
    scheduler.ScheduleTask(_Animator, ANIMATION_PERIOD);
}

u16
Led::_Animator()
{
    return led.Animator();
}

u16
Led::Animator()
{
    if (pattern & (1 << patPos)) {
        AVR_BIT_SET8(AVR_REG_PORT(LED_PORT), LED_PIN);
    } else {
        AVR_BIT_CLR8(AVR_REG_PORT(LED_PORT), LED_PIN);
    }
    patPos++;
    return ANIMATION_PERIOD;
}

void
Led::SetMode(Mode mode)
{
    this->mode = static_cast<u8>(mode);
    switch (mode) {
    case Mode::STANDBY:
        pattern = Pattern::STANDBY;
        break;
    case Mode::FAILURE:
        pattern = Pattern::FAILURE;
        break;
    }
    patPos = 0;
}
