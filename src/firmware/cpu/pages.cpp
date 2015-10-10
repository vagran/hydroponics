/* This file is a part of 'hydroponics' project.
 * Copyright (c) 2012-2015, Artyom Lebedev <artyom.lebedev@gmail.com>
 * All rights reserved.
 * See LICENSE file for copyright details.
 */

/** @file pages.cpp */

#include "cpu.h"

using namespace adk;

namespace ManCtrl_Light {

void
OnClosed(u16)
{
    app.SetNextPage(Application::GetPageTypeCode<Menu>(),
                    ManualControlMenu::Fabric);
}

void
OnChanged(u16 value)
{
    light.SetLevel(value);
}

void
Fabric(void *p)
{
    TPage *sel = new (p) TPage(strings.LightControl, light.GetLevel(), 0, 255);
    Menu::returnPos = Menu::FindAction(ManualControlMenu::actions, Fabric);
    sel->onClosed = OnClosed;
    sel->onChanged = OnChanged;
}

} /* namespace ManCtrl_Light */


namespace ManCtrl_Pump {

void
OnClosed(u16)
{
    app.SetNextPage(Application::GetPageTypeCode<Menu>(),
                    ManualControlMenu::Fabric);
}

void
OnChanged(u16 value)
{
    pump.SetLevel(value);
}

void
Fabric(void *p)
{
    TPage *sel = new (p) TPage(strings.PumpControl, pump.GetLevel(), 0, 255);
    Menu::returnPos = Menu::FindAction(ManualControlMenu::actions, Fabric);
    sel->onClosed = OnClosed;
    sel->onChanged = OnChanged;
}

} /* namespace ManCtrl_Pump */


namespace Status_LvlGauge {

void
OnClosed(u16)
{
    app.SetNextPage(Application::GetPageTypeCode<Menu>(),
                    StatusMenu::Fabric);
}

void
Poll()
{
    static_cast<TPage *>(app.CurPage())->SetValue(lvlGauge.GetValue());
}

void
Fabric(void *p)
{
    TPage *sel = new (p) TPage(strings.LvlGaugeStatus, lvlGauge.GetValue(),
                               0, 0xff, true);
    Menu::returnPos = Menu::FindAction(StatusMenu::actions, Fabric);
    sel->onClosed = OnClosed;
    sel->poll = Poll;
}

} /* namespace Status_LvlGauge */


namespace Status_LightSensor {

struct {
    u8 sensorA:1;
} g;

void
OnClosed(u16)
{
    app.SetNextPage(Application::GetPageTypeCode<Menu>(),
                    StatusMenu::Fabric);
}

void
Poll()
{
    static_cast<TPage *>(app.CurPage())->SetValue(
        g.sensorA ? light.GetSensorA() : light.GetSensorB());
}

void
FabricA(void *p)
{
    g.sensorA = true;
    TPage *sel = new (p) TPage(strings.LightSensorA, light.GetSensorA(),
                               0, 0xff, true);
    Menu::returnPos = Menu::FindAction(StatusMenu::actions, FabricA);
    sel->onClosed = OnClosed;
    sel->poll = Poll;
}

void
FabricB(void *p)
{
    g.sensorA = false;
    TPage *sel = new (p) TPage(strings.LightSensorB, light.GetSensorB(),
                               0, 0xff, true);
    Menu::returnPos = Menu::FindAction(StatusMenu::actions, FabricB);
    sel->onClosed = OnClosed;
    sel->poll = Poll;
}

} /* namespace Status_LightSensor */


namespace ClbLvlGauge_MinValue {

void
Poll()
{
    static_cast<TPage *>(app.CurPage())->SetHint(lvlGauge.GetRawValue());
}

void
OnClosed(u16)
{
    lvlGauge.SetMinValue(static_cast<TPage *>(app.CurPage())->GetValue());
    lvlGauge.SaveSettings();
    app.SetNextPage(Application::GetPageTypeCode<Menu>(),
                    LvlGaugeCalibrationMenu::Fabric);
}

void
Fabric(void *p)
{
    TPage *sel = new (p) TPage(strings.LvlGaugeClbMin, lvlGauge.GetMinValue(),
                               0, 0xffff);
    Menu::returnPos = Menu::FindAction(LvlGaugeCalibrationMenu::actions, Fabric);
    sel->onClosed = OnClosed;
    sel->poll = Poll;
}

} /* namespace ClbLvlGauge_MinValue */


namespace ClbLvlGauge_MaxValue {

void
OnClosed(u16)
{
    lvlGauge.SetMaxValue(static_cast<TPage *>(app.CurPage())->GetValue());
    lvlGauge.SaveSettings();
    app.SetNextPage(Application::GetPageTypeCode<Menu>(),
                    LvlGaugeCalibrationMenu::Fabric);
}

void
Fabric(void *p)
{
    TPage *sel = new (p) TPage(strings.LvlGaugeClbMax, lvlGauge.GetMaxValue(),
                               0, 0xffff);
    Menu::returnPos = Menu::FindAction(LvlGaugeCalibrationMenu::actions, Fabric);
    sel->onClosed = OnClosed;
    sel->poll = ClbLvlGauge_MinValue::Poll;
}

} /* namespace ClbLvlGauge_MaxValue */


