/* This file is a part of 'hydroponics' project.
 * Copyright (c) 2012-2015, Artyom Lebedev <artyom.lebedev@gmail.com>
 * All rights reserved.
 * See COPYING file for copyright details.
 */

/** @file menu.h */

#ifndef MENU_H_
#define MENU_H_

class Menu: Page {
public:
    /** Additional null terminator after last item. */
    Menu(char *items, u8 pos = 0):
        items(items), isPgm(false), curItem(pos)
    {}

    /** Additional null terminator after last item. */
    Menu(const char *items, u8 pos = 0):
        items(items), isPgm(true), curItem(pos)
    {}

    /** Override to process selection event. */
    virtual void
    OnItemSelected(u8 idx __UNUSED)
    {}

    virtual void
    OnButtonPressed() override;

    virtual void
    OnRotEncClick(bool dir) override;

    virtual void
    Poll() override;

    virtual bool
    RequestClose() override;

private:
    enum {
        NUM_LINES = 8,
        LEFT_GAP = 5
    };

    enum DrawState {
        ITEMS
    };

    const char *items;

    u8 isPgm:1,
       drawInProgress:1,
       drawPending:1,
       closeRequested:1,
       drawState:2,
       reserved:2,

       curDrawItem:3;
    const char *curDrawItemText;

    u8 curItem, numItems, topItem;

    void
    Initialize();

    void
    Draw();

    void
    DrawHandler();

    static void
    _DrawHandler();

    const char *
    SkipItems(const char *p, u8 num);
};

#include "menus.h"

#endif /* MENU_H_ */
