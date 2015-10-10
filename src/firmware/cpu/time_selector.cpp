/* This file is a part of 'hydroponics' project.
 * Copyright (c) 2015, Artyom Lebedev <artyom.lebedev@gmail.com>
 * All rights reserved.
 * See LICENSE file for copyright details.
 */

/** @file time_selector.cpp */

#include "cpu.h"

using namespace adk;

TimeSelector::TimeSelector(const char *title, Time initialValue):
    title(title), value(initialValue)
{
    drawInProgress = false;
    drawPending = false;
    closeRequested = false;
    selection = Selection::SEL_HOUR;
    Draw(true);
}

void
TimeSelector::Draw(bool full)
{
    AtomicSection as;
    if (closeRequested) {
        return;
    }
    if (drawInProgress) {
        drawPending = true;
        fullDrawPending = full;
        return;
    }
    drawInProgress = true;
    drawState = full ? DrawState::FULL_DRAW : DrawState::PARTIAL_DRAW;
    IssueDrawRequest();
}

void
TimeSelector::IssueDrawRequest()
{
    switch (drawState) {
    case DrawState::TITLE:
        textWriter.Write(Display::Viewport{0, 127, 0, 1}, title, false, false,
                         _DrawHandler);
        break;
    case DrawState::CANCEL: {
        u8 x1 = 10;
        u8 x2 = x1 + (FONT_WIDTH + 1) * 6;
        textWriter.Write(Display::Viewport{x1, x2, 6, 6},
                         strings.Cancel, selection == SEL_CANCEL, false,
                         _DrawHandler);
        break;
    }
    case DrawState::OK: {
        u8 x1 = 10 + (FONT_WIDTH + 1) * 6 + 20;
        u8 x2 = x1 + (FONT_WIDTH + 1) * 2;
        textWriter.Write(Display::Viewport{x1, x2, 6, 6},
                         strings.OK, selection == SEL_OK, false, _DrawHandler);
        break;
    }
    case DrawState::SEP: {
        u8 x1 = CLOCK_COL + (FONT_WIDTH + 1) * 2;
        u8 x2 = x1 + FONT_WIDTH + 1;
        buf[0] = ':';
        buf[1] = 0;
        textWriter.Write(Display::Viewport{x1, x2, 3, 3}, buf, false, false,
                         _DrawHandler);
        break;
    }
    case DrawState::HOUR:
        Strings::StrClockNum(value.hour, buf);
        textWriter.Write(
            Display::Viewport{CLOCK_COL, CLOCK_COL + (FONT_WIDTH + 1) * 2, 3, 3},
            buf, selection == SEL_HOUR, false, _DrawHandler);
        break;
    case DrawState::MIN: {
        Strings::StrClockNum(value.min, buf);
        u8 x1 = CLOCK_COL + (FONT_WIDTH + 1) * 3;
        u8 x2 = x1 + (FONT_WIDTH + 1) * 2;
        textWriter.Write(Display::Viewport{x1, x2, 3, 3}, buf,
            selection == SEL_MIN, false, _DrawHandler);
        break;
    }
    }
}

void
TimeSelector::DrawHandler()
{
    AtomicSection as;

    if (closeRequested ||
        (drawPending && (fullDrawPending || drawState >= DrawState::PARTIAL_DRAW))) {

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
TimeSelector::_DrawHandler()
{
    static_cast<TimeSelector *>(app.CurPage())->DrawHandler();
}

void
TimeSelector::OnButtonPressed()
{
    switch (selection) {
    case SEL_HOUR:
        selection = SEL_MIN;
        Draw();
        break;
    case SEL_MIN:
        selection = SEL_CANCEL;
        Draw(true);
        break;
    case SEL_CANCEL:
        if (onClosed) {
            onClosed(false);
        }
        break;
    case SEL_OK:
        if (onClosed) {
            onClosed(true);
        }
        break;
    }
}

void
TimeSelector::OnRotEncClick(bool dir)
{
    switch (selection) {
    case SEL_HOUR:
        if (dir) {
            value.hour++;
            if (value.hour > 23) {
                value.hour = 0;
            }
        } else {
            if (value.hour == 0) {
                value.hour = 23;
            } else {
                value.hour--;
            }
        }
        Draw();
        break;
    case SEL_MIN:
        if (dir) {
            value.min++;
            if (value.min > 59) {
                value.min = 0;
            }
        } else {
            if (value.min == 0) {
                value.min = 59;
            } else {
                value.min--;
            }
        }
        Draw();
        break;
    case SEL_CANCEL:
        if (dir) {
            selection = SEL_OK;
        } else {
            selection = SEL_MIN;
        }
        Draw(true);
        break;
    case SEL_OK:
        if (dir) {
            selection = SEL_HOUR;
        } else {
            selection = SEL_CANCEL;
        }
        Draw(true);
        break;
    }
}
