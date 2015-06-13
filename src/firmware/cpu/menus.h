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
        I_MANUAL_CONTROL,
        I_CALIBRATION,
        I_SETUP
    };

    MainMenu(u8 pos = 0):
        Menu(strings.MainMenu, pos)
    {}

    virtual void
    OnItemSelected(u8 idx) override;
};

class ManualControlMenu: Menu {
public:
    enum Idx {
        I_RETURN,
        I_LIGHT,
        I_PUMP
    };

    ManualControlMenu(u8 pos = 0):
        Menu(strings.ManualControlMenu, pos)
    {}

//    virtual void
//    OnItemSelected(u8 idx) override;
};

class CalibrationMenu: Menu {
public:
    enum Idx {
        I_RETURN,
        I_LIGHT,
        I_LEVEL_GAUGE,
        I_TEMPERATURE
    };

    CalibrationMenu(u8 pos = 0):
        Menu(strings.CalibrationMenu, pos)
    {}

//    virtual void
//    OnItemSelected(u8 idx) override;
};

class SetupMenu: Menu {
public:
    enum Idx {
        I_RETURN,
        I_TIME,
        I_FLOODING,
        I_LIGHTING
    };

    SetupMenu(u8 pos = 0):
        Menu(strings.SetupMenu, pos)
    {}

//    virtual void
//    OnItemSelected(u8 idx) override;
};

#endif /* MENUS_H_ */
