/* This file is a part of 'hydroponics' project.
 * Copyright (c) 2012-2015, Artyom Lebedev <artyom.lebedev@gmail.com>
 * All rights reserved.
 * See COPYING file for copyright details.
 */

/** @file linear_value_selector.h
 * TODO insert description here.
 */

#ifndef LINEAR_VALUE_SELECTOR_H_
#define LINEAR_VALUE_SELECTOR_H_

/** Page for linear value selection. */
class LinearValueSelector: public Page {
public:
    enum {
        PRINTER_BUF_SIZE
    };

    /** Callback for converting value to string representation.
     *
     * @param value Raw value.
     * @param buf Character buffer for the result. Size is PRINTER_BUF_SIZE.
     */
    typedef void (*Printer)(u16 value, char *buf);

    LinearValueSelector(const char *title, u16 initialValue, u16 minValue,
                        u16 maxValue, Printer printer = nullptr);

};



#endif /* LINEAR_VALUE_SELECTOR_H_ */
