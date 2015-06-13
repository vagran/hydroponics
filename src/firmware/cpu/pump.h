/* This file is a part of 'hydroponics' project.
 * Copyright (c) 2012-2015, Artyom Lebedev <artyom.lebedev@gmail.com>
 * All rights reserved.
 * See COPYING file for copyright details.
 */

/** @file pump.h */

#ifndef PUMP_H_
#define PUMP_H_

class Pump {
public:
    void
    SetLevel(u8 lvl)
    {
        curLevel = lvl;
        Pwm2Set(lvl);
    }

    u8
    GetLevel()
    {
        return curLevel;
    }

private:
    u8 curLevel = 0;

};

extern Pump pump;

#endif /* PUMP_H_ */
