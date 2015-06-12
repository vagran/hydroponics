/* This file is a part of 'hydroponics' project.
 * Copyright (c) 2012-2015, Artyom Lebedev <artyom.lebedev@gmail.com>
 * All rights reserved.
 * See COPYING file for copyright details.
 */

/** @file application.h
 * TODO insert description here.
 */

#ifndef APPLICATION_H_
#define APPLICATION_H_

/** Base class for displayed page. */
class Page {
public:
    Page()
    {
        display.Clear();
    }

    /** Called in periodically polling loop. */
    virtual void
    Poll()
    {}

    virtual void
    OnButtonPressed()
    {}

    virtual void
    OnButtonLongPressed()
    {}

    /** Invoked when rotary encoder rotated on one click.
     *
     * @param dir CW direction when true, CCW when false.
     */
    virtual void
    OnRotEncClick(bool dir __UNUSED)
    {}

    /** Request page closing.
     *
     * @return True if page can be closed (destructed), false if not yet (e.g.
     *      asynchronous operation in progress).
     */
    virtual bool
    RequestClose()
    {
        return true;
    }
};

/* Pages. */
#include "main_page.h"
#include "menu.h"

/** Encapsulates high-level application logic. */
class Application {
public:
    enum Pages {
        MAIN,
        MAIN_MENU
    };

    Application();

    void
    Poll();

    void
    OnButtonPressed();

    void
    OnButtonLongPressed();

    /** Invoked when rotary encoder rotated on one click.
     *
     * @param dir CW direction when true, CCW when false.
     */
    void
    OnRotEncClick(bool dir);

    /** Set next page to display.
     *
     * @param page Page to set.
     * @param menuPos Menu position if menu page.
     */
    inline void
    SetNextPage(Pages page, u8 menuPos = 0)
    {
        nextPageCode = page;
        this->menuPos = menuPos;
    }

    inline Page *
    CurPage()
    {
        if (curPage.CurType()) {
            return reinterpret_cast<Page *>(curPage.GetPtr());
        }
        return nullptr;
    }

private:
    Variant<MainPage, MainMenu> curPage;

    /** Pages enum. */
    u8 curPageCode:4,
       nextPageCode:4,

       menuPos;

    void
    SetPage(Pages page);
};

extern Application app;

#endif /* APPLICATION_H_ */
