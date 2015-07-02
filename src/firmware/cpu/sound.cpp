/* This file is a part of 'hydroponics' project.
 * Copyright (c) 2012-2015, Artyom Lebedev <artyom.lebedev@gmail.com>
 * All rights reserved.
 * See COPYING file for copyright details.
 */

/** @file sound.cpp */

#include "cpu.h"

using namespace adk;

Sound sound;

Sound::Sound()
{
    running = false;
    patPos = 0;
}

void
Sound::Initialize()
{
    scheduler.ScheduleTask(_Animator, ANIMATION_PERIOD);
}

void
Sound::SetPattern(u16 pattern, bool repeat)
{
    this->pattern = pattern;
    this->repeat = repeat;
    patPos = 0;
    running = true;
    isActive = false;
    rtc.SetSound(false);
}

u16
Sound::_Animator()
{
    return sound.Animator();
}

u16
Sound::Animator()
{
    if (!running) {
        return ANIMATION_PERIOD;
    }
    if (pattern & (1 << patPos)) {
        if (!isActive) {
            isActive = true;
            rtc.SetSound(true);
        }
    } else {
        if (isActive) {
            isActive = false;
            rtc.SetSound(false);
        }
    }
    patPos++;
    if (patPos == 0 && !repeat) {
        running = false;
        if (isActive) {
            isActive = false;
            rtc.SetSound(false);
        }
        return ANIMATION_PERIOD;
    }
    return ANIMATION_PERIOD;
}
