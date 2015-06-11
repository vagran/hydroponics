/* This file is a part of 'hydroponics' project.
 * Copyright (c) 2012-2015, Artyom Lebedev <artyom.lebedev@gmail.com>
 * All rights reserved.
 * See COPYING file for copyright details.
 */

/** @file main_page.cpp */

#include "cpu.h"

using namespace adk;

MainMenu menu;//XXX

//XXX
static u8 tmpMode;

void
MainPage::OnButtonPressed()
{
    //XXX
    tmpMode = ~tmpMode;

    static bool inv = false;

    textWriter.Write(Display::Viewport {0, 127, 1, 2},
        strings.Test, inv);//"Warning! Low water", inv);

    bitmapWriter.Write(8, 6, &bitmaps.Thermometer, inv);
    bitmapWriter.Write(16, 6, &bitmaps.Sun, inv);

    inv = !inv;
}

void
MainPage::OnRotEncClick(bool dir)
{
    //XXX
    if (tmpMode) {
        u8 pwm = Pwm3Get();
        if (dir && pwm < 255) {
            pwm++;
        } else if (!dir && pwm > 0) {
            pwm--;
        }
        Pwm3Set(pwm);
    } else {
        u8 pwm = Pwm2Get();
        if (dir && pwm < 255) {
            pwm++;
        } else if (!dir && pwm > 0) {
            pwm--;
        }
        Pwm2Set(pwm);
    }
}
