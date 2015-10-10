/* This file is a part of 'hydroponics' project.
 * Copyright (c) 2012-2015, Artyom Lebedev <artyom.lebedev@gmail.com>
 * All rights reserved.
 * See LICENSE file for copyright details.
 */

/** @file lighting.cpp */

#include "cpu.h"

using namespace adk;

Light light;

void
Light::OnAdcResult(u8 channel, u16 value)
{
    if (channel == AdcChannel::SENSOR_A) {
        curSensorA = value >> 2;
    } else if (channel == AdcChannel::SENSOR_B) {
        curSensorB = value >> 2;
    }
}

void
Light::Enable()
{
    scheduler.ScheduleTask(PeriodicTask, MEASUREMENT_PERIOD);
}

u16
Light::PeriodicTask()
{
    adc.ScheduleConversion(AdcChannel::SENSOR_A);
    adc.ScheduleConversion(AdcChannel::SENSOR_B);
    return MEASUREMENT_PERIOD;
}
