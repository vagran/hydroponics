/* This file is a part of 'hydroponics' project.
 * Copyright (c) 2015, Artyom Lebedev <artyom.lebedev@gmail.com>
 * All rights reserved.
 * See LICENSE file for copyright details.
 */

/** @file main_page.cpp */

#include "cpu.h"

using namespace adk;

void
MainPage::Fabric(void *p)
{
    new (p) MainPage();
}

MainPage::MainPage()
{
    drawMask = DrawMask::M_ALL;
    drawInProgress = false;
    closeRequested = false;
    pumpActive = false;
    drainActive = false;
    isDaylight = flooder.IsAmbientDaylight();

    flooderStatus = Flooder::Status::IDLE;
    flooderError = 0;

    scheduler.ScheduleTask(_AnimationTask, ANIMATION_PERIOD);

    watLevelBottom = MAX_WATER_LEVEL;
    watLevelTop = 0;

    SetStatus(flooder.GetStatusString());
}

void
MainPage::OnButtonPressed()
{
    app.SetNextPage(Application::GetPageTypeCode<Menu>(),
                    MainMenu::Fabric);
}

void
MainPage::OnRotEncClick(bool dir __UNUSED/*XXX*/)
{
    //XXX
}

void
MainPage::Poll()
{
    Page::Poll();
    AtomicSection as;
    if (!drawInProgress && drawMask) {
        Draw(drawMask);
    }
}

bool
MainPage::RequestClose()
{
    AtomicSection as;
    if (!closeRequested) {
        closeRequested = true;
        scheduler.UnscheduleTask(_AnimationTask);
    }
    return !drawInProgress;
}

void
MainPage::Draw(u16 mask)
{
    AtomicSection as;
    if (closeRequested) {
        return;
    }
    drawMask |= mask;
    if (drawInProgress) {
        return;
    }
    drawInProgress = true;
    drawState = 0;
    IssueDrawRequest();
}

