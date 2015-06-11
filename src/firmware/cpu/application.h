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
};

/* Pages. */
#include "main_page.h"
#include "menu.h"

/** Encapsulates high-level application logic. */
class Application {
public:
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

private:
    Variant<MainPage, Menu> curPage;

    inline Page *
    CurPage()
    {
        if (curPage.CurType()) {
            return reinterpret_cast<Page *>(curPage.GetPtr());
        }
        return nullptr;
    }
};

extern Application app;

#endif /* APPLICATION_H_ */
