/* This file is a part of 'hydroponics' project.
 * Copyright (c) 2015, Artyom Lebedev <artyom.lebedev@gmail.com>
 * All rights reserved.
 * See COPYING file for copyright details.
 */
#include <adk.h>
#include "cpu.h"

static inline void
OnClockMinute();

static inline void
OnAdcResult(u8 type, u16 value);

static inline void
OnButtonPressed();

static inline void
OnButtonLongPressed();

/* ****************************************************************************/
/* System clock. */

static u32 g_clockTicks = 0;

/** Current hour, minute, second and day of week. */
static u8 g_clockHour, g_clockMin, g_clockSec, g_clockDow;

typedef struct {
    /** Non-zero if scheduled. */
    u16 delay;
    TaskHandler handler;
} Task;

static Task g_tasks[MAX_TASKS];

TaskId
ScheduleTask(TaskHandler handler, u16 delay)
{
    u8 sreg = SREG;
    cli();
    for (TaskId id = 0; id < MAX_TASKS; id++) {
        if (g_tasks[id].delay == 0) {
            g_tasks[id].delay = delay;
            g_tasks[id].handler = handler;
            SREG = sreg;
            return id;
        }
    }
    SREG = sreg;
    return INVALID_TASK;
}

void
UnscheduleTask(TaskId id)
{
    u8 sreg = SREG;
    cli();
    g_tasks[id].delay = 0;
    SREG = sreg;
}

static inline void
ProcessTaskQueue()
{
    for (TaskId id = 0; id < MAX_TASKS; id++) {
        Task *task = &g_tasks[id];
        if (task->delay != 0) {
            task->delay--;
            if (task->delay == 0) {
                /* Temporarily reserve this task entry. */
                task->delay = 1;
                task->delay = task->handler();
            }
        }
    }
}

static inline u32
GetClockTicks()
{
    u8 sreg = SREG;
    cli();
    u32 ret = g_clockTicks;
    SREG = sreg;
    return ret;
}

/** Called on each second. */
static inline void
OnClockSecond()
{
    g_clockSec++;
    if (g_clockSec < 60) {
        return;
    }
    g_clockSec = 0;
    g_clockMin++;
    if (g_clockMin >= 60) {
        g_clockMin = 0;
        g_clockHour++;
        if (g_clockHour >= 24) {
            g_clockHour = 0;
            g_clockDow++;
            if (g_clockDow >= 7) {
                g_clockDow = 0;
            }
        }
    }
    OnClockMinute();
}

/** Clock ticks occur with TICK_FREQ frequency. */
static inline void
OnClockTick()
{
    static u8 divisor = TICK_FREQ;

    g_clockTicks++;
    divisor--;
    if (divisor == 0) {
        divisor = TICK_FREQ;
        OnClockSecond();
    }
    ProcessTaskQueue();
}

ISR(TIMER1_COMPA_vect)
{
    OnClockTick();
}

static inline void
ClockInit()
{
    /* CLK / 8
     * WGM = 0100 (CTC)
     */
    TCCR1B = _BV(CS11) | _BV(WGM12);
    OCR1A = CLOCK_MAX_VALUE;
    TIMSK1 = _BV(OCIE1A);
}

/* ****************************************************************************/
/* ADC */

#define ADC_IN_BATTERY  _BV(MUX0) //XXX revise
#define ADC_IN_TEMP     _BV(MUX3)

#define ADC_IN_MASK     (ADC_IN_BATTERY | ADC_IN_TEMP)

static inline void
AdcInit()
{
    /* Internal 1.1V reference. */
    ADMUX = _BV(REFS1);
    ADCSRA = _BV(ADEN) | _BV(ADIE) | _BV(ADPS0) | _BV(ADPS1) | _BV(ADPS2);
    DIDR0 = _BV(ADC1D);
}

static u8 g_adcPending, g_adcCurrent, g_adcSkip;

static inline u8
AdcSleepDisabled()
{
    u8 sreg = SREG;
    cli();
    u8 result = g_adcCurrent != 0 || g_adcPending != 0;
    SREG = sreg;
    return result;
}

static inline void
_StartConvertion(u8 input)
{
    ADMUX = (ADMUX & ~ADC_IN_MASK) | input;
    AVR_BIT_SET8(ADCSRA, ADSC);
}

static inline void
AdcStart(u8 input)
{
    u8 sreg = SREG;
    cli();

    if (g_adcCurrent == 0) {
        g_adcSkip = 2;
        g_adcCurrent = input;
        _StartConvertion(input);
    } else {
        g_adcPending = input;
    }

    SREG = sreg;
}

