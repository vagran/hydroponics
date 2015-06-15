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
    pumpActive = false;
    drainActive = false;

    //XXX
    watLevelBottom = MAX_WATER_LEVEL;
    watLevelTop = 0;
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
    drainActive = !drainActive;
    if (dir) {
        if (watLevelBottom < MAX_WATER_LEVEL) {
            watLevelBottom++;
        }
        if (watLevelTop > 0) {
            watLevelTop--;
        }
    } else {
        if (watLevelBottom > 0) {
            watLevelBottom--;
        }
        if (watLevelTop < MAX_WATER_LEVEL) {
            watLevelTop++;
        }
    }
    Draw(DrawMask::M_PUMP | DrawMask::M_DRAIN | DrawMask::M_BOTTOM_WATER |
         DrawMask::M_TOP_WATER);
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
    drawInProgress = true;
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
            bitmapWriter.Write(POT_COL - 5, POT_PAGE + 2,
                               &bitmaps.PumpPipeTop, false,
                               _DrawHandler);
            return;

        case DrawState::POT_WALLS_PUMP2:
            bitmapWriter.Write(POT_COL - 5, POT_PAGE + 4,
                               &bitmaps.PumpPipeBottom, false,
                               _DrawHandler);
            return;

        case DrawState::PUMP:
            if (!(drawMask & DrawMask::M_PUMP)) {
                drawState = DrawState::DRAIN;
                break;
            }
            drawMask &= ~DrawMask::M_PUMP;
            bitmapWriter.Write(POT_COL - 8, POT_PAGE + 3,
                               pumpActive ? &bitmaps.PumpActive : &bitmaps.PumpInactive,
                               false, _DrawHandler);
            return;

        case DrawState::DRAIN:
            if (!(drawMask & DrawMask::M_DRAIN)) {
                drawState = DrawState::TOP_WATER;
                break;
            }
            drawMask &= ~DrawMask::M_DRAIN;
            if (drainActive) {
                bitmapWriter.Write(POT_COL + 17, POT_PAGE + 1,
                                   &bitmaps.SyphonDrain, false, _DrawHandler);
            } else {
                bitmapWriter.Clear(POT_COL + 17, POT_PAGE + 1,
                                   &bitmaps.SyphonDrain, false, _DrawHandler);
            }
            return;

        case DrawState::TOP_WATER:
            if (!(drawMask & DrawMask::M_TOP_WATER)) {
                drawState = DrawState::BOTTOM_WATER;
                break;
            }
            drawMask &= ~DrawMask::M_TOP_WATER;
            display.Output(Display::Viewport{POT_COL + 1, POT_COL + 15,
                                             POT_PAGE, POT_PAGE + 2},
                           _DisplayOutputHandler);
            return;

        case DrawState::BOTTOM_WATER:
            if (!(drawMask & DrawMask::M_BOTTOM_WATER)) {
                drawState = DrawState::DONE;
                break;
            }
            drawMask &= ~DrawMask::M_BOTTOM_WATER;
            display.Output(Display::Viewport{POT_COL + 1, POT_COL + 22,
                                             POT_PAGE + 3, POT_PAGE + 5},
                           _DisplayOutputHandler);
            return;

        default:
            break;
        }
    } while (drawMask && drawState < DrawState::DONE);
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
    drawState++;
    if (drawState != DrawState::DONE) {
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

/** Get byte which has numBits most significant bits set. */
static inline u8
GetFillBits(u8 numBits)
{
    u8 result = 0;
    while (numBits) {
        result = (result >> 1) | 0x80;
        numBits--;
    }
    return result;
}

bool
MainPage::DisplayOutputHandler(u8 column, u8 page, u8 *data)
{
    u8 _data = 0;
    if (drawState == DrawState::TOP_WATER) {

        if (page == POT_PAGE) {
            _data = 0b00000001;
            if (watLevelTop > 15) {
                _data |= GetFillBits(watLevelTop - 15);
            }
        } else if (page == POT_PAGE + 1) {
            if (watLevelTop >= 15) {
                _data = 0xff;
            } else if (watLevelTop > 7) {
                _data = GetFillBits(watLevelTop - 7);
            } else {
                _data = 0;
            }
        } else {
            if (watLevelTop >= 7) {
                _data = 0xff;
            } else {
                _data = 0x80 | (GetFillBits(watLevelTop) >> 1);
            }
        }

        if (column == POT_COL + 15 && page == POT_PAGE + 2) {
            DrawHandler();
        }

    } else if (drawState == DrawState::BOTTOM_WATER) {

        if (page == POT_PAGE + 3) {
            if (watLevelBottom > 15) {
                _data = GetFillBits(watLevelBottom - 15);
            } else {
                _data = 0;
            }
        } else if (page == POT_PAGE + 4) {
            if (watLevelBottom >= 15) {
                _data = 0xff;
            } else if (watLevelBottom > 7) {
                _data = GetFillBits(watLevelBottom - 7);
            } else {
                _data = 0;
            }
        } else {
            if (watLevelBottom >= 7) {
                _data = 0xff;
            } else {
                _data = 0x80 | (GetFillBits(watLevelBottom) >> 1);
            }
        }

        if (column == POT_COL + 22 && page == POT_PAGE + 5) {
            DrawHandler();
        }
    }
    *data = _data;
    return true;
}

bool
MainPage::_DisplayOutputHandler(u8 column, u8 page, u8 *data)
{
    return static_cast<MainPage *>(app.CurPage())->
        DisplayOutputHandler(column, page, data);
}
