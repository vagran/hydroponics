/* This file is a part of 'hydroponics' project.
 * Copyright (c) 2012-2015, Artyom Lebedev <artyom.lebedev@gmail.com>
 * All rights reserved.
 * See LICENSE file for copyright details.
 */

/** @file adc.cpp */

#include "cpu.h"

using namespace adk;

Adc adc;

void
OnAdcResult(u8 channel, u16 result)
{
    if (channel == Light::AdcChannel::SENSOR_A ||
        channel == Light::AdcChannel::SENSOR_B) {

        light.OnAdcResult(channel, result);
    }
}

Adc::Adc()
{
    /* Internal Vcc reference. */
    ADMUX = _BV(REFS0);
    ADCSRA = _BV(ADEN) | _BV(ADIE) | _BV(ADPS0) | _BV(ADPS1) | _BV(ADPS2);
    inProgress = false;
    curChannel = 0;
}

void
Adc::Poll()
{
    AtomicSection as;
    if (pendingChannelsMask && !inProgress) {
        inProgress = true;
        NextConversion();
    }
}

void
Adc::ScheduleConversion(u8 channel)
{
    if (channel >= NUM_CHANNELS) {
        return;
    }
    AtomicSection as;
    pendingChannelsMask |= 1 << channel;
    if (!inProgress) {
        inProgress = true;
        NextConversion();
    }
}

bool
Adc::SleepEnabled()
{
    AtomicSection as;
    return !inProgress;
}

void
Adc::StartConversion(u8 channel)
{
    ADMUX = (ADMUX & ~0xf) | channel;
    AVR_BIT_SET8(ADCSRA, ADSC);
}

void
Adc::NextConversion()
{
    if (!pendingChannelsMask) {
        inProgress = false;
        curChannel = 0;
        return;
    }
    u8 channel = curChannel + 1;
    while (true) {
        if (channel >= NUM_CHANNELS) {
            channel = 0;
        }
        u8 mask = 1 << channel;
        if (pendingChannelsMask & mask) {
            pendingChannelsMask &= ~mask;
            curChannel = channel;
            StartConversion(channel);
            return;
        }
        channel++;
    }
}

void
Adc::HandleInterrupt()
{
    OnAdcResult(curChannel, (u16)ADCL | ((u16)ADCH << 8));
    NextConversion();
}

ISR(ADC_vect)
{
    adc.HandleInterrupt();
}
