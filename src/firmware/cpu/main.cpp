/* This file is a part of 'hydroponics' project.
 * Copyright (c) 2015, Artyom Lebedev <artyom.lebedev@gmail.com>
 * All rights reserved.
 * See COPYING file for copyright details.
 */
#include "cpu.h"

using namespace adk;

const Strings strings PROGMEM;

static inline void
OnAdcResult(u8 type, u16 value);

/** Global bit-field variables for saving RAM space. Should be accessed with
 * interrupts disabled.
 */
static struct {
    /** Measurement in progress. */
    u8 lvlGaugeActive:1,

    /* Failure flags. */
    /** Level gauge failure. */
       failLvlGauge:1;
} g;

/* ****************************************************************************/
/* System clock and scheduler. */

Clock clock;
adk::Scheduler scheduler;

/** Clock ticks occur with TICK_FREQ frequency. */
ISR(TIMER0_OVF_vect)
{
    clock.Tick();
}

Clock::Clock()
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
bool
AdcSleepEnabled()
{
    return g_adcCurrent == 0 && g_adcPending == 0;
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
    AtomicSection as;

    if (g_adcCurrent == 0) {
        g_adcSkip = 2;
        g_adcCurrent = input;
        _StartConvertion(input);
    } else {
        g_adcPending = input;
    }
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

#define BTN_JITTER_DELAY    TASK_DELAY_MS(100)
#define BTN_LONG_DELAY      TASK_DELAY_MS(1000)

static u16
BtnPoll()
{
    static u8 pressCnt;

    /* Active is pull to ground. */
    if (AVR_BIT_GET8(AVR_REG_PIN(BUTTON_PORT), BUTTON_PIN)) {
        if (pressCnt >= BTN_JITTER_DELAY && pressCnt < BTN_LONG_DELAY) {
            app.OnButtonPressed();
        }
        pressCnt = 0;
        return 1;
    }
    if (pressCnt >= BTN_LONG_DELAY) {
        return 1;
    }
    pressCnt++;
    if (pressCnt == BTN_LONG_DELAY) {
        app.OnButtonLongPressed();
    }
    return 1;
}

static inline void
BtnInit()
{
    /* Enable pull-up resistor on button pin. */
    AVR_BIT_SET8(AVR_REG_PORT(BUTTON_PORT), BUTTON_PIN);
    scheduler.ScheduleTask(BtnPoll, 1);
}

/* ****************************************************************************/
/* Rotary encoder. */

class RotEnc {
public:
    enum {
        /** Anti-jittering delay in timer 0 ticks. */
        ROT_ENC_JITTER_DELAY = 8
    };

    RotEnc()
    {
        /* Lines initial state is high since RE has both lines open in stable
         * position between clicks.
         */
        curA = 1;
        curB = 1;
        /* Enable pull-up resistor on signal lines. */
        AVR_BIT_SET8(AVR_REG_PORT(ROT_ENC_A_PORT), ROT_ENC_A_PIN);
        AVR_BIT_SET8(AVR_REG_PORT(ROT_ENC_B_PORT), ROT_ENC_B_PIN);
        /* Use pin-change interrupts for rotary encoder processing. */
        AVR_BIT_SET8(PCICR, PCIE1);
        AVR_BIT_SET8(PCMSK1, PCINT8);
        AVR_BIT_SET8(PCMSK1, PCINT9);
    }

    /** Filter jittering on line A. */
    void
    HandleLineAInterrupt()
    {
        u8 pin = AVR_BIT_GET8(AVR_REG_PIN(ROT_ENC_A_PORT), ROT_ENC_A_PIN) ? 1 : 0;
        if (pin != curA) {
            curA = pin;
            stepCount += pin != curB ? 1 : -1;
            CheckClick();
        }
        AVR_BIT_CLR8(TIMSK0, OCIE0A);
        pendingA = false;
    }

    /** Filter jittering on line B. */
    void
    HandleLineBInterrupt()
    {
        u8 pin = AVR_BIT_GET8(AVR_REG_PIN(ROT_ENC_B_PORT), ROT_ENC_B_PIN) ? 1 : 0;
        if (pin != curB) {
            curB = pin;
            stepCount += pin == curA ? 1 : -1;
            CheckClick();
        }
        AVR_BIT_CLR8(TIMSK0, OCIE0B);
        pendingB = false;
    }

    /** Pin change interrupt may be shared with other periphery so do not make
     * any assumptions about this.
     */
    void
    HandlePinChangeInterrupt()
    {
        CheckLines();
        /* Ensure line state is not missed while processing the interrupt. */
        linesCheckPending = true;
        scheduler.SchedulePoll();
    }

    void
    Poll()
    {
        AtomicSection as;
        if (linesCheckPending) {
            CheckLines();
            linesCheckPending = false;
        }
    }
private:
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

    /** Check if full click performed. */
    void
    CheckClick()
    {
        if (curA && curB) {
            /* Stable position between clicks. Check if full click performed. */
            i8 dir;
            if (stepCount >= 4) {
                dir = 1;
            } else if (stepCount <= -4) {
                dir = -1;
            } else {
                dir = 0;
            }
            stepCount = 0;
            if (dir != 0) {
                app.OnRotEncClick(dir > 0);
            }
        }
    }

    void
    CheckLines()
    {
        u8 pin = AVR_BIT_GET8(AVR_REG_PIN(ROT_ENC_A_PORT), ROT_ENC_A_PIN) ? 1 : 0;
        if (pin != curA) {
            if (!pendingA) {
                /* Schedule anti-jittering delay. */
                OCR0A = TCNT0 + ROT_ENC_JITTER_DELAY;
                TIFR0 = _BV(OCF0A);
                AVR_BIT_SET8(TIMSK0, OCIE0A);
                pendingA = true;
            }
        } else {
            pendingA = false;
            AVR_BIT_CLR8(TIMSK0, OCIE0A);
        }

        pin = AVR_BIT_GET8(AVR_REG_PIN(ROT_ENC_B_PORT), ROT_ENC_B_PIN) ? 1 : 0;
        if (pin != curB) {
            if (!pendingB) {
                /* Schedule anti-jittering delay. */
                OCR0B = TCNT0 + ROT_ENC_JITTER_DELAY;
                TIFR0 = _BV(OCF0B);
                AVR_BIT_SET8(TIMSK0, OCIE0B);
                pendingB = true;
            }
        } else {
            pendingB = false;
            AVR_BIT_CLR8(TIMSK0, OCIE0B);
        }
    }
} __PACKED;

RotEnc rotEnc;

ISR(TIMER0_COMPA_vect)
{
    rotEnc.HandleLineAInterrupt();
}

ISR(TIMER0_COMPB_vect)
{
    rotEnc.HandleLineBInterrupt();
}

// May be used also for other events.
ISR(PCINT1_vect)
{
    rotEnc.HandlePinChangeInterrupt();
}

/* ****************************************************************************/
/* Level gauge. */

static u16 g_lvlGaugeResult;

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
    g_lvlGaugeResult = ICR1;
    scheduler.SchedulePoll();
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

static inline void
LvlGaugePoll()
{
    cli();
    u16 value = g_lvlGaugeResult;
    g_lvlGaugeResult = 0;
    sei();
    if (value == 0) {
        return;
    }
    OnLvlGaugeResult(value);
}

/** Start level measurement. */
static void
LvlGaugeStart()
{
    AtomicSection as;

    if (g.lvlGaugeActive) {
        /* Measurement in progress. Drop the new request. */
        return;
    }
    g.lvlGaugeActive = TRUE;

    if (AVR_BIT_GET8(AVR_REG_PIN(LVL_GAUGE_ECHO_PORT), LVL_GAUGE_ECHO_PIN)) {
        /* Previous echo still active. Probably sensor failure. */
        g.failLvlGauge = TRUE;
        return;
    }
    /* Set trigger line and poll for echo start. */
    AVR_BIT_SET8(AVR_REG_PORT(LVL_GAUGE_TRIG_PORT), LVL_GAUGE_TRIG_PIN);
    while (!AVR_BIT_GET8(AVR_REG_PIN(LVL_GAUGE_ECHO_PORT), LVL_GAUGE_ECHO_PIN));

    /* Echo pulse started. Start counting. */
    TCNT1 = 0;
    /* Reset pending counter interrupts if any. */
    TIFR1 = _BV(ICF1) | _BV(TOV1);

    /* Clear trigger line. */
    AVR_BIT_CLR8(AVR_REG_PORT(LVL_GAUGE_TRIG_PORT), LVL_GAUGE_TRIG_PIN);
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

u16
Test()
{
    PINB = 0x10;
    LvlGaugeStart();//XXX
    return TASK_DELAY_MS(500);
}

void
adk::PollFunc()
{
    rotEnc.Poll();
    LvlGaugePoll();
    i2cBus.Poll();
    display.Poll();
    textWriter.Poll();
    bitmapWriter.Poll();
    app.Poll();
}

int
main(void)
{
    LvlGaugeInit();
    BtnInit();
    PwmInit();
    display.Initialize();
    display.Clear();

    //XXX
    DDRB |= 0x1e;

    scheduler.Run();
}
