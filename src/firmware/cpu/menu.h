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
    Menu(char *items):
        items(items), isPgm(false)
    {}

    /** Additional null terminator after last item. */
    Menu(const char *items):
        items(items), isPgm(true)
    {}

private:
    const char *items;
    bool isPgm;
};

#include "menus.h"

#endif /* MENU_H_ */
