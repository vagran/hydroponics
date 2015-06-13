/* This file is a part of 'hydroponics' project.
 * Copyright (c) 2012-2015, Artyom Lebedev <artyom.lebedev@gmail.com>
 * All rights reserved.
 * See COPYING file for copyright details.
 */

/** @file menu.h */

#ifndef MENU_H_
#define MENU_H_

/** Use as terminator in menu actions lists. */
#define MENU_ACTIONS_END {0xff, nullptr}

class Menu: public Page {
public:
    struct Action {
        /** Zero to ignore action (handle in overridden OnItemSelected() method. */
        u8 typeCode;
        VariantFabric fabric;
    } __PACKED;

    /** @param items Item strings. Additional null terminator after last item.
     *  @param pos Initially selected item index.
     *  @param actions Default page navigation actions. Size should be equal to
     *      number of items in "items" argument.
     *  @param returnAction Return action index if special handling needed, -1
     *      if not needed.
     */
    Menu(char *items, u8 pos = 0, const Action *actions = nullptr,
         i8 returnAction = -1):
        items(items), actions(actions), isPgm(false), curItem(pos),
        returnAction(returnAction)
    {
        Initialize();
    }

    /** Additional null terminator after last item. */
    Menu(const char *items, u8 pos = 0, const Action *actions = nullptr,
         i8 returnAction = -1):
        items(items), actions(actions), isPgm(true), curItem(pos),
        returnAction(returnAction)
    {
        Initialize();
    }

    /** Override to process selection event. */
    virtual void
    OnItemSelected(u8 idx);

    virtual void
    OnButtonPressed() override;

    virtual void
    OnRotEncClick(bool dir) override;

    virtual void
    Poll() override;

    virtual bool
    RequestClose() override;

    /** Find action with the specified fabric in the provided actions array.
     *
     * @param actions Actions array.
     * @param fabric Fabric handler to find.
     * @return Index of the found action, -1 if not found.
     */
    static i8
    FindAction(const Action *actions, VariantFabric fabric);

    /** Selected item when returning from submenu. */
    static u8 returnPos;

private:
    enum {
        NUM_LINES = 8,
        LEFT_GAP = 5
    };

    enum DrawState {
        ITEMS,
        UP_ICON,
        DOWN_ICON,
        DONE
    };

    const char *items;
    const Action *actions;

    u8 isPgm:1,
       drawInProgress:1,
       drawPending:1,
       closeRequested:1,
       drawState:2,
       reserved:2,

       curDrawItem:3;
    const char *curDrawItemText;

    u8 curItem, numItems, topItem;
    i8 returnAction;

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
} __PACKED;

#include "menus.h"

#endif /* MENU_H_ */
