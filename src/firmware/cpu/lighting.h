/* This file is a part of 'hydroponics' project.
 * Copyright (c) 2015, Artyom Lebedev <artyom.lebedev@gmail.com>
 * All rights reserved.
 * See LICENSE file for copyright details.
 */

/** @file lighting.h */

#ifndef LIGHTING_H_
#define LIGHTING_H_

class Light {
public:
    enum AdcChannel {
        SENSOR_A = 0,
        SENSOR_B = 1
    };

    void
    Enable();

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

    u8
    GetSensorA()
    {
        return curSensorA;
    }

    u8
    GetSensorB()
    {
        return curSensorB;
    }

    void
    OnAdcResult(u8 channel, u16 value);

private:
    enum {
        MEASUREMENT_PERIOD = TASK_DELAY_S(2)
    };

    u8 curLevel = 0;
    u8 curSensorA, curSensorB;

    static u16 PeriodicTask();

} __PACKED;

extern Light light;

#endif /* LIGHTING_H_ */
