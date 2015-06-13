/* This file is a part of 'hydroponics' project.
 * Copyright (c) 2012-2015, Artyom Lebedev <artyom.lebedev@gmail.com>
 * All rights reserved.
 * See COPYING file for copyright details.
 */

/** @file main_page.cpp */

#include "cpu.h"

using namespace adk;

void
MainPage::Fabric(void *p)
{
    new (p) MainPage();
}

MainPage::MainPage()
{
    //XXX
    static bool inv = false;
    textWriter.Write(Display::Viewport {0, 127, 1, 2},
        strings.Test, inv);//"Warning! Low water", inv);

    bitmapWriter.Write(8, 6, &bitmaps.Thermometer, inv);
    bitmapWriter.Write(16, 6, &bitmaps.Sun, inv);
}

void
MainPage::OnButtonPressed()
{
    app.SetNextPage(Application::GetPageTypeCode<Menu>(),
                    MainMenu::Fabric);
}

void
MainPage::OnRotEncClick(bool dir __UNUSED/*XXX*/)
{
    //XXX
}
