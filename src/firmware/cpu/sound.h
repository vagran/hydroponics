/* This file is a part of 'hydroponics' project.
 * Copyright (c) 2012-2015, Artyom Lebedev <artyom.lebedev@gmail.com>
 * All rights reserved.
 * See LICENSE file for copyright details.
 */

/** @file sound.h */

#ifndef SOUND_H_
#define SOUND_H_

class Sound {
public:
    Sound();

    void
    Initialize();

    /** Set pattern to play.
     *
     * @param pattern Scanned from LSB to MSB.
     * @param repeat Play pattern one time if false, repeat continuously if true.
     */
    void
    SetPattern(u16 pattern, bool repeat = true);

private:
    enum {
        ANIMATION_PERIOD = TASK_DELAY_MS(100)
    };

    /** Current pattern. */
    u16 pattern;
    /** Current position in pattern. */
    u8 patPos:4,
    /** Is the pattern scan currently running. */
       running:1,
    /** Is the beeper currently on. */
       isActive:1,
    /** Repeat current pattern. */
       repeat:1,
       :1;

    static u16
    _Animator();

    u16
    Animator();
} __PACKED;

extern Sound sound;

#endif /* SOUND_H_ */
