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
        POT_PAGE = 1
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
        LAST = PUMP
    };

    enum DrawMask {
        M_STATIC =     0x0001,
        M_PUMP =       0x0002,

        M_ALL = M_STATIC | M_PUMP
    };

    u8 drawState:5,
       drawInProgress:1,
       closeRequested:1,
       pumpActive:1;

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