void
MainPage::IssueDrawRequest()
{
    do {
        switch (drawState) {

        case DrawState::POT_WALLS_1:
            if (!(drawMask & DrawMask::M_STATIC)) {
                drawState = DrawState::PUMP;
                break;
            }
            drawMask &= ~DrawMask::M_STATIC;
            bitmapWriter.Write(POT_COL, POT_PAGE, &bitmaps.PotWall, false,
                               _DrawHandler);
            return;

        case DrawState::POT_WALLS_2:
            bitmapWriter.Write(POT_COL + 23, POT_PAGE, &bitmaps.PotWall, false,
                               _DrawHandler);
            return;

        case DrawState::POT_WALLS_3:
            bitmapWriter.Write(POT_COL + 16, POT_PAGE, &bitmaps.SiphonTop, false,
                               _DrawHandler);
            return;

        case DrawState::POT_WALLS_4:
            bitmapWriter.Write(POT_COL + 16, POT_PAGE + 2,
                               &bitmaps.SiphonBottom, false,
                               _DrawHandler);
            return;

        case DrawState::POT_WALLS_5:
            bitmapWriter.Write(POT_COL + 16, POT_PAGE + 1,
                               &bitmaps.SiphonWall, false,
                               _DrawHandler);
            return;

        case DrawState::POT_WALLS_PUMP1:
            bitmapWriter.Write(POT_COL - 5, POT_PAGE + 2,
                               &bitmaps.PumpPipeTop, false,
                               _DrawHandler);
            return;

        case DrawState::POT_WALLS_PUMP2:
            bitmapWriter.Write(POT_COL - 5, POT_PAGE + 4,
                               &bitmaps.PumpPipeBottom, false,
                               _DrawHandler);
            return;

        case DrawState::SUNRISE_ICON:
            bitmapWriter.Write(0, 2, &bitmaps.Sun, false,
                               _DrawHandler);
            return;

        case DrawState::SUNSET_ICON:
            bitmapWriter.Write(9 + 5 * (FONT_WIDTH + 1), 2, &bitmaps.Sunset,
                               false, _DrawHandler);
            return;

        case DrawState::TEMP_ICON:
            bitmapWriter.Write(0, 1, &bitmaps.Thermometer, false,
                               _DrawHandler);
            return;

        case DrawState::PUMP:
            if (!(drawMask & DrawMask::M_PUMP)) {
                drawState = DrawState::DRAIN;
                break;
            }
            drawMask &= ~DrawMask::M_PUMP;
            bitmapWriter.Write(POT_COL - 8, POT_PAGE + 3,
                               pumpActive ? &bitmaps.PumpActive : &bitmaps.PumpInactive,
                               false, _DrawHandler);
            return;

        case DrawState::DRAIN:
            if (!(drawMask & DrawMask::M_DRAIN)) {
                drawState = DrawState::TOP_WATER;
                break;
            }
            drawMask &= ~DrawMask::M_DRAIN;
            if (drainActive) {
                bitmapWriter.Write(POT_COL + 17, POT_PAGE + 1,
                                   &bitmaps.SyphonDrain, false, _DrawHandler);
            } else {
                bitmapWriter.Clear(POT_COL + 17, POT_PAGE + 1,
                                   &bitmaps.SyphonDrain, false, _DrawHandler);
            }
            return;

        case DrawState::TOP_WATER:
            if (!(drawMask & DrawMask::M_TOP_WATER)) {
                drawState = DrawState::BOTTOM_WATER;
                break;
            }
            drawMask &= ~DrawMask::M_TOP_WATER;
            display.Output(Display::Viewport{POT_COL + 1, POT_COL + 15,
                                             POT_PAGE, POT_PAGE + 2},
                           _DisplayOutputHandler);
            return;

        case DrawState::BOTTOM_WATER:
            if (!(drawMask & DrawMask::M_BOTTOM_WATER)) {
                drawState = DrawState::STATUS;
                break;
            }
            drawMask &= ~DrawMask::M_BOTTOM_WATER;
            display.Output(Display::Viewport{POT_COL + 1, POT_COL + 22,
                                             POT_PAGE + 3, POT_PAGE + 5},
                           _DisplayOutputHandler);
            return;

        case DrawState::STATUS:
            if (!(drawMask & DrawMask::M_STATUS)) {
                drawState = DrawState::DAYLIGHT;
                break;
            }
            drawMask &= ~DrawMask::M_STATUS;
            if (!status) {
                display.Clear(Display::Viewport{0, 127, 7, 7});
                drawState = DrawState::DAYLIGHT;
                break;
            }
            if (isStatusPgm) {
                textWriter.Write(Display::Viewport{0, 127, 7, 7},
                                 status + statusOffset, false,
                                 true, _DrawHandler);
            } else {
                textWriter.Write(Display::Viewport{0, 127, 7, 7},
                                 const_cast<char *>(status) + statusOffset, false,
                                 true, _DrawHandler);
            }
            return;

        case DrawState::DAYLIGHT:
            if (!(drawMask & DrawMask::M_DAYLIGHT)) {
               drawState = DrawState::CLOCK;
               break;
           }
           drawMask &= ~DrawMask::M_DAYLIGHT;
           bitmapWriter.Write(0, 0, flooder.IsAmbientDaylight() ? &bitmaps.Sun :
                                                                  &bitmaps.Moon,
                              false, _DrawHandler);
           return;

        case DrawState::CLOCK:
            if (!(drawMask & DrawMask::M_CLOCK)) {
                drawState = DrawState::SUNRISE;
                break;
            }
            drawMask &= ~DrawMask::M_CLOCK;
            GetClockText();
            textWriter.Write(Display::Viewport{12, 12 + (FONT_WIDTH + 1) * 8 - 1, 0, 0},
                             textBuf, false, true, _DrawHandler);
            return;

        case DrawState::SUNRISE:
            if (!(drawMask & DrawMask::M_SUNRISE)) {
                drawState = DrawState::SUNSET;
                break;
            }
            drawMask &= ~DrawMask::M_SUNRISE;
            GetTimeText(flooder.GetLastSunriseTime());
            textWriter.Write(Display::Viewport{9, 9 + 5 * (FONT_WIDTH + 1) - 1,
                                               2, 2},
                             textBuf, false, true, _DrawHandler);
            return;

        case DrawState::SUNSET: {
            if (!(drawMask & DrawMask::M_SUNSET)) {
                drawState = DrawState::TEMPERATURE;
                break;
            }
            drawMask &= ~DrawMask::M_SUNSET;
            GetTimeText(flooder.GetLastSunsetTime());
            u8 x1 = 9 + 5 * (FONT_WIDTH + 1) + 9;
            u8 x2 = x1 + 5 * (FONT_WIDTH + 1) - 1;
            textWriter.Write(Display::Viewport{x1, x2, 2, 2},
                             textBuf, false, true, _DrawHandler);
            return;
        }

        case DrawState::TEMPERATURE:
            if (!(drawMask & DrawMask::M_TEMPERATURE)) {
                drawState = DrawState::FLOOD_TIME;
                break;
            }
            drawMask &= ~DrawMask::M_TEMPERATURE;
            GetTemperatureText();
            textWriter.Write(Display::Viewport{7, POT_COL - 2, 1, 1},
                             textBuf, false, true, _DrawHandler);
            return;


        case DrawState::FLOOD_TIME: {
            if (!(drawMask & DrawMask::M_FLOOD_TIME)) {
                drawState = DrawState::DONE;
                break;
            }
            drawMask &= ~DrawMask::M_FLOOD_TIME;
            Time t = flooder.GetNextFloodTime();
            if (t) {
                GetTimeText(t - rtc.GetTime().GetTime());
            } else {
                textBuf[0] = '-';
                textBuf[1] = '-';
                textBuf[2] = ':';
                textBuf[3] = '-';
                textBuf[4] = '-';
                textBuf[5] = 0;
            }
            textWriter.Write(Display::Viewport{128 - (FONT_WIDTH + 1) * 5, 127, 0, 0},
                             textBuf, false, true, _DrawHandler);
            return;
        }
        default:
            break;
        }
    } while (drawMask && drawState < DrawState::DONE);
    drawInProgress = false;
}

