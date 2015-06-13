/* This file is a part of 'hydroponics' project.
 * Copyright (c) 2012-2015, Artyom Lebedev <artyom.lebedev@gmail.com>
 * All rights reserved.
 * See COPYING file for copyright details.
 */

/** @file menus.cpp */

#include "cpu.h"

using namespace adk;

void
MainMenu::OnItemSelected(u8 idx)
{
    switch (static_cast<Idx>(idx)) {
    case I_RETURN:
        app.SetNextPage(Application::Pages::MAIN);
        break;
    case I_MANUAL_CONTROL:
        app.SetNextPage(Application::Pages::MANUAL_CONTROL_MENU);
        break;
    case I_CALIBRATION:
        app.SetNextPage(Application::Pages::CALIBRATION_MENU);
        break;
    case I_SETUP:
        app.SetNextPage(Application::Pages::SETUP_MENU);
        break;
    }
}
