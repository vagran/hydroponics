/* This file is a part of 'hydroponics' project.
 * Copyright (c) 2012-2015, Artyom Lebedev <artyom.lebedev@gmail.com>
 * All rights reserved.
 * See COPYING file for copyright details.
 */

/** @file lighting.h */

#ifndef LIGHTING_H_
#define LIGHTING_H_

class Light {
public:
    void
    SetLevel(u8 lvl)
    {
        curLevel = lvl;
        Pwm3Set(lvl);
    }

    u8
    GetLevel()
    {
        return curLevel;
    }

private:
    u8 curLevel = 0;

};

extern Light light;

#endif /* LIGHTING_H_ */
