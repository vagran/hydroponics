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
    drawMask = DrawMask::M_ALL;
    drawInProgress = false;
    closeRequested = false;
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
    AtomicSection as;
    pumpActive = !pumpActive;
    Draw(DrawMask::M_PUMP);
}

void
MainPage::Poll()
{
    Page::Poll();
    AtomicSection as;
    if (!drawInProgress && drawMask) {
        Draw(drawMask);
    }
}

bool
MainPage::RequestClose()
{
    AtomicSection as;
    closeRequested = true;
    return !drawInProgress;
}

void
MainPage::Draw(u16 mask)
{
    AtomicSection as;
    if (closeRequested) {
        return;
    }
    drawMask |= mask;
    if (drawInProgress) {
        return;
    }
    drawState = 0;
    IssueDrawRequest();
}

void
MainPage::IssueDrawRequest()
{
    do {
        switch (drawState) {

        case DrawState::POT_WALLS_1:
            if (!(drawMask & DrawMask::M_STATIC)) {
                drawState = DrawState::PUMP;
                break;
            }
            drawMask &= ~DrawMask::M_STATIC;
            bitmapWriter.Write(POT_COL, POT_PAGE, &bitmaps.PotWall, false,
                               _DrawHandler);
            return;

        case DrawState::POT_WALLS_2:
            bitmapWriter.Write(POT_COL + 23, POT_PAGE, &bitmaps.PotWall, false,
                               _DrawHandler);
            return;

        case DrawState::POT_WALLS_3:
            bitmapWriter.Write(POT_COL + 16, POT_PAGE, &bitmaps.SiphonTop, false,
                               _DrawHandler);
            return;

        case DrawState::POT_WALLS_4:
            bitmapWriter.Write(POT_COL + 16, POT_PAGE + 2,
                               &bitmaps.SiphonBottom, false,
                               _DrawHandler);
            return;

        case DrawState::POT_WALLS_5:
            bitmapWriter.Write(POT_COL + 16, POT_PAGE + 1,
                               &bitmaps.SiphonWall, false,
                               _DrawHandler);
            return;

        case DrawState::POT_WALLS_PUMP1:
            bitmapWriter.Write(POT_COL - 4, POT_PAGE + 2,
                               &bitmaps.PumpPipeTop, false,
                               _DrawHandler);
            return;

        case DrawState::POT_WALLS_PUMP2:
            bitmapWriter.Write(POT_COL - 4, POT_PAGE + 4,
                               &bitmaps.PumpPipeBottom, false,
                               _DrawHandler);
            return;

        case DrawState::PUMP:
            if (!(drawMask & DrawMask::M_PUMP)) {
                drawState = DrawState::PUMP;//XXX
                break;
            }
            drawMask &= ~DrawMask::M_PUMP;
            bitmapWriter.Write(POT_COL - 8, POT_PAGE + 3,
                               pumpActive ? &bitmaps.PumpActive : &bitmaps.PumpInactive,
                               false, _DrawHandler);
            return;

        default:
            break;
        }
    } while (drawMask || drawState < DrawState::LAST);
    drawInProgress = false;
}

void
MainPage::DrawHandler()
{
    AtomicSection as;

    if (closeRequested) {
        drawInProgress = false;
        return;
    }
    if (drawState != DrawState::LAST) {
        drawState++;
        IssueDrawRequest();
        return;
    }
    drawInProgress = false;
}

void
MainPage::_DrawHandler()
{
    return static_cast<MainPage *>(app.CurPage())->DrawHandler();
}

bool
MainPage::DisplayOutputHandler(u8 , u8 , u8 *)
{
    //XXX
    return true;
}

bool
MainPage::_DisplayOutputHandler(u8 column, u8 page, u8 *data)
{
    return static_cast<MainPage *>(app.CurPage())->
        DisplayOutputHandler(column, page, data);
}
