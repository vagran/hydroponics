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
    MENU_ACTIONS_END
};

void
Fabric(void *p)
{
    new (p) Menu(strings.ManualControlMenu, Menu::returnPos, actions, 0);
    Menu::returnPos = Menu::FindAction(MainMenu::actions, Fabric);
}

} /* namespace ManualControlMenu */


namespace CalibrationMenu {

const Menu::Action actions[] = {
    {Application::GetPageTypeCode<Menu>(), MainMenu::Fabric},
    {0, nullptr},
    {0, nullptr},
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
    {0, nullptr},
    {0, nullptr},
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


namespace StatusMenu {

const Menu::Action actions[] = {
    {Application::GetPageTypeCode<Menu>(), MainMenu::Fabric},
    {0, nullptr},
    {Application::GetPageTypeCode<Status_LvlGauge::TPage>(),
     Status_LvlGauge::Fabric},
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