void
MainPage::DrawHandler()
{
    AtomicSection as;

    if (closeRequested) {
        drawInProgress = false;
        return;
    }
    drawState++;
    if (drawState != DrawState::DONE) {
        IssueDrawRequest();
        return;
    }
    drawInProgress = false;
}

void
MainPage::_DrawHandler()
{
    return static_cast<MainPage *>(app.CurPage())->DrawHandler();
}

/** Get byte which has numBits most significant bits set. */
static inline u8
GetFillBits(u8 numBits)
{
    u8 result = 0;
    while (numBits) {
        result = (result >> 1) | 0x80;
        numBits--;
    }
    return result;
}

bool
MainPage::DisplayOutputHandler(u8 column, u8 page, u8 *data)
{
    u8 _data = 0;
    if (drawState == DrawState::TOP_WATER) {

        if (page == POT_PAGE) {
            _data = 0b00000001;
            if (watLevelTop > 15) {
                _data |= GetFillBits(watLevelTop - 15);
            }
        } else if (page == POT_PAGE + 1) {
            if (watLevelTop >= 15) {
                _data = 0xff;
            } else if (watLevelTop > 7) {
                _data = GetFillBits(watLevelTop - 7);
            } else {
                _data = 0;
            }
        } else {
            if (watLevelTop >= 7) {
                _data = 0xff;
            } else {
                _data = 0x80 | (GetFillBits(watLevelTop) >> 1);
            }
        }

        if (column == POT_COL + 15 && page == POT_PAGE + 2) {
            DrawHandler();
        }

    } else if (drawState == DrawState::BOTTOM_WATER) {

        if (page == POT_PAGE + 3) {
            if (watLevelBottom > 15) {
                _data = GetFillBits(watLevelBottom - 15);
            } else {
                _data = 0;
            }
        } else if (page == POT_PAGE + 4) {
            if (watLevelBottom >= 15) {
                _data = 0xff;
            } else if (watLevelBottom > 7) {
                _data = GetFillBits(watLevelBottom - 7);
            } else {
                _data = 0;
            }
        } else {
            if (watLevelBottom >= 7) {
                _data = 0xff;
            } else {
                _data = 0x80 | (GetFillBits(watLevelBottom) >> 1);
            }
        }

        if (column == POT_COL + 22 && page == POT_PAGE + 5) {
            DrawHandler();
        }
    }
    *data = _data;
    return true;
}

bool
MainPage::_DisplayOutputHandler(u8 column, u8 page, u8 *data)
{
    return static_cast<MainPage *>(app.CurPage())->
        DisplayOutputHandler(column, page, data);
}

void
MainPage::SetStatus(const char *status, bool isPgm)
{
    AtomicSection as;
    this->status = status;
    isStatusPgm = isPgm;
    statusOffset = 0;
    statusScrollDir = false;
    if (status) {
        if (isPgm) {
            statusLen = strlen_P(status);
        } else {
            statusLen = strlen(status);
        }
    } else {
        statusLen = 0;
    }
    Draw(DrawMask::M_STATUS);
}

u16
MainPage::_AnimationTask()
{
    return static_cast<MainPage *>(app.CurPage())->
        AnimationTask();
}

