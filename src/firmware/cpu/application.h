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
    typedef void (*PollHandler)();

    PollHandler poll = nullptr;

    Page()
    {
        display.Clear();
    }

    /** Called in periodically polling loop. */
    virtual void
    Poll();

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
#include "linear_value_selector.h"
#include "main_page.h"
#include "pages.h"
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

    /** Set next page to display.
     *
     * @param pageTypeCode Code for the page type, obtained by GetPageTypeCode().
     * @param page Page to set.
     * @param menuPos Menu position if menu page.
     */
    void
    SetNextPage(u8 pageTypeCode, VariantFabric page);

    inline Page *
    CurPage()
    {
        if (curPage.CurType()) {
            return reinterpret_cast<Page *>(curPage.GetPtr());
        }
        return nullptr;
    }

    /** Get type code for the specified page class. */
    template <class TPage>
    static constexpr u8
    GetPageTypeCode()
    {
        return decltype(curPage)::GetTypeCode<TPage>();
    }

private:
    Variant<MainPage,
            Menu,
            LinearValueSelector> curPage;

    u8 nextPageTypeCode: 5,
       reserved:3;
    /** Next page fabric if next page pending. */
    VariantFabric nextPage = nullptr;

    void
    SetPage(VariantFabric page);
};

extern Application app;

#endif /* APPLICATION_H_ */
