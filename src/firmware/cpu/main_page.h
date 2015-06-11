/* This file is a part of 'hydroponics' project.
 * Copyright (c) 2012-2015, Artyom Lebedev <artyom.lebedev@gmail.com>
 * All rights reserved.
 * See COPYING file for copyright details.
 */

/** @file main_page.h */

#ifndef MAIN_PAGE_H_
#define MAIN_PAGE_H_

/** Main application screen. */
class MainPage: Page {
    virtual void
    OnButtonPressed() override;

    virtual void
    OnRotEncClick(bool dir) override;
};

#endif /* MAIN_PAGE_H_ */