void
MainPage::CheckFlooderStatus()
{
    u8 newStatus = flooder.GetStatus();
    u8 newError = flooder.GetErrorCode();
    if (newStatus != flooderStatus ||
        (flooderStatus == Flooder::Status::FAILURE && newError != flooderError)) {

        if (newStatus != Flooder::Status::FAILURE) {
            SetStatus(flooder.GetStatusString());
        } else {
            SetStatus(flooder.GetErrorString());
            sound.SetPattern(0x0001, true);
            led.SetMode(Led::Mode::FAILURE);
        }
        flooderStatus = newStatus;
        flooderError = newError;
    }

    if (newStatus != Flooder::Status::FLOODING &&
        newStatus != Flooder::Status::FLOOD_FINAL) {

        if (pumpActive) {
            pumpActive = false;
            drawMask |= DrawMask::M_PUMP;
        }
    } else {
        pumpActive = !pumpActive;
        drawMask |= DrawMask::M_PUMP;
    }

    if (newStatus != Flooder::Status::DRAINING &&
        newStatus != Flooder::Status::FLOOD_FINAL) {

        if (drainActive) {
            drainActive = false;
            drawMask |= DrawMask::M_DRAIN | DrawMask::M_FLOOD_TIME;
        }
    } else {
        drainActive = !drainActive;
        drawMask |= DrawMask::M_DRAIN;
    }

    u8 level = lvlGauge.GetValue();
    level = static_cast<u16>(level) * MAX_WATER_LEVEL / 0xff;
    if (level != watLevelBottom) {
        watLevelBottom = level;
        drawMask |= DrawMask::M_BOTTOM_WATER;
    }

    level = flooder.GetTopPotWaterLevel();
    level = static_cast<u16>(level) * MAX_WATER_LEVEL / 0xff;
    if (level != watLevelTop) {
        watLevelTop = level;
        drawMask |= DrawMask::M_TOP_WATER;
    }

    u8 dl = flooder.IsAmbientDaylight() ? 1 : 0;
    if (dl != isDaylight) {
        isDaylight = dl;
        drawMask |= DrawMask::M_DAYLIGHT | DrawMask::M_SUNRISE | DrawMask::M_SUNSET;
    }

    Time curTime = rtc.GetTime().GetTime();
    if ((curTime.min & 1) != lastMinute) {
        lastMinute = curTime.min & 1;
        drawMask |= DrawMask::M_FLOOD_TIME;
    }
}

u16
MainPage::AnimationTask()
{
    AtomicSection as;
    animationDivider++;

    CheckFlooderStatus();
    rtc.Update();

    drawMask |= DrawMask::M_CLOCK;

    if ((animationDivider & 31) == 0) {
        drawMask |= DrawMask::M_TEMPERATURE;
    }

    if (statusLen > 18) {
        bool pauseSet = false;
        if (!statusPause) {
            if (statusOffset == 0) {
                statusScrollDir = false;
                statusPause = true;
                pauseSet = true;
            } else if (statusOffset == statusLen - 18) {
                statusScrollDir = true;
                statusPause = true;
                pauseSet = true;
            }
        } else {
            statusPause = false;
        }

        if (!pauseSet) {
            if (statusScrollDir) {
                statusOffset--;
            } else {
                statusOffset++;
            }
            Draw(DrawMask::M_STATUS);
        }
    }
    return ANIMATION_PERIOD;
}

void
MainPage::GetClockText()
{
    Rtc::Time time = rtc.GetTime();
    Strings::StrClockNum(time.hour, textBuf);
    textBuf[2] = ':';
    Strings::StrClockNum(time.min, textBuf + 3);
    textBuf[5] = ':';
    Strings::StrClockNum(time.sec, textBuf + 6);
}

void
MainPage::GetTimeText(Time t)
{
    Strings::StrClockNum(t.hour, textBuf);
    textBuf[2] = ':';
    Strings::StrClockNum(t.min, textBuf + 3);
}

void
MainPage::GetTemperatureText()
{
    i16 temp = rtc.GetTemperature();
    itoa(temp >> 2, textBuf, 10);
    u8 len = strlen(textBuf);
    textBuf[len] = '.';
    u8 frac = temp & 3;
    textBuf[len + 1] = '0' + ((10 * frac + 2) >> 2);
    textBuf[len + 2] = 0x10;
    textBuf[len + 3] = 'C';
    textBuf[len + 4] = 0;
}
