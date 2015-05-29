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
OnRotEncClick(u8 dir);

/** Global bit-field variables for saving RAM space. Should be accessed with
 * interrupts disabled.
 */
static struct {
    /** Any interrupt can set this flag to indicate that scheduler polling round
     * should be executed.
     */
    u8 pollPending:1,
    /** Measurement in progress. */
       lvlGaugeActive:1;
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

u8
ScheduleTask(TaskHandler handler, u16 delay)
{
    u8 sreg = SREG;
    cli();
    for (TaskId id = 0; id < MAX_TASKS; id++) {
        if (g_tasks[id].delay == 0) {
            g_tasks[id].delay = delay;
            g_tasks[id].handler = handler;
            SREG = sreg;
            return TRUE;
        }
    }
    SREG = sreg;
    return FALSE;
}

u8
UnscheduleTask(TaskHandler handler)
{
    u8 sreg = SREG;
    cli();
    for (TaskId id = 0; id < MAX_TASKS; id++) {
        if (g_tasks[id].handler == handler) {
            g_tasks[id].delay = 0;
            SREG = sreg;
            return TRUE;
        }
    }
    SREG = sreg;
    return FALSE;
}

/** Process scheduled tasks. */
static inline void
SchedulerPoll()
{
    cli();
    u16 ticks = g_schedulerTicks;
    g_schedulerTicks = 0;
    sei();

    if (ticks == 0) {
        return;
    }

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
                    /* Schedule additional round instantly. */
                    g.pollPending = TRUE;
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
    g.pollPending = TRUE;
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

/** Prevent MCU from sleeping when ADC conversion in progress (since it will
 * probably stop I/O clock and will fail the conversion).
 */
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
/* Rotary encoder. */

/** Anti-jittering delay in timer 0 ticks. */
#define ROT_ENC_JITTER_DELAY    8

static struct {
    /** Rotary encoder steps counter. Incremented or decremented after each
     * step depending on rotation direction. When full click is rotated the
     * counter is checked. One click is four steps. In case of half-click
     * bidirectional rotation or any wrong signaling the click is not accounted.
     */
    i8 stepCount;
    /** Current filtered state of line A. */
    u8 curA:1,
    /** Current filtered state of line B. */
       curB:1,
    /** Line A change pending. */
       pendingA:1,
   /** Line B change pending. */
       pendingB:1,
    /** Pending lines check in poll function. */
       linesCheckPending:1;
} g_re;

/** Check if full click performed. */
static void
RotEncCheckClick()
{
    if (g_re.curA && g_re.curB) {
        /* Stable position between clicks. Check if full click performed. */
        i8 dir;
        if (g_re.stepCount >= 4) {
            dir = 1;
        } else if (g_re.stepCount <= -4) {
            dir = -1;
        } else {
            dir = 0;
        }
        g_re.stepCount = 0;
        if (dir != 0) {
            OnRotEncClick(dir > 0);
        }
    }
}

/** Filter jittering on line A. */
ISR(TIMER0_COMPA_vect)
{
    u8 pin = AVR_BIT_GET8(AVR_REG_PIN(ROT_ENC_A_PORT), ROT_ENC_A_PIN) ? 1 : 0;
    if (pin != g_re.curA) {
        g_re.curA = pin;
        g_re.stepCount += pin != g_re.curB ? 1 : -1;
        RotEncCheckClick();
    }
    AVR_BIT_CLR8(TIMSK0, OCIE0A);
    g_re.pendingA = FALSE;
}

/** Filter jittering on line B. */
ISR(TIMER0_COMPB_vect)
{
    u8 pin = AVR_BIT_GET8(AVR_REG_PIN(ROT_ENC_B_PORT), ROT_ENC_B_PIN) ? 1 : 0;
    if (pin != g_re.curB) {
        g_re.curB = pin;
        g_re.stepCount += pin == g_re.curA ? 1 : -1;
        RotEncCheckClick();
    }
    AVR_BIT_CLR8(TIMSK0, OCIE0B);
    g_re.pendingB = FALSE;
}

static void
RotEncCheckLines()
{
    u8 pin = AVR_BIT_GET8(AVR_REG_PIN(ROT_ENC_A_PORT), ROT_ENC_A_PIN) ? 1 : 0;
    if (pin != g_re.curA) {
        if (!g_re.pendingA) {
            /* Schedule anti-jittering delay. */
            OCR0A = TCNT0 + ROT_ENC_JITTER_DELAY;
            TIFR0 = _BV(OCF0A);
            AVR_BIT_SET8(TIMSK0, OCIE0A);
            g_re.pendingA = TRUE;
        }
    } else {
        g_re.pendingA = FALSE;
        AVR_BIT_CLR8(TIMSK0, OCIE0A);
    }

    pin = AVR_BIT_GET8(AVR_REG_PIN(ROT_ENC_B_PORT), ROT_ENC_B_PIN) ? 1 : 0;
    if (pin != g_re.curB) {
        if (!g_re.pendingB) {
            /* Schedule anti-jittering delay. */
            OCR0B = TCNT0 + ROT_ENC_JITTER_DELAY;
            TIFR0 = _BV(OCF0B);
            AVR_BIT_SET8(TIMSK0, OCIE0B);
            g_re.pendingB = TRUE;
        }
    } else {
        g_re.pendingB = FALSE;
        AVR_BIT_CLR8(TIMSK0, OCIE0B);
    }
}

static inline void
HandleRotEncInterrupt()
{
    RotEncCheckLines();
    /* Ensure line state is not missed while processing the interrupt. */
    g_re.linesCheckPending = TRUE;
    g.pollPending = TRUE;
}

// May be used also for other events.
ISR(PCINT1_vect)
{
    HandleRotEncInterrupt();
}

static inline void
RotEncPoll()
{
    cli();
    if (g_re.linesCheckPending) {
        RotEncCheckLines();
        g_re.linesCheckPending = FALSE;
    }
    sei();
}

static inline void
RotEncInit()
{
    /* Lines initial state is high since RE has both lines open in stable
     * position between clicks.
     */
    g_re.curA = 1;
    g_re.curB = 1;
    /* Enable pull-up resistor on signal lines. */
    AVR_BIT_SET8(AVR_REG_PORT(ROT_ENC_A_PORT), ROT_ENC_A_PIN);
    AVR_BIT_SET8(AVR_REG_PORT(ROT_ENC_B_PORT), ROT_ENC_B_PIN);
    /* Use pin-change interrupts for rotary encoder processing. */
    AVR_BIT_SET8(PCICR, PCIE1);
    AVR_BIT_SET8(PCMSK1, PCINT8);
    AVR_BIT_SET8(PCMSK1, PCINT9);
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
/* PWM signals generator. */

static u8 pwm3Value;

ISR(TIMER2_OVF_vect)
{

#   define PWM3_CNT_BITS    6
#   define PWM3_CNT_MAX     ((1 << PWM3_CNT_BITS) - 1)

    static struct {
        /* Current value. */
        u8 value: PWM3_CNT_BITS,
        /* Current direction. */
           dir:1;
    } cnt;

    if (cnt.value == (pwm3Value >> (8 - PWM3_CNT_BITS))) {
        if (cnt.value == PWM3_CNT_MAX || cnt.value == 0) {
            if ((cnt.value == PWM3_CNT_MAX && !PWM3_INVERSE) ||
                (cnt.value == 0 && PWM3_INVERSE)) {

                AVR_BIT_SET8(AVR_REG_PORT(PWM3_PORT), PWM3_PIN);
            } else {
                AVR_BIT_CLR8(AVR_REG_PORT(PWM3_PORT), PWM3_PIN);
            }
        } else if (cnt.dir == PWM3_INVERSE) {
            AVR_BIT_SET8(AVR_REG_PORT(PWM3_PORT), PWM3_PIN);
        } else {
            AVR_BIT_CLR8(AVR_REG_PORT(PWM3_PORT), PWM3_PIN);
        }
    }

    if (cnt.dir) {
        if (cnt.value == PWM3_CNT_MAX) {
            cnt.value--;
            cnt.dir = FALSE;
        } else {
            cnt.value++;
        }
    } else {
        if (cnt.value == 0) {
            cnt.value++;
            cnt.dir = TRUE;
        } else {
            cnt.value--;
        }
    }
}

static inline void
PwmInit()
{
    /* Phase correct PWM mode, maximal frequency (~39kHz). */
    TCCR2A = _BV(WGM20) | _BV(COM2A1) |
#       if PWM1_INVERSE
            _BV(COM2A0) |
#       else
            0 |
#       endif
        _BV(COM2B1) |
#       if PWM2_INVERSE
            _BV(COM2B0)
#       else
            0
#       endif
    ;
    TCCR2B = _BV(CS20);
    TIMSK2 = _BV(TOIE2);

    AVR_BIT_SET8(AVR_REG_DDR(PWM1_PORT), PWM1_PIN);
    AVR_BIT_SET8(AVR_REG_DDR(PWM2_PORT), PWM2_PIN);
    AVR_BIT_SET8(AVR_REG_DDR(PWM3_PORT), PWM3_PIN);
}

void
Pwm1Set(u8 value)
{
    OCR2A = value;
}

u8
Pwm1Get()
{
    return OCR2A;
}

void
Pwm2Set(u8 value)
{
    OCR2B = value;
}

u8
Pwm2Get()
{
    return OCR2B;
}

void
Pwm3Set(u8 value)
{
    pwm3Value = value;
}

u8
Pwm3Get()
{
    return pwm3Value;
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
OnRotEncClick(u8 dir __UNUSED)
{
    u8 mask = PORTB & 0x1e;
    if (dir) {
        mask = (mask >> 1) & 0x1e;
        if (mask == 0) {
            mask = 0x10;
        }
    } else {
        mask = (mask << 1) & 0x1e;
        if (mask == 0) {
            mask = 0x2;
        }
    }
    PORTB = (PORTB & ~0x1e) | mask;

    u8 pwm = Pwm3Get();
    if (dir && pwm < 255) {
        pwm++;
    } else if (!dir && pwm > 0) {
        pwm--;
    }
    Pwm3Set(pwm);
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
    PwmInit();

    //XXX
    DDRB |= 0x1e;
    PORTB |= 0x2;
    //ScheduleTask(Test, TASK_DELAY_MS(500));

    while (1) {
        g.pollPending = FALSE;
        sei();

        SchedulerPoll();
        RotEncPoll();

        cli();
        if (!g.pollPending && !AdcSleepDisabled()) {
            AVR_BIT_SET8(MCUCR, SE);
            /* Atomic sleeping. */
            __asm__ volatile ("sei; sleep");
            AVR_BIT_CLR8(MCUCR, SE);
            cli();
        }
    }
}
