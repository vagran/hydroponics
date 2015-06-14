/* This file is a part of 'hydroponics' project.
 * Copyright (c) 2012-2015, Artyom Lebedev <artyom.lebedev@gmail.com>
 * All rights reserved.
 * See COPYING file for copyright details.
 */

/** @file level_gauge.cpp */

#include "cpu.h"

using namespace adk;

LevelGauge lvlGauge;

LevelGauge::LevelGauge()
{
    enabled = false;
    accResult = 0;
    /* Running at system clock frequency, normal mode. Input capture for falling
     * edge.
     */
    TCCR1B = _BV(CS10) | _BV(ICNC1);
    TIMSK1 = _BV(ICIE1) | _BV(TOIE1);
    AVR_BIT_SET8(AVR_REG_DDR(LVL_GAUGE_TRIG_PORT), LVL_GAUGE_TRIG_PIN);

    /* Workaround for strange behaviour when echo output is initially high
     * forever. Make one trigger pulse.
     */
    AVR_BIT_SET8(AVR_REG_PORT(LVL_GAUGE_TRIG_PORT), LVL_GAUGE_TRIG_PIN);
    _delay_us(10);
    AVR_BIT_CLR8(AVR_REG_PORT(LVL_GAUGE_TRIG_PORT), LVL_GAUGE_TRIG_PIN);
    _delay_ms(10);
    /* Wait for echo end. */
    while (AVR_BIT_GET8(AVR_REG_PIN(LVL_GAUGE_ECHO_PORT), LVL_GAUGE_ECHO_PIN));
}

void
LevelGauge::Trigger()
{
    AtomicSection as;

    if (inProgress) {
        /* Measurement in progress. Drop the new request. */
        return;
    }
    if (AVR_BIT_GET8(AVR_REG_PIN(LVL_GAUGE_ECHO_PORT), LVL_GAUGE_ECHO_PIN)) {
        /* Previous echo still active. Probably sensor failure. */
        failure = true;
        return;
    }

    inProgress = true;
    echoStarted = false;
    overflowSeen = false;
    /* Set trigger line and wait for echo start. */
    AVR_BIT_SET8(AVR_REG_PORT(LVL_GAUGE_TRIG_PORT), LVL_GAUGE_TRIG_PIN);
    /* Capture rising edge. */
    AVR_BIT_SET8(TCCR1B, ICES1);
    TCNT1 = 0;
    /* Reset pending counter interrupts if any. */
    TIFR1 = _BV(ICF1) | _BV(TOV1);
}

void
LevelGauge::Timer1Ovf()
{
    if (!inProgress) {
        /* No active measurement. */
        return;
    }
    if (!overflowSeen) {
        overflowSeen = true;
        return;
    }
    inProgress = false;
    /* Indicate out-of-range. */
    OnResult(0xffff);
}

void
LevelGauge::Timer1Capt()
{
    u16 time = ICR1;
    if (!inProgress) {
        /* No active measurement. */
        return;
    }
    if (!echoStarted) {
        /* Capture falling edge. */
        AVR_BIT_CLR8(TCCR1B, ICES1);
        /* Clear pending capture interrupt, required if edge changed. */
        TIFR1 = _BV(ICF1);
        /* End trigger pulse. */
        AVR_BIT_CLR8(AVR_REG_PORT(LVL_GAUGE_TRIG_PORT), LVL_GAUGE_TRIG_PIN);

        /* Echo line should be high. */
        if (!AVR_BIT_GET8(AVR_REG_PIN(LVL_GAUGE_ECHO_PORT), LVL_GAUGE_ECHO_PIN)) {
            failure = true;
            inProgress = false;
            return;
        }

        echoStartTime = time;
        echoStarted = true;
        return;
    }
    /* Echo line should be low. */
    if (AVR_BIT_GET8(AVR_REG_PIN(LVL_GAUGE_ECHO_PORT), LVL_GAUGE_ECHO_PIN)) {
        failure = true;
        inProgress = false;
        return;
    }
    if (overflowSeen &&
        (static_cast<u32>(time) + 0x10000ul - echoStartTime > 0xffff)) {

        OnResult(0xffff);
    } else {
        OnResult(time - echoStartTime);
    }
    inProgress = false;
}

ISR(TIMER1_OVF_vect)
{
    lvlGauge.Timer1Ovf();
}

ISR(TIMER1_CAPT_vect)
{
    lvlGauge.Timer1Capt();
}

void
LevelGauge::Enable()
{
    AtomicSection as;
    if (!enabled) {
        enabled = true;
        scheduler.ScheduleTask(_PeriodicTask, INTERVAL);
    }
}

void
LevelGauge::Disable()
{
    AtomicSection as;
    if (enabled) {
        enabled = false;
        scheduler.UnscheduleTask(_PeriodicTask);
    }
}

u16
LevelGauge::_PeriodicTask()
{
    return lvlGauge.PeriodicTask();
}

u16
LevelGauge::PeriodicTask()
{
    Trigger();
    return INTERVAL;
}

void
LevelGauge::OnResult(u16 result)
{
    if (accResult == 0) {
        accResult = result << ROLL_AVG_BITS;
    } else {
        accResult = accResult - (accResult >> ROLL_AVG_BITS) + result;
    }
}

u8
LevelGauge::GetValue()
{
    u16 value = GetRawValue();
    if (value < minValue) {
        value = minValue;
    } else if (value > maxValue) {
        value = maxValue;
    }
    value -= minValue;
    if (minValue == maxValue) {
        return 0;
    }
    u8 result = static_cast<u32>(value) * 0xff / (maxValue - minValue);
    return 0xff - result;
}
