/* This file is a part of 'hydroponics' project.
 * Copyright (c) 2012-2015, Artyom Lebedev <artyom.lebedev@gmail.com>
 * All rights reserved.
 * See COPYING file for copyright details.
 */

/** @file application.cpp
 * TODO insert description here.
 */

#include "cpu.h"

using namespace adk;

Application app;

void
Application::Poll()
{
    Page *page = CurPage();
    if (page) {
        page->Poll();
    }
}

void
Application::OnButtonPressed()
{
    Page *page = CurPage();
    if (page) {
        page->OnButtonPressed();
    }
}

void
Application::OnButtonLongPressed()
{
    Page *page = CurPage();
    if (page) {
        page->OnButtonLongPressed();
    }
}

void
Application::OnRotEncClick(bool dir)
{
    Page *page = CurPage();
    if (page) {
        page->OnRotEncClick(dir);
    }
}
