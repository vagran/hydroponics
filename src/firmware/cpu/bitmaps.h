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
        0b01101010,
        0b10011110,
        0b10000001,
        0b10011110,
        0b01101010
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

    /** Menu up icon. */
    DEF_BITMAP(Up, 1,
        0b00010000,
        0b00011000,
        0b00011100,
        0b00011000,
        0b00010000
    )

    /** Menu down icon. */
    DEF_BITMAP(Down, 1,
        0b00010000,
        0b00110000,
        0b01110000,
        0b00110000,
        0b00010000
    )

    /** Menu down icon. */
    DEF_BITMAP(LinearValueSelectorHint, 1,
        0b01110000,
        0b01111111,
        0b01110000
    )

    DEF_BITMAP(PotWall, 6,
        0b11111111,
        0b11111111,
        0b11111111,
        0b11111111,
        0b11111111,
        0b11111111
    )

    DEF_BITMAP(SiphonTop, 1,
        0b11111101,
        0b00000101,
        0b00000101,
        0b00000101,
        0b00000101,
        0b00000101,
        0b00000101
    )

    DEF_BITMAP(SiphonBottom, 1,
        0b11111111,
        0b10000000,
        0b10000000,
        0b10000000,
        0b10000000,
        0b10000000,
        0b10000000
    )

    DEF_BITMAP(SiphonWall, 1,
        0b11111111
    )

    DEF_BITMAP(SyphonDrain, 1,
        0b00100000,
        0b01100000,
        0b11111111,
        0b11111111,
        0b01100000,
        0b00100000
    )

    DEF_BITMAP(PumpPipeTop, 1,
        0b11111111,
        0b00000001,
        0b00000001,
        0b00000001
    )

    DEF_BITMAP(PumpPipeBottom, 1,
        0b11111111,
        0b10000000,
        0b10000000,
        0b10000000
    )

    DEF_BITMAP(PumpActive, 1,
        0b00111100,
        0b01110010,
        0b10111101,
        0b10111111,
        0b10111111,
        0b10111101,
        0b01110010,
        0b00111100
    )

    DEF_BITMAP(PumpInactive, 1,
        0b00111100,
        0b01110010,
        0b10101101,
        0b10100011,
        0b10100011,
        0b10101101,
        0b01110010,
        0b00111100
    )
} __PACKED;

extern const Bitmaps bitmaps PROGMEM;

#endif /* BITMAPS_H_ */
