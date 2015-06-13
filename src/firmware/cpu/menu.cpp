/* This file is a part of 'hydroponics' project.
 * Copyright (c) 2012-2015, Artyom Lebedev <artyom.lebedev@gmail.com>
 * All rights reserved.
 * See COPYING file for copyright details.
 */

/** @file menu.cpp */

#include "cpu.h"

using namespace adk;

u8 Menu::returnPos;

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
    bool isSel = topItem + curDrawItem == curItem;
    if (isPgm) {
        textWriter.Write(Display::Viewport {LEFT_GAP, 127, 0, 0},
                         curDrawItemText, isSel, true, _DrawHandler);
    } else {
        textWriter.Write(Display::Viewport {LEFT_GAP, 127, 0, 0},
                         const_cast<char *>(curDrawItemText), isSel, true,
                         _DrawHandler);
    }
}

void
Menu::DrawHandler()
{
    AtomicSection as;

    if (closeRequested || drawPending) {
        drawInProgress = false;
        return;
    }

    while (drawState == DrawState::ITEMS) {
        if (curDrawItem >= NUM_LINES - 1 ||
            curDrawItem + topItem >= numItems - 1) {

            drawState = DrawState::UP_ICON;
            break;
        }

        curDrawItem++;
        curDrawItemText = SkipItems(curDrawItemText, 1);
        bool isSel = topItem + curDrawItem == curItem;
        if (isPgm) {
            textWriter.Write(Display::Viewport {LEFT_GAP, 127, curDrawItem, curDrawItem},
                             curDrawItemText, isSel, true, _DrawHandler);
        } else {
            textWriter.Write(Display::Viewport {LEFT_GAP, 127, curDrawItem, curDrawItem},
                             const_cast<char *>(curDrawItemText), isSel, true,
                             _DrawHandler);
        }
        return;
    }

    while (drawState == DrawState::UP_ICON) {
        drawState = DrawState::DOWN_ICON;
        if (topItem == 0) {
            bitmapWriter.Clear(0, 0, &bitmaps.Up, false, _DrawHandler);
        } else {
            bitmapWriter.Write(0, 0, &bitmaps.Up, false, _DrawHandler);
        }
        return;
    }

    while (drawState == DrawState::DOWN_ICON) {
        drawState = DrawState::DONE;
        if (numItems - topItem <= NUM_LINES) {
            bitmapWriter.Clear(0, 7, &bitmaps.Down, false, _DrawHandler);
        } else {
            bitmapWriter.Write(0, 7, &bitmaps.Down, false, _DrawHandler);
        }
        return;
    }

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
    if (!closeRequested) {
        OnItemSelected(curItem);
    }
}

void
Menu::OnRotEncClick(bool dir)
{
    AtomicSection as;
    if (dir) {
        if (curItem < numItems - 1) {
            curItem++;
        }
    } else if (curItem > 0) {
        curItem--;
    }
    if (topItem < curItem && curItem - topItem >= NUM_LINES) {
        topItem = curItem + 1 - NUM_LINES;
    } else if (topItem > curItem) {
        topItem = curItem;
    }
    Draw();
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

void
Menu::OnItemSelected(u8 idx)
{
    if (!actions) {
        return;
    }
    Action a;
    memcpy_P(&a, actions + idx, sizeof(a));
    if (!a.typeCode) {
        return;
    }
    if (returnAction != -1 && idx != returnAction) {
        returnPos = 0;
    }
    app.SetNextPage(a.typeCode, a.fabric);
}

i8
Menu::FindAction(const Action *actions, u8 numActions, VariantFabric fabric)
{
    if (!actions) {
        return -1;
    }
    i8 idx = 0;
    const Action *p = actions;
    while (numActions) {
        Action a;
        memcpy_P(&a, p, sizeof(a));
        if (a.fabric == fabric) {
            return idx;
        }
        p++;
        idx++;
    }
    return -1;
}
