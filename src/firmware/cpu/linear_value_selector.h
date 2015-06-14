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
        PRINTER_BUF_SIZE = 16
    };

    /** Callback for converting value to string representation.
     *
     * @param value Raw value.
     * @param buf Character buffer for the result. Size is PRINTER_BUF_SIZE.
     */
    typedef void (*Printer)(u16 value, char *buf);

    /** Handler for value change events. */
    typedef void (*Handler)(u16 value);

    Printer printer = nullptr;
    Handler onChanged = nullptr;
    Handler onClosed = nullptr;

    LinearValueSelector(const char *title, u16 initialValue, u16 minValue,
                        u16 maxValue, bool readOnly = false);

    u16
    GetValue()
    {
        return value;
    }

    void
    SetValue(u16 value);

    /** Set hint value to display under the gauge. */
    void
    SetHint(u16 hintValue);

    void
    UnsetHint();

    /** Format value to string to show on the page. */
    virtual void
    Print(u16 value, char *buf);

    virtual void
    OnChanged(u16 value);

    virtual void
    OnClosed(u16 value);


    virtual void
    OnButtonPressed() override;

    virtual void
    OnButtonLongPressed() override;

    virtual void
    OnRotEncClick(bool dir) override;

    virtual void
    Poll() override;

    virtual bool
    RequestClose() override;

private:
    enum DrawState {
        FULL_DRAW,
        TITLE = FULL_DRAW,
        FINE,
        HINT_CLEAR,
        HINT,

        PARTIAL_DRAW,
        GAUGE = PARTIAL_DRAW,
        VALUE,
        PERCENTS,

        LAST = PERCENTS
    };
    const char *title;
    u16 value, minValue, maxValue, hintValue, oldHintValue;
    char buf[PRINTER_BUF_SIZE];

    u8 drawInProgress:1,
       drawPending:1,
       fullDrawPending:1,
       closeRequested:1,
       drawState:3,
       fineInc:1,

       gaugeLen:7,
       readOnly:1,

       hintSet:1,
       hintUpdated:1,
       reserved:7;

    void
    Draw(bool full = false);

    /** Issue draw request depending on current draw state. */
    void
    IssueDrawRequest();

    void
    DrawHandler();

    static void
    _DrawHandler();

    bool
    GaugeDrawHandler(u8 column, u8 page, u8 *data);

    static bool
    _GaugeDrawHandler(u8 column, u8 page, u8 *data);

    inline u8
    GetGaugeLen(u16 value)
    {
        return static_cast<u32>(value - minValue) * 126 / (maxValue - minValue);
    }

} __PACKED;



#endif /* LINEAR_VALUE_SELECTOR_H_ */
