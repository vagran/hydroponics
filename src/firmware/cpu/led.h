/* This file is a part of 'hydroponics' project.
 * Copyright (c) 2015, Artyom Lebedev <artyom.lebedev@gmail.com>
 * All rights reserved.
 * See LICENSE file for copyright details.
 */

/** @file led.h */

#ifndef LED_H_
#define LED_H_

class Led {
public:
    enum class Mode {
        STANDBY,
        FAILURE
    };

    Led();

    void
    Initialize();

    void
    SetMode(Mode mode);

private:
    enum {
        ANIMATION_PERIOD = TASK_DELAY_MS(100)
    };

    enum Pattern {
        STANDBY = 0x0001,
        FAILURE = 0x00ff
    };

    /** Current pattern. */
    u16 pattern;
    /** Current mode. */
    u8 mode:4,
    /** Current position in current pattern. */
       patPos:4;

    static u16
    _Animator();

    u16
    Animator();
} __PACKED;

extern Led led;

#endif /* LED_H_ */
