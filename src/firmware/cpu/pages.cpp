/* This file is a part of 'hydroponics' project.
 * Copyright (c) 2012-2015, Artyom Lebedev <artyom.lebedev@gmail.com>
 * All rights reserved.
 * See COPYING file for copyright details.
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
    LinearValueSelector *sel =
        new (p) LinearValueSelector(strings.LightControl, light.GetLevel(),
                                    0, 255);
    Menu::returnPos = Menu::FindAction(ManualControlMenu::actions,
                                       ManCtrl_Light::Fabric);
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
    LinearValueSelector *sel =
        new (p) LinearValueSelector(strings.PumpControl, pump.GetLevel(),
                                    0, 255);
    Menu::returnPos = Menu::FindAction(ManualControlMenu::actions,
                                       ManCtrl_Pump::Fabric);
    sel->onClosed = OnClosed;
    sel->onChanged = OnChanged;
}

} /* namespace ManCtrl_Pump */
