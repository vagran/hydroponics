/* This file is a part of 'hydroponics' project.
 * Copyright (c) 2012-2015, Artyom Lebedev <artyom.lebedev@gmail.com>
 * All rights reserved.
 * See COPYING file for copyright details.
 */

/** @file linear_value_selector.cpp */

#include "cpu.h"

using namespace adk;

LinearValueSelector::LinearValueSelector(const char *title,
    u16 initialValue, u16 minValue, u16 maxValue):
    title(title), value(initialValue), minValue(minValue), maxValue(maxValue)
{
    drawInProgress = false;
    drawPending = false;
    closeRequested = false;
    fineInc = false;
    Draw(true);
}

void
LinearValueSelector::Print(u16 value, char *buf)
{
    if (printer) {
        printer(value, buf);
    } else {
        utoa(value, buf, 10);
    }
}

void
LinearValueSelector::OnChanged(u16 value)
{
    if (onChanged) {
        onChanged(value);
    }
}

void
LinearValueSelector::OnClosed(u16 value)
{
    if (onClosed) {
        onClosed(value);
    }
}

void
LinearValueSelector::OnButtonPressed()
{
    OnClosed(value);
}

void
LinearValueSelector::OnButtonLongPressed()
{
    AtomicSection as;
    fineInc = !fineInc;
    Draw(true);
}

void
LinearValueSelector::OnRotEncClick(bool dir)
{
    AtomicSection as;
    bool changed = false;
    u16 inc = (maxValue - minValue) / 100;
    if (inc == 0) {
        inc = 1;
    }
    if (dir) {
        if (value < maxValue) {
            if (fineInc) {
                value++;
            } else if (maxValue - value > inc) {
                value += inc;
            } else {
                value = maxValue;
            }
            changed = true;
        }
    } else if (value > minValue) {
        if (fineInc) {
            value--;
        } else if (value - minValue > inc) {
            value -= inc;
        } else {
            value = minValue;
        }
        changed = true;
    }
    if (changed) {
        OnChanged(value);
    }
    Draw();
}

void
LinearValueSelector::Poll()
{
    AtomicSection as;
    if (drawPending && !drawInProgress) {
        drawPending = false;
        Draw(fullDrawPending);
    }
}

bool
LinearValueSelector::RequestClose()
{
    AtomicSection as;
    closeRequested = true;
    return !drawInProgress;
}

void
LinearValueSelector::Draw(bool full)
{
    AtomicSection as;
    if (closeRequested) {
        return;
    }
    if (drawInProgress) {
        drawPending = true;
        fullDrawPending = full;
        return;
    }
    drawInProgress = true;
    drawState = full ? DrawState::FULL_DRAW : DrawState::PARTIAL_DRAW;
    IssueDrawRequest();
}

void
LinearValueSelector::IssueDrawRequest()
{
    switch (drawState) {
    case DrawState::TITLE:
        textWriter.Write(Display::Viewport{0, 127, 0, 1}, title, false, false,
                         _DrawHandler);
        break;
    case DrawState::FINE:
        if (fineInc) {
            textWriter.Write(Display::Viewport{0, 127, 7, 7}, strings.Fine,
                             true, false, _DrawHandler);
        } else {
            textWriter.Clear(Display::Viewport{0, 127, 7, 7}, strings.Fine,
                             false, false, _DrawHandler);
        }
        break;
    case DrawState::GAUGE:
        gaugeLen = static_cast<u32>(value - minValue) * 126 /
                   (maxValue - minValue);
        display.Output(Display::Viewport{0, 127, 4, 4}, _GaugeDrawHandler);
        break;
    case DrawState::VALUE:
        Print(value, buf);
        textWriter.Write(Display::Viewport{0, 127, 2, 3}, buf,
                         false, true, _DrawHandler);
        break;
    case DrawState::PERCENTS:
        u8 prc = static_cast<u32>(value - minValue) * 100 /
            (maxValue - minValue);
        utoa(prc, buf, 10);
        u8 len = strlen(buf);
        buf[len] = '%';
        buf[len + 1] = 0;
        textWriter.Write(Display::Viewport{56, 88, 6, 6}, buf,
                         false, true, _DrawHandler);
        break;
    }
}

void
LinearValueSelector::DrawHandler()
{
    AtomicSection as;

    if (closeRequested ||
        (drawPending && (fullDrawPending || drawState >= DrawState::PARTIAL_DRAW))) {

        drawInProgress = false;
        return;
    }

    if (drawState != DrawState::LAST) {
        drawState++;
        IssueDrawRequest();
        return;
    }
    drawInProgress = false;
}

void
LinearValueSelector::_DrawHandler()
{
    static_cast<LinearValueSelector *>(app.CurPage())->DrawHandler();
}

bool
LinearValueSelector::GaugeDrawHandler(u8 column, u8 page __UNUSED, u8 *data)
{
    u8 px;
    if (column == 0 || column == 127) {
        px = 0b11111111;
    } else if (column == 64) {
        px = 0b11100011;
    } else if (column == 32 || column == 96) {
        px = 0b01100011;
    } else {
        px = 0b00100010;
    }
    if (column <= gaugeLen) {
        px |= 0b00011100;
    }
    *data = px;
    if (column == 127) {
        DrawHandler();
    }
    return true;
}

bool
LinearValueSelector::_GaugeDrawHandler(u8 column, u8 page, u8 *data)
{
    return static_cast<LinearValueSelector *>(app.CurPage())->
        GaugeDrawHandler(column, page, data);
}
