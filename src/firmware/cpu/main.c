/* This file is a part of 'hydroponics' project.
 * Copyright (c) 2015, Artyom Lebedev <artyom.lebedev@gmail.com>
 * All rights reserved.
 * See COPYING file for copyright details.
 */
#include <adk.h>
#include "cpu.h"

static inline void
OnAdcResult(u8 type, u16 value);

static inline void
OnButtonPressed();

static inline void
OnButtonLongPressed();

/** Invoked when rotary encoder rotated on one tick.
 *
 * @param dir CW direction when TRUE, CCW when FALSE.
 */
static void
OnRotEncTick(u8 dir);

/** Global bit-field variables for saving RAM space. Should be accessed with
 * interrupts disabled.
 */
static struct {
    /** Measurement in progress. */
    u8 lvlGaugeActive:1;
} g;

/* ****************************************************************************/
/* System clock and scheduler. */

/** System clock ticks. Use @ref GetClockTicks() to get the current value. */
static u32 g_clockTicks = 0;

typedef struct {
    /** Non-zero if scheduled. */
    u16 delay;
    TaskHandler handler;
} Task;

static Task g_tasks[MAX_TASKS];
/** Number of scheduler ticks passed from previous tasks run. */
static u16 g_schedulerTicks;

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

/** Process scheduled tasks.
 * @return TRUE if needs additional run.
 */
