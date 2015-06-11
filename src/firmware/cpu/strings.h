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

    /* Menus */
    DEF_STR(MainMenu,  "Return\0"
                       "Test1\0"
                       "Test2\0"
                       "Test3\0"
                       "Test4\0"
                       "Test5\0"
                       "Test6\0"
                       "Test7\0"
                       "Test8\0"
                       "Test9\0"
                       "Test10\0"
                       "Test11\0"
                       "Test12\0")
} __PACKED;

extern const Strings strings PROGMEM;

#endif /* STRINGS_H_ */