namespace SetupFlooding_PumpThrottle {

void
OnClosed(u16)
{
    flooder.SetPumpThrottle(static_cast<TPage *>(app.CurPage())->GetValue());
    app.SetNextPage(Application::GetPageTypeCode<Menu>(),
                    FloodingSetupMenu::Fabric);
}

void
Fabric(void *p)
{
    TPage *sel = new (p) TPage(strings.FloodingPumpThrottle,
                               flooder.GetPumpThrottle(), 0, 0xff);
    Menu::returnPos = Menu::FindAction(FloodingSetupMenu::actions, Fabric);
    sel->onClosed = OnClosed;
}

} /* namespace SetupFlooding_PumpThrottle */


namespace SetupFlooding_PumpBoostThrottle {

void
OnClosed(u16)
{
    flooder.SetPumpBoostThrottle(static_cast<TPage *>(app.CurPage())->GetValue());
    app.SetNextPage(Application::GetPageTypeCode<Menu>(),
                    FloodingSetupMenu::Fabric);
}

void
Fabric(void *p)
{
    TPage *sel = new (p) TPage(strings.FloodingPumpBoostThrottle,
                               flooder.GetPumpBoostThrottle(), 0, 0xff);
    Menu::returnPos = Menu::FindAction(FloodingSetupMenu::actions, Fabric);
    sel->onClosed = OnClosed;
}

} /* namespace SetupFlooding_PumpThrottle */


namespace SetupFlooding_MinSunriseTime {

void
OnClosed(bool accepted)
{
    if (accepted) {
        Time time = static_cast<TimeSelector *>(app.CurPage())->GetValue();
        flooder.SetMinSunriseTime(time);
    }
    app.SetNextPage(Application::GetPageTypeCode<Menu>(),
                    FloodingSetupMenu::Fabric);
}

void
Fabric(void *p)
{
    TPage *sel = new (p) TPage(strings.FloodingMinSunriseTime,
                               flooder.GetMinSunriseTime());
    Menu::returnPos = Menu::FindAction(FloodingSetupMenu::actions, Fabric);
    sel->onClosed = OnClosed;
}

} /* namespace SetupFlooding_MinSunriseTime */


namespace SetupFlooding_FirstFloodDelay {

void
OnClosed(bool accepted)
{
    if (accepted) {
        Time time = static_cast<TimeSelector *>(app.CurPage())->GetValue();
        flooder.SetFirstFloodDelay(time);
    }
    app.SetNextPage(Application::GetPageTypeCode<Menu>(),
                    FloodingSetupMenu::Fabric);
}

void
Fabric(void *p)
{
    TPage *sel = new (p) TPage(strings.FloodingFirstFloodDelay,
                               flooder.GetFirstFloodDelay());
    Menu::returnPos = Menu::FindAction(FloodingSetupMenu::actions, Fabric);
    sel->onClosed = OnClosed;
}

} /* namespace SetupFlooding_FirstFloodDelay */


namespace SetupFlooding_FloodDuration {

void
OnClosed(bool accepted)
{
    if (accepted) {
        Time time = static_cast<TimeSelector *>(app.CurPage())->GetValue();
        flooder.SetFloodDuration(time);
    }
    app.SetNextPage(Application::GetPageTypeCode<Menu>(), \
                    FloodingSetupMenu::Fabric);
}

void
Fabric(void *p)
{
    TPage *sel = new (p) TPage(strings.FloodingFloodDuration,
                               flooder.GetFloodDuration());
    Menu::returnPos = Menu::FindAction(FloodingSetupMenu::actions, Fabric);
    sel->onClosed = OnClosed;
}

} /* namespace SetupFlooding_FloodDuration */


namespace SetupFlooding_FloodPeriod {

void
OnClosed(bool accepted)
{
    if (accepted) {
        Time time = static_cast<TimeSelector *>(app.CurPage())->GetValue();
        flooder.SetFloodPeriod(time);
    }
    app.SetNextPage(Application::GetPageTypeCode<Menu>(),
                    FloodingSetupMenu::Fabric);
}

void
Fabric(void *p)
{
    TPage *sel = new (p) TPage(strings.FloodingFloodPeriod,
                               flooder.GetFloodPeriod());
    Menu::returnPos = Menu::FindAction(FloodingSetupMenu::actions, Fabric);
    sel->onClosed = OnClosed;
}

} /* namespace SetupFlooding_FloodPeriod */


namespace SetupFlooding_MaxSunsetTime {

void
OnClosed(bool accepted)
{
    if (accepted) {
        Time time = static_cast<TimeSelector *>(app.CurPage())->GetValue();
        flooder.SetMaxSunsetTime(time);
    }
    app.SetNextPage(Application::GetPageTypeCode<Menu>(),
                    FloodingSetupMenu::Fabric);
}

void
Fabric(void *p)
{
    TPage *sel = new (p) TPage(strings.FloodingMaxSunsetTime,
                               flooder.GetMaxSunsetTime());
    Menu::returnPos = Menu::FindAction(FloodingSetupMenu::actions, Fabric);
    sel->onClosed = OnClosed;
}

} /* namespace SetupFlooding_MaxSunsetTime */


namespace SetupTime {

void
OnClosed(bool accepted)
{
    if (accepted) {
        Time time = static_cast<TimeSelector *>(app.CurPage())->GetValue();
        rtc.SetTime(Rtc::Time{time.hour, time.min, 0});
    }
    app.SetNextPage(Application::GetPageTypeCode<Menu>(), SetupMenu::Fabric);
}

void
Fabric(void *p)
{
    Rtc::Time curTime = rtc.GetTime();
    TPage *sel = new (p) TPage(strings.TimeSetup,
                               Time{curTime.hour, curTime.min});
    Menu::returnPos = Menu::FindAction(SetupMenu::actions, Fabric);
    sel->onClosed = OnClosed;
}

} /* namespace SetupTime */
