/* This file is a part of 'hydroponics' project.
 * Copyright (c) 2012-2015, Artyom Lebedev <artyom.lebedev@gmail.com>
 * All rights reserved.
 * See COPYING file for copyright details.
 */

/** @file flooder.h
 * TODO insert description here.
 */

#ifndef FLOODER_H_
#define FLOODER_H_

class Flooder {
public:
    enum Status {
        IDLE,
        FLOODING,
        FLOOD_FINAL,
        DRAINING,
        FAILURE
    };

    enum ErrorCode {
        /** Too low water level for flooding start. */
        LOW_WATER
    };

    Flooder();

    Status
    GetStatus()
    {
        return static_cast<Status>(status);
    }

    const char *
    GetStatusString();

    ErrorCode
    GetErrorCode()
    {
        return static_cast<ErrorCode>(errorCode);
    }

    const char *
    GetErrorString();

    void
    StartFlooding();

    static u8
    GetPumpThrottle();

    static void
    SetPumpThrottle(u8 value);

    static u8
    GetPumpBoostThrottle();

    static void
    SetPumpBoostThrottle(u8 value);

    /** Get heuristically calculated water level in the top pot. 255 is 100%. */
    u8
    GetTopPotWaterLevel();

private:
    enum {
        /** Minimal water level to start flooding, in percents. */
        MIN_START_WATER = 95,
        /** Control polling period. */
        POLL_PERIOD = TASK_DELAY_S(3),
    };
    u8 status:3,
       errorCode:3,
       :2;
    /** Water level when cycle started. */
    u8 startLevel = 0;
    /** Water level on most recent gauge reading. */
    u8 lastWaterLevel;
    /** Water level when siphon reached. */
    u8 siphonLevel = 0;

    /** Throttle value to use when pump is on. */
    static u8 EEMEM eePumpThrottle,
    /** Throttle value to use when pump is boosted on final run. */
                    eePumpBoostThrottle;

    static u16
    _FloodPoll();

    u16
    FloodPoll();

} __PACKED;

extern Flooder flooder;

#endif /* FLOODER_H_ */
