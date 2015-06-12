/* This file is a part of 'hydroponics' project.
 * Copyright (c) 2012-2015, Artyom Lebedev <artyom.lebedev@gmail.com>
 * All rights reserved.
 * See COPYING file for copyright details.
 */

/** @file menu.cpp */

#include "cpu.h"

using namespace adk;

void
Menu::Initialize()
{
    drawInProgress = false;
    drawPending = false;
    closeRequested = false;

    numItems = 0;
    const char *p = items;
    bool prevNull = false, charSeen = false;
    while (true) {
        char c;
        if (isPgm) {
            c = pgm_read_byte(p);
        } else {
            c = *p;
        }
        p++;
        if (!c) {
            charSeen = false;
            if (prevNull) {
                break;
            } else {
                prevNull = true;
            }
        } else {
            prevNull = false;
            if (!charSeen) {
                numItems++;
                charSeen = true;
            }
        }
    }

    if (curItem >= numItems) {
        curItem = numItems - 1;
    }

    if (curItem >= NUM_LINES) {
        topItem = curItem - NUM_LINES / 2;
        if (topItem > numItems - NUM_LINES) {
            topItem = numItems - NUM_LINES;
        }
    } else {
        topItem = 0;
    }

    Draw();
}

const char *
Menu::SkipItems(const char *p, u8 num)
{
    bool prevNull = false, charSeen = false;
    while (num) {
        char c;
        if (isPgm) {
            c = pgm_read_byte(p);
        } else {
            c = *p;
        }
        p++;
        if (!c) {
            charSeen = false;
            if (prevNull) {
                break;
            } else {
                prevNull = true;
                num--;
            }
        } else {
            prevNull = false;
            if (!charSeen) {
                numItems++;
                charSeen = true;
            }
        }
    }
    return p;
}

void
Menu::Draw()
{
    AtomicSection as;
    if (closeRequested) {
        return;
    }
    if (drawInProgress) {
        drawPending = true;
        return;
    }
    drawInProgress = true;
    drawState = DrawState::ITEMS;
    curDrawItem = 0;
    curDrawItemText = SkipItems(items, topItem);
    bool inv = topItem + curDrawItem == curItem;
    if (isPgm) {
        textWriter.Write(Display::Viewport {LEFT_GAP, 127, 0, 0},
                         curDrawItemText, inv, _DrawHandler);
    } else {
        textWriter.Write(Display::Viewport {LEFT_GAP, 127, 0, 0},
                         const_cast<char *>(curDrawItemText), inv, _DrawHandler);
    }
}

void
Menu::DrawHandler()
{
    AtomicSection as;

    //XXX

    drawInProgress = false;
}

void
Menu::_DrawHandler()
{
    static_cast<Menu *>(app.CurPage())->DrawHandler();
}

void
Menu::OnButtonPressed()
{

}

void
Menu::OnRotEncClick(bool dir __UNUSED/*XXX*/)
{

}

void
Menu::Poll()
{
    AtomicSection as;
    if (drawPending && !drawInProgress) {
        drawPending = false;
        Draw();
    }
}

bool
Menu::RequestClose()
{
    AtomicSection as;
    closeRequested = true;
    return !drawInProgress;
}
