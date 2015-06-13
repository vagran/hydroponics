/* This file is a part of 'hydroponics' project.
 * Copyright (c) 2012-2015, Artyom Lebedev <artyom.lebedev@gmail.com>
 * All rights reserved.
 * See COPYING file for copyright details.
 */

/** @file strings.h
 * Strings to place in program memory (flash).
 */

#ifndef STRINGS_H_
#define STRINGS_H_

/** Define string for placing in program memory. */
#define DEF_STR(__name, __text) \
    const char __name[sizeof(__text)] = __text;

class Strings {
public:
    DEF_STR(Test, "*+Iacebdpqghoswyz1234567890")
    DEF_STR(Test2, "Test string")

    DEF_STR(Fine, " Fine ")
    DEF_STR(LightControl, "Light control")
    DEF_STR(PumpControl, "Pump control")

    /* Menus */
    DEF_STR(MainMenu,
            "Return\0"
            "Manual control\0"
            "Calibration\0"
            "Setup\0")

    DEF_STR(ManualControlMenu,
            "Return\0"
            "Light\0"
            "Pump\0")

    DEF_STR(CalibrationMenu,
            "Return\0"
            "Light\0"
            "Level gauge\0"
            "Temperature\0")

    DEF_STR(SetupMenu,
            "Return\0"
            "Time\0"
            "Flooding\0"
            "Lighting\0")

} __PACKED;

extern const Strings strings PROGMEM;

#endif /* STRINGS_H_ */
