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
Page::Poll()
{
    if (poll) {
        poll();
    }
}

Application::Application()
{
    SetNextPage(GetPageTypeCode<MainPage>(), MainPage::Fabric);
}

void
Application::Initialize()
{
    sound.SetPattern(0xffff, false);
    scheduler.ScheduleTask(_Tick, TASK_DELAY_S(1));
}

void
Application::SetNextPage(u8 pageTypeCode, VariantFabric page)
{
    AtomicSection as;
    nextPageTypeCode = pageTypeCode;
    nextPage = page;
}

void
Application::Poll()
{
    AtomicSection as;
    Page *page = CurPage();
    if (nextPage) {
        if (!page || page->RequestClose()) {
            SetPage(nextPage);
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
Application::SetPage(VariantFabric page)
{
    curPage.Engage(nextPageTypeCode, page);
    nextPage = nullptr;
}

u16
Application::_Tick()
{
    return app.Tick();
}

u16
Application::Tick()
{
    rtc.Update();
    //XXX
    return TASK_DELAY_S(60);
}
