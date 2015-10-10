/* This file is a part of 'hydroponics' project.
 * Copyright (c) 2012-2015, Artyom Lebedev <artyom.lebedev@gmail.com>
 * All rights reserved.
 * See LICENSE file for copyright details.
 */

/** @file time_selector.h
 * TODO insert description here.
 */

#ifndef TIME_SELECTOR_H_
#define TIME_SELECTOR_H_

class TimeSelector: public Page {
public:
    /** Handler for page closing. */
    typedef void (*CloseHandler)(bool accepted);

    /** Invoked when new value accepted. */
    CloseHandler onClosed;

    TimeSelector(const char *title, Time initialValue);

    /** Get current time value. */
    Time
    GetValue()
    {
        return value;
    }

    void
    OnButtonPressed() override;

    /** Invoked when rotary encoder rotated on one click.
     *
     * @param dir CW direction when true, CCW when false.
     */
    void
    OnRotEncClick(bool dir) override;

private:
    enum {
        CLOCK_COL = 48
    };

    enum DrawState {
        FULL_DRAW,
        TITLE = FULL_DRAW,
        CANCEL,
        OK,
        SEP,

        PARTIAL_DRAW,
        HOUR = PARTIAL_DRAW,
        MIN,

        LAST = MIN
    };

    enum Selection {
        SEL_HOUR,
        SEL_MIN,
        SEL_CANCEL,
        SEL_OK
    };

    const char *title;
    Time value;
    char buf[4];

    u8 drawInProgress:1,
       drawPending:1,
       fullDrawPending:1,
       closeRequested:1,
       drawState:3,
       :1,

       selection:2,
       :6;

    void
    Draw(bool full = false);

    /** Issue draw request depending on current draw state. */
    void
    IssueDrawRequest();

    void
    DrawHandler();

    static void
    _DrawHandler();

} __PACKED;

#endif /* TIME_SELECTOR_H_ */
