/* This file is a part of 'hydroponics' project.
 * Copyright (c) 2015, Artyom Lebedev <artyom.lebedev@gmail.com>
 * All rights reserved.
 * See COPYING file for copyright details.
 */

#include "cpu.h"

using namespace adk;

const Strings strings PROGMEM;

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
        AVR_BIT_SET8(PCICR, PCIE2);
        AVR_BIT_SET8(PCMSK2, PCINT16);
        AVR_BIT_SET8(PCMSK2, PCINT17);
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
ISR(PCINT2_vect)
{
    rotEnc.HandlePinChangeInterrupt();
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
    TCCR2A = _BV(WGM20) |

#       if PWM1_ENABLED
            _BV(COM2A1) |
#           if PWM1_INVERSE
                _BV(COM2A0) |
#           else
                0 |
#           endif
#       else
            0 |
#       endif

#       if PWM2_ENABLED
            _BV(COM2B1) |
#           if PWM2_INVERSE
                _BV(COM2B0)
#           else
                0
#           endif
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

bool
SleepEnabled()
{
    return adc.SleepEnabled();
}

void
adk::PollFunc()
{
    rotEnc.Poll();
    i2cBus.Poll();
    adc.Poll();
    rtc.Poll();
    display.Poll();
    textWriter.Poll();
    bitmapWriter.Poll();
    app.Poll();
}

int
main(void)
{
    BtnInit();
    PwmInit();

    //XXX light sensor A
    DDRD |= 1 << 5;
    //XXX light sensor B
    DDRB |= 1 << 2;

    led.Initialize();
    rtc.Initialize();
    sound.Initialize();
    display.Initialize();
    display.Clear();
    lvlGauge.Enable();
    light.Enable();
    app.Initialize();

    scheduler.Run();
}