static inline u8
SchedulerPoll()
{
    cli();
    u16 ticks = g_schedulerTicks;
    g_schedulerTicks = 0;
    sei();

    if (ticks == 0) {
        return FALSE;
    }

    u8 ret = FALSE;
    u8 pendingSkipped = FALSE;
    for (TaskId id = 0; id < MAX_TASKS; id++) {
        Task *task = &g_tasks[id];

        if (task->delay != 0) {
            if (task->delay <= ticks) {
                /* Preserve delay during the call to reserve this entry. */
                task->delay = task->handler();
                if (task->delay != 0) {
                    pendingSkipped = TRUE;
                }
                /* Update ticks counter after each task run. */
                cli();
                if (pendingSkipped && g_schedulerTicks != 0) {
                    ret = TRUE;
                }
                ticks += g_schedulerTicks;
                g_schedulerTicks = 0;
                sei();
            } else {
                task->delay -= ticks;
                pendingSkipped = TRUE;
            }
        }
    }
    return ret;
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

/** Clock ticks occur with TICK_FREQ frequency. */
ISR(TIMER0_OVF_vect)
{
    g_clockTicks++;
    g_schedulerTicks++;
}

static inline void
ClockInit()
{
    /* CLK / 1024, Normal mode. */
    TCCR0B = _BV(CS02) | _BV(CS00);
    TIMSK0 = _BV(TOIE0);
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
    return g_adcCurrent != 0 || g_adcPending != 0;
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
        g_adcPending = FALSE;
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
/* Rotary encoder.
 * Suppress jitter and decode rotation direction.
 */

#define ROT_ENC_JITTER_DELAY    TASK_DELAY_MS(150)

struct {
    /** Current filtered state of line A. */
    u8 curStateA:1,
    /** Current filtered state of line B. */
       curStateB:1;
} g_re;

/** Check if line A has state changed.
 *
 * @return TRUE if state changed.
 */
static inline u8
RotEncCheckA()
{
    static u8 cnt;
    u8 pin = AVR_BIT_GET8(AVR_REG_PIN(ROT_ENC_A_PORT), ROT_ENC_A_PIN) ? 1 : 0;
    if (pin != g_re.curStateA) {
        if (cnt >= ROT_ENC_JITTER_DELAY) {
            cnt = 0;
            g_re.curStateA = pin;
            return TRUE;
        }
        cnt++;
        return FALSE;
    }
    cnt = 0;
    return FALSE;
}

/** Check if line B has state changed.
 *
 * @return TRUE if state changed.
 */
static inline u8
RotEncCheckB()
{
    static u8 cnt;
    u8 pin = AVR_BIT_GET8(AVR_REG_PIN(ROT_ENC_B_PORT), ROT_ENC_B_PIN) ? 1 : 0;
    if (pin != g_re.curStateB) {
        if (cnt >= ROT_ENC_JITTER_DELAY) {
            cnt = 0;
            g_re.curStateB = pin;
            return TRUE;
        }
        cnt++;
        return FALSE;
    }
    cnt = 0;
    return FALSE;
}

static u16
RotEncPoll()
{
    if (RotEncCheckA()) {
        OnRotEncTick(g_re.curStateA != g_re.curStateB);
    }
    if (RotEncCheckB()) {
        OnRotEncTick(g_re.curStateA == g_re.curStateB);
    }
    return 1;
}

static inline void
RotEncInit()
{
    /* Enable pull-up resistor on signal lines. */
    AVR_BIT_SET8(AVR_REG_PORT(ROT_ENC_A_PORT), ROT_ENC_A_PIN);
    AVR_BIT_SET8(AVR_REG_PORT(ROT_ENC_B_PORT), ROT_ENC_B_PIN);
    g_re.curStateA = AVR_BIT_GET8(AVR_REG_PIN(ROT_ENC_A_PORT), ROT_ENC_A_PIN) ? 1 : 0;
    g_re.curStateB = AVR_BIT_GET8(AVR_REG_PIN(ROT_ENC_B_PORT), ROT_ENC_B_PIN) ? 1 : 0;
    ScheduleTask(RotEncPoll, 1);
}

/* ****************************************************************************/
/* Level gauge. */

/** Invoked when measurement complete.
 * @param value xxx
 */
static void
OnLvlGaugeResult(u16 value);

ISR(TIMER1_OVF_vect)
{
    if (!g.lvlGaugeActive) {
        /* No active measurement. */
        return;
    }
    g.lvlGaugeActive = FALSE;
    /* Indicate out-of-range. */
    OnLvlGaugeResult(0xffff);
}

ISR(TIMER1_CAPT_vect)
{
    if (!g.lvlGaugeActive) {
        /* No active measurement. */
        return;
    }
    g.lvlGaugeActive = FALSE;
    OnLvlGaugeResult(TCNT1);
}

static inline void
LvlGaugeInit()
{
    /* Running at system clock frequency, normal mode. Input capture for falling
     * edge.
     */
    TIMSK1 = _BV(ICIE1) | _BV(TOIE1);
    AVR_BIT_SET8(AVR_REG_DDR(LVL_GAUGE_TRIG_PORT), LVL_GAUGE_TRIG_PIN);

    /* Workaround for strange behaviour when echo output is initially high
     * forever. Make one trigger pulse.
     */
    AVR_BIT_SET8(AVR_REG_PORT(LVL_GAUGE_TRIG_PORT), LVL_GAUGE_TRIG_PIN);
    _delay_us(10);
    AVR_BIT_CLR8(AVR_REG_PORT(LVL_GAUGE_TRIG_PORT), LVL_GAUGE_TRIG_PIN);
}

/** Start level measurement. */
static void
LvlGaugeStart()
{
    u8 sreg = SREG;
    cli();

    if (g.lvlGaugeActive) {
        /* Measurement in progress. Drop the new request. */
        return;
    }
    g.lvlGaugeActive = TRUE;

    /* Skip previous echo if active. */
    while (AVR_BIT_GET8(AVR_REG_PIN(LVL_GAUGE_ECHO_PORT), LVL_GAUGE_ECHO_PIN));
    /* Set trigger line and poll for echo start. */
    AVR_BIT_SET8(AVR_REG_PORT(LVL_GAUGE_TRIG_PORT), LVL_GAUGE_TRIG_PIN);
    while (!AVR_BIT_GET8(AVR_REG_PIN(LVL_GAUGE_ECHO_PORT), LVL_GAUGE_ECHO_PIN));

    /* Echo pulse started. Start counting. */
    TCNT1 = 0;
    /* Reset pending counter interrupts if any. */
    TIFR1 = _BV(ICF1) | _BV(TOV1);

    /* Clear trigger line. */
    AVR_BIT_CLR8(AVR_REG_PORT(LVL_GAUGE_TRIG_PORT), LVL_GAUGE_TRIG_PIN);

    SREG = sreg;
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

static void
OnLvlGaugeResult(u16 value __UNUSED)
{
    //LvlGaugeStart();//XXX
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

void
OnRotEncTick(u8 dir __UNUSED)
{
    //XXX
}

u16
Test()
{
    PINB = 0x10;
    LvlGaugeStart();//XXX
    return TASK_DELAY_MS(500);
}

int
main(void)
{
    ClockInit();
    LvlGaugeInit();
    BtnInit();
    RotEncInit();

    //XXX
    DDRB |= 0x1e;
    PORTB |= 0x1e;
    ScheduleTask(Test, TASK_DELAY_MS(500));

    sei();
    while (1) {
        u8 noSleep = SchedulerPoll();
        //XXX rest polls here
        /* Assuming ADC conversion can not be started from interrupt. */
        if (!noSleep && !AdcSleepDisabled()) {
            AVR_BIT_SET8(MCUCR, SE);
            __asm__ volatile ("sleep");
            AVR_BIT_CLR8(MCUCR, SE);
        }
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
