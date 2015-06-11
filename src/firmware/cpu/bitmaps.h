/* This file is a part of 'hydroponics' project.
 * Copyright (c) 2012-2015, Artyom Lebedev <artyom.lebedev@gmail.com>
 * All rights reserved.
 * See COPYING file for copyright details.
 */

/** @file bitmaps.h
 * Definition for used bitmap images.
 */

#ifndef BITMAPS_H_
#define BITMAPS_H_

template<u8... data>
constexpr static u8
Bitmap_NumDataBytes()
{
    return sizeof...(data);
}

/** Define bitmap.
 * @param __name Name for accessing.
 * @param __numPages Number of pages in the bitmap. Number of columns defined as
 *      total number of data bytes divided by number of pages.
 * @param __VA_ARGS__ Data bytes.
 */
#define DEF_BITMAP(__name, __numPages, ...) \
    const u8 __CONCAT(__name, __data__) \
        [Bitmap_NumDataBytes<__VA_ARGS__>()] = { __VA_ARGS__ }; \
    const Bitmap __name { \
        reinterpret_cast<const u8 *>(OFFSETOF(Bitmaps, __CONCAT(__name, __data__))), \
        __numPages, \
        sizeof(__CONCAT(__name, __data__)) / __numPages};

/** Global bitmaps repository. Stored in program memory. */
class Bitmaps {
public:

    /** Thermometer icon. */
    DEF_BITMAP(Thermometer, 1,
        0b00010100,
        0b01111110,
        0b10000001,
        0b01111110,
        0b00010100
    )

    /** Sun icon. */
    DEF_BITMAP(Sun, 1,
        0b00100100,
        0b00011000,
        0b10100101,
        0b01000010,
        0b01000010,
        0b10100101,
        0b00011000,
        0b00100100
    )

    DEF_BITMAP(Test2, 1, 42, 45)
    DEF_BITMAP(Test3, 2, 42, 43, 44, 45)
} __PACKED;

extern const Bitmaps bitmaps PROGMEM;

#endif /* BITMAPS_H_ */
