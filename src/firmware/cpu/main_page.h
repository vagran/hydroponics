/* This file is a part of 'hydroponics' project.
 * Copyright (c) 2012-2015, Artyom Lebedev <artyom.lebedev@gmail.com>
 * All rights reserved.
 * See LICENSE file for copyright details.
 */

/** @file main_page.h */

#ifndef MAIN_PAGE_H_
#define MAIN_PAGE_H_

/** Main application screen. */
class MainPage: public Page {
public:
    static void
    Fabric(void *p);

    MainPage();

    /** Set status line text. String read from RAM. */
    void
    SetStatus(char *status)
    {
        SetStatus(status, false);
    }

    /** Set status line text. String read from program memory. */
    void
    SetStatus(const char *status)
    {
        SetStatus(status, true);
    }


    virtual void
    OnButtonPressed() override;

    virtual void
    OnRotEncClick(bool dir) override;

    virtual void
    Poll() override;

    virtual bool
    RequestClose() override;

private:
    enum {
        POT_COL = 104,
        POT_PAGE = 1,
        MAX_WATER_LEVEL = 21,
        ANIMATION_PERIOD = TASK_DELAY_MS(500)
    };

    enum DrawState {
        POT_WALLS_1,
        POT_WALLS_2,
        POT_WALLS_3,
        POT_WALLS_4,
        POT_WALLS_5,
        POT_WALLS_PUMP1,
        POT_WALLS_PUMP2,
        SUNRISE_ICON,
        SUNSET_ICON,
        TEMP_ICON,

        PUMP,
        DRAIN,
        TOP_WATER,
        BOTTOM_WATER,
        STATUS,
        DAYLIGHT,
        CLOCK,
        SUNRISE,
        SUNSET,
        TEMPERATURE,
        FLOOD_TIME,
        DONE
    };

    enum DrawMask {
        M_STATIC =          0x0001,
        M_PUMP =            0x0002,
        M_DRAIN =           0x0004,
        M_TOP_WATER =       0x0008,
        M_BOTTOM_WATER =    0x0010,
        M_STATUS =          0x0020,
        M_CLOCK =           0x0040,
        M_TEMPERATURE =     0x0080,
        M_DAYLIGHT =        0x0100,
        M_SUNRISE =         0x0200,
        M_SUNSET =          0x0400,
        M_FLOOD_TIME =      0x0800,

        M_ALL = M_STATIC | M_PUMP | M_DRAIN | M_TOP_WATER | M_BOTTOM_WATER |
                M_STATUS | M_CLOCK | M_TEMPERATURE | M_DAYLIGHT | M_SUNRISE |
                M_SUNSET | M_FLOOD_TIME
    };

    u8 drawState:5,
       drawInProgress:1,
       closeRequested:1,
       pumpActive:1,

       watLevelTop:5,
       drainActive:1,
       isStatusPgm:1,
       /** Current scrolling direction of long status text. */
       statusScrollDir:1,

       watLevelBottom:5,
       statusPause:1,
       isDaylight:1,
       /** Minute counter for once per minute refreshes. */
       lastMinute:1;

    u16 drawMask;
    const char *status = nullptr;
    /** Length of status string. */
    u8 statusLen = 0,
    /** Current scrolling offset of long status string. */
       statusOffset;

    /** Used to divide animation frequency, continuously incremented. */
    u8 animationDivider;

    u8 flooderStatus:3,
       flooderError:3,
       :2;

    char textBuf[18];

    void
    Draw(u16 mask);

    void
    DrawHandler();

    static void
    _DrawHandler();

    bool
    DisplayOutputHandler(u8 column, u8 page, u8 *data);

    static bool
    _DisplayOutputHandler(u8 column, u8 page, u8 *data);

    /** Issue draw request depending on current draw state. Promote current
     * draw state if necessary.
     */
    void
    IssueDrawRequest();

    void
    SetStatus(const char *status, bool isPgm);

    void
    CheckFlooderStatus();

    static u16
    _AnimationTask();

    u16
    AnimationTask();

    void
    GetClockText();

    void
    GetTimeText(Time t);

    void
    GetTemperatureText();

} __PACKED;

#endif /* MAIN_PAGE_H_ */
