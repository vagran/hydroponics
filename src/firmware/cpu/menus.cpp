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

    //XXX
    case I_TEST1:
    case I_TEST2:
        break;
    }
}
