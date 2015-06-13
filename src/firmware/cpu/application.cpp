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

Application::Application()
{
    curPageCode = Pages::NONE;
    nextPageCode = Pages::MAIN;
}

void
Application::Poll()
{
    AtomicSection as;
    Page *page = CurPage();
    if (nextPageCode != curPageCode) {
        if (!page || page->RequestClose()) {
            SetPage(static_cast<Pages>(nextPageCode));
            page = CurPage();
        }
    }
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

void
Application::SetPage(Pages page)
{
    switch (page) {
    case Pages::NONE:
        curPage.Disengage();
        break;
    case Pages::MAIN:
        curPage.Engage<MainPage>();
        break;
    case Pages::MAIN_MENU:
        curPage.Engage<MainMenu>(menuPos);
        break;
    case Pages::MANUAL_CONTROL_MENU:
        curPage.Engage<ManualControlMenu>(menuPos);
        break;
    case Pages::CALIBRATION_MENU:
        curPage.Engage<CalibrationMenu>(menuPos);
        break;
    case Pages::SETUP_MENU:
        curPage.Engage<SetupMenu>(menuPos);
        break;
    }
    curPageCode = nextPageCode;
}
