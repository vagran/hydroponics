/* This file is a part of 'hydroponics' project.
 * Copyright (c) 2012-2015, Artyom Lebedev <artyom.lebedev@gmail.com>
 * All rights reserved.
 * See COPYING file for copyright details.
 */

/** @file menus.h */

#ifndef MENUS_H_
#define MENUS_H_

class MainMenu: Menu {
public:
    enum Idx {
        I_RETURN,
        I_TEST1,
        I_TEST2
    };

    MainMenu(u8 pos = 0):
        Menu(strings.MainMenu, pos)
    {}
};

#endif /* MENUS_H_ */
