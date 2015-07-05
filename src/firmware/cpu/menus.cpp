/* This file is a part of 'hydroponics' project.
 * Copyright (c) 2012-2015, Artyom Lebedev <artyom.lebedev@gmail.com>
 * All rights reserved.
 * See COPYING file for copyright details.
 */

/** @file menus.cpp */

#include "cpu.h"

using namespace adk;

namespace MainMenu {

const Menu::Action actions[] = {
    {Application::GetPageTypeCode<MainPage>(), MainPage::Fabric},
    {Application::GetPageTypeCode<Menu>(), ManualControlMenu::Fabric},
    {Application::GetPageTypeCode<Menu>(), CalibrationMenu::Fabric},
    {Application::GetPageTypeCode<Menu>(), SetupMenu::Fabric},
    {Application::GetPageTypeCode<Menu>(), StatusMenu::Fabric},
    MENU_ACTIONS_END
};

void
Fabric(void *p)
{
    new (p) Menu(strings.MainMenu, Menu::returnPos, actions);
    Menu::returnPos = 0;
}

} /* namespace MainMenu */


namespace ManualControlMenu {

const Menu::Action actions[] = {
    {Application::GetPageTypeCode<Menu>(), MainMenu::Fabric},
    {Application::GetPageTypeCode<ManCtrl_Light::TPage>(), ManCtrl_Light::Fabric},
    {Application::GetPageTypeCode<ManCtrl_Pump::TPage>(), ManCtrl_Pump::Fabric},
    {0, nullptr}, /* Start flooding */
    MENU_ACTIONS_END
};

bool
CustomActionHandler(u8 idx)
{
    switch (idx) {
    case 3: /* Start flooding */
        flooder.StartFlooding();
        break;
    default:
        return false;
    }
    return true;
}

void
Fabric(void *p)
{
    Menu *menu = new (p) Menu(strings.ManualControlMenu, Menu::returnPos,
                              actions, 0);
    menu->itemHandler = CustomActionHandler;
    Menu::returnPos = Menu::FindAction(MainMenu::actions, Fabric);
}

} /* namespace ManualControlMenu */


namespace CalibrationMenu {

const Menu::Action actions[] = {
    {Application::GetPageTypeCode<Menu>(), MainMenu::Fabric},
    {0, nullptr},
    {Application::GetPageTypeCode<Menu>(), LvlGaugeCalibrationMenu::Fabric},
    {0, nullptr},
    MENU_ACTIONS_END
};

void
Fabric(void *p)
{
    new (p) Menu(strings.CalibrationMenu, Menu::returnPos, actions, 0);
    Menu::returnPos = Menu::FindAction(MainMenu::actions, Fabric);
}

} /* namespace CalibrationMenu */


namespace SetupMenu {

const Menu::Action actions[] = {
    {Application::GetPageTypeCode<Menu>(), MainMenu::Fabric},
    {Application::GetPageTypeCode<SetupTime::TPage>(), SetupTime::Fabric},
    {Application::GetPageTypeCode<Menu>(), FloodingSetupMenu::Fabric},
    {0, nullptr},
    MENU_ACTIONS_END
};

void
Fabric(void *p)
{
    new (p) Menu(strings.SetupMenu, Menu::returnPos, actions, 0);
    Menu::returnPos = Menu::FindAction(MainMenu::actions, Fabric);
}

} /* namespace SetupMenu */


namespace FloodingSetupMenu {

const Menu::Action actions[] = {
    {Application::GetPageTypeCode<Menu>(), SetupMenu::Fabric},
    {Application::GetPageTypeCode<SetupFlooding_PumpThrottle::TPage>(),
                                  SetupFlooding_PumpThrottle::Fabric},
    {Application::GetPageTypeCode<SetupFlooding_PumpBoostThrottle::TPage>(),
                                  SetupFlooding_PumpBoostThrottle::Fabric},
    MENU_ACTIONS_END
};

void
Fabric(void *p)
{
    new (p) Menu(strings.FloodingSetupMenu, Menu::returnPos, actions, 0);
    Menu::returnPos = Menu::FindAction(SetupMenu::actions, Fabric);
}

} /* namespace FloodingSetupMenu */


namespace StatusMenu {

const Menu::Action actions[] = {
    {Application::GetPageTypeCode<Menu>(), MainMenu::Fabric},
    {Application::GetPageTypeCode<Status_LvlGauge::TPage>(),
     Status_LvlGauge::Fabric},
    {Application::GetPageTypeCode<Status_LightSensor::TPage>(),
     Status_LightSensor::FabricA},
    {Application::GetPageTypeCode<Status_LightSensor::TPage>(),
     Status_LightSensor::FabricB},
    {0, nullptr},
    MENU_ACTIONS_END
};

void
Fabric(void *p)
{
    new (p) Menu(strings.StatusMenu, Menu::returnPos, actions, 0);
    Menu::returnPos = Menu::FindAction(MainMenu::actions, Fabric);
}

} /* namespace SetupMenu */


namespace LvlGaugeCalibrationMenu {

const Menu::Action actions[] = {
    {Application::GetPageTypeCode<Menu>(), CalibrationMenu::Fabric},
    {Application::GetPageTypeCode<ClbLvlGauge_MinValue::TPage>(),
     ClbLvlGauge_MinValue::Fabric},
    {Application::GetPageTypeCode<ClbLvlGauge_MaxValue::TPage>(),
     ClbLvlGauge_MaxValue::Fabric},
    MENU_ACTIONS_END
};

void
Fabric(void *p)
{
    new (p) Menu(strings.LvlGaugeCalibrationMenu, Menu::returnPos, actions, 0);
    Menu::returnPos = Menu::FindAction(CalibrationMenu::actions, Fabric);
}

} /* namespace LvlGaugeCalibrationMenu */
