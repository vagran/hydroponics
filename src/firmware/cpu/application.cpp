/* This file is a part of 'hydroponics' project.
 * Copyright (c) 2012-2015, Artyom Lebedev <artyom.lebedev@gmail.com>
 * All rights reserved.
 * See COPYING file for copyright details.
 */

/** @file application.cpp
 * TODO insert description here.
 */

#include "cpu.h"

using namespace adk;

Application app;

class A {
public:
    int i;
    A(int i):i(i){}
};

Variant<int, A, u8> v;

void
Application::Poll()
{
    v.Get<int>();
    v.Disengage();
    v.Engage<int>(1.0);
}
