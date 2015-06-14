/* This file is a part of 'hydroponics' project.
 * Copyright (c) 2012-2015, Artyom Lebedev <artyom.lebedev@gmail.com>
 * All rights reserved.
 * See COPYING file for copyright details.
 */

/** @file level_gauge.h */

#ifndef LEVEL_GAUGE_H_
#define LEVEL_GAUGE_H_

class LevelGauge {
public:
    enum {
        INTERVAL = TASK_DELAY_MS(500)
    };

    LevelGauge();

    void
    Trigger();

    u16
    GetRawValue()
    {
        return accResult >> ROLL_AVG_BITS;
    }

    /** Enable periodic measurements. */
    void
    Enable();

    /** Disable periodic measurements. */
    void
    Disable();

    void
    Timer1Ovf();

    void
    Timer1Capt();
private:
    enum {
        /** Additional bits for rolling average calculation. */
        ROLL_AVG_BITS = 2
    };
    /** Accumulated result. */
    u32 accResult:16 + ROLL_AVG_BITS,
        enabled:1,
        inProgress:1,
    /* Echo front pulse detected. */
        echoStarted:1,
        overflowSeen:1,
        failure:1,
        reserved:9;
    u16 echoStartTime;

    static u16
    _PeriodicTask();

    u16
    PeriodicTask();

    void
    OnResult(u16 result);
} __PACKED;

extern LevelGauge lvlGauge;

#endif /* LEVEL_GAUGE_H_ */