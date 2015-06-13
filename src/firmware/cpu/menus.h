/* This file is a part of 'hydroponics' project.
 * Copyright (c) 2012-2015, Artyom Lebedev <artyom.lebedev@gmail.com>
 * All rights reserved.
 * See COPYING file for copyright details.
 */

/** @file menus.h */

#ifndef MENUS_H_
#define MENUS_H_

namespace MainMenu {
    extern const Menu::Action actions[] PROGMEM;

    void
    Fabric(void *p);
};

namespace ManualControlMenu {
    extern const Menu::Action actions[] PROGMEM;

    void
    Fabric(void *p);
}

namespace CalibrationMenu {
    extern const Menu::Action actions[] PROGMEM;

    void
    Fabric(void *p);
}

namespace SetupMenu {
    extern const Menu::Action actions[] PROGMEM;

    void
    Fabric(void *p);
}

#endif /* MENUS_H_ */