ISR(ADC_vect)
{
    if (g_adcSkip) {
        g_adcSkip--;
    } else {
        OnAdcResult(g_adcCurrent, (u16)ADCL | ((u16)ADCH << 8));
        g_adcCurrent = g_adcPending;
        g_adcPending = 0;
    }
    if (g_adcCurrent) {
        _StartConvertion(g_adcCurrent);
    }
}

/* ****************************************************************************/
/* Temperature measurement. */

static struct {
    /** Accumulated samples since last launch. */
    u16 accumulated;
    /** Calibration offset for 0 C. */
    u16 calOffset:10,
        pending:1,
        :5,
        curTempRaw;
    /** Calibration factor. 0x80 is 1.0, 0xff is 1.996 */
    u8 calFactor;
    u8 curTemp;
} g_temp;

static inline void
TempInit()
{
    //XXX
//    g_temp.calOffset = eeprom_read_word(&e_tempCalOffset);
//    g_temp.calFactor = eeprom_read_byte(&e_tempCalFactor);
}

static inline void
TempFeedValue(u16 value)
{
    if (!g_temp.pending) {
        return;
    }
    g_temp.pending = 0;

    g_temp.curTempRaw = value;
    i32 temp = value - g_temp.calOffset;
    /* Celsius * 2. */
    temp = ((temp * g_temp.calFactor) >> 6) + 100;
    /* Base is -50C */
    g_temp.curTemp = temp;

    i16 delta = (temp - 100) / 2;
    if (delta < 0) {
        return;
    }
    if ((u32)g_temp.accumulated + delta >= 0xffff) {
        g_temp.accumulated = 0xffff;
    } else {
        g_temp.accumulated += delta;
    }
}

/* ****************************************************************************/
/* Button control. */

#define BTN_JITTER_DELAY    TASK_DELAY_MS(250)
#define BTN_LONG_DELAY      TASK_DELAY_MS(1500)

static u16
BtnPoll()
{
    static u8 pressCnt;

    /* Active is pull to ground. */
    if (AVR_BIT_GET8(AVR_REG_PIN(BUTTON_PORT), BUTTON_PIN)) {
        if (pressCnt >= BTN_JITTER_DELAY && pressCnt < BTN_LONG_DELAY) {
            OnButtonPressed();
        }
        pressCnt = 0;
        return 1;
    }
    if (pressCnt >= BTN_LONG_DELAY) {
        return 1;
    }
    pressCnt++;
    if (pressCnt == BTN_LONG_DELAY) {
        OnButtonLongPressed();
    }
    return 1;
}

static inline void
BtnInit()
{
    /* Enable pull-up resistor on button pin. */
    AVR_BIT_SET8(AVR_REG_PORT(BUTTON_PORT), BUTTON_PIN);
    ScheduleTask(BtnPoll, 1);
}

/* ****************************************************************************/
/* XXX */

static inline void
OnAdcResult(u8 type __UNUSED, u16 value __UNUSED)
{
    //XXX
//    if (type == ADC_IN_BATTERY) {
//        g_wat.curBat = value;
//        if (g_temp.pending) {
//            AdcStart(ADC_IN_TEMP);
//        }
//    } else {
//        TempFeedValue(value);
//    }
}

static inline void
OnClockMinute()
{
    //XXX
//    g_temp.pending = 1;
//    AdcStart(ADC_IN_BATTERY);
//    if (g_wat.lastLaunch != 0xffff) {
//        g_wat.lastLaunch++;
//    }
//
//    /* Check if launch should be initiated. */
//    if ((g_wat.setDowMask & ((u8)1 << g_clockDow)) == 0) {
//        return;
//    }
//    if (g_wat.setHour != g_clockHour || g_wat.setMin != g_clockMin) {
//        return;
//    }
//
//    /* Skip blocked launch. */
//    if (g_wat.blocked) {
//        g_wat.blocked = 0;
//        g_wat.lastLaunch = 0;
//        g_temp.accumulated = 0;
//        return;
//    }
//
//    WatLaunch(0);
}

static inline void
OnButtonPressed()
{
    //XXX
}

static inline void
OnButtonLongPressed()
{
    //XXX
}

int
main(void)
{
    AVR_BIT_SET8(DDRB, 1);
    AVR_BIT_SET8(PORTB, 1);
    while (1) {

    }
//    AdcInit();
//    ClockInit();
//    TempInit();
//    BtnInit();
//
//    sei();
//    while (1) {
//        if (!AdcSleepDisabled()) {
//            AVR_BIT_SET8(MCUCR, SE);
//            __asm__ volatile ("sleep");
//            AVR_BIT_CLR8(MCUCR, SE);
//        }
//    };
}
