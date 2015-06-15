/* This file is a part of 'hydroponics' project.
 * Copyright (c) 2012-2015, Artyom Lebedev <artyom.lebedev@gmail.com>
 * All rights reserved.
 * See COPYING file for copyright details.
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
        MAX_WATER_LEVEL = 21
    };

    enum DrawState {
        POT_WALLS_1,
        POT_WALLS_2,
        POT_WALLS_3,
        POT_WALLS_4,
        POT_WALLS_5,
        POT_WALLS_PUMP1,
        POT_WALLS_PUMP2,

        PUMP,
        DRAIN,
        TOP_WATER,
        BOTTOM_WATER,
        DONE
    };

    enum DrawMask {
        M_STATIC =          0x0001,
        M_PUMP =            0x0002,
        M_DRAIN =           0x0004,
        M_TOP_WATER =       0x0008,
        M_BOTTOM_WATER =    0x0010,

        M_ALL = M_STATIC | M_PUMP | M_DRAIN | M_TOP_WATER | M_BOTTOM_WATER
    };

    u8 drawState:5,
       drawInProgress:1,
       closeRequested:1,
       pumpActive:1,

       watLevelTop:5,
       drainActive:1,
       reserved:2,

       watLevelBottom:5,
       reserved2:3;

    u16 drawMask;

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

} __PACKED;

#endif /* MAIN_PAGE_H_ */
