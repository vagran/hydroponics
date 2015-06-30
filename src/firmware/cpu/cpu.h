/* This file is a part of 'hydroponics' project.
 * Copyright (c) 2015, Artyom Lebedev <artyom.lebedev@gmail.com>
 * All rights reserved.
 * See COPYING file for copyright details.
 */
#ifndef CPU_H_
#define CPU_H_

bool
SleepEnabled();

#define SCHEDULER_CHECK_SLEEPING_ALLOWED SleepEnabled

#define I2C_USE_PULLUP

#include <adk.h>

//XXX revise
/** Port for button. */
#define BUTTON_PORT   D
/** Pin for button. */
#define BUTTON_PIN    2

/** Port for rotary encoder line A. */
#define ROT_ENC_A_PORT   D
/** Pin for rotary encoder line A. */
#define ROT_ENC_A_PIN    1
/** Port for rotary encoder line B. */
#define ROT_ENC_B_PORT   D
/** Pin for rotary encoder line B. */
#define ROT_ENC_B_PIN    0

/** Port for level gauge trigger line. */
#define LVL_GAUGE_TRIG_PORT     D
/** Pin for level gauge trigger line. */
#define LVL_GAUGE_TRIG_PIN      7
/** Port for level gauge echo line. Limited to input capture unit pins. */
#define LVL_GAUGE_ECHO_PORT     B
/** Pin for level gauge echo line. */
#define LVL_GAUGE_ECHO_PIN      0


/** Port for the first PWM channel. */
#define PWM1_PORT               B
/** Pin for the first PWM channel. */
#define PWM1_PIN                3
/** Inverse output for the first PWM channel when TRUE. */
#define PWM1_INVERSE            FALSE
/** Whether the first PWM channel is enabled. */
#define PWM1_ENABLED            0

/** Port for the second PWM channel. */
#define PWM2_PORT               D
/** Pin for the second PWM channel. */
#define PWM2_PIN                3
/** Inverse output for the second PWM channel when TRUE. */
#define PWM2_INVERSE            FALSE
/** Whether the second PWM channel is enabled. */
#define PWM2_ENABLED            1

/** Port for the third (low frequency) PWM channel. */
#define PWM3_PORT               D
/** Pin for the third (low frequency) PWM channel. */
#define PWM3_PIN                4
/** Inverse output for the third (low frequency) PWM channel when TRUE. */
#define PWM3_INVERSE            TRUE

void
Pwm1Set(u8 value);
u8
Pwm1Get();
void
Pwm2Set(u8 value);
u8
Pwm2Get();
void
Pwm3Set(u8 value);
u8
Pwm3Get();


/** Clock ticks frequency. */
#define TICK_FREQ       (ADK_MCU_FREQ / 1024 / 256)
/** Clock tick period in ms. */
#define TICK_PERIOD_MS  (1000 / TICK_FREQ)

/** Convert ticks to miliseconds. */
#define TICK_TO_MS(__t) ((u32)(__t) * 1024 * 256 / (ADK_MCU_FREQ / 1000))
/** Convert ticks to seconds. */
#define TICK_TO_S(__t) ((u32)(__t) * 1024 * 256 / ADK_MCU_FREQ)

extern adk::Scheduler scheduler;

/** System clock ticks. Use @ref Clock::GetTicks() to get the current value. */
class Clock {
public:
    Clock();

    /** Get current value of the system clock. */
    u32
    GetTicks()
    {
        adk::AtomicSection as;
        return ticks;
    }

    /** Should be called from interrupt. */
    void
    Tick()
    {
        ticks++;
        scheduler.Tick();
    }
private:
    u32 ticks;
} __PACKED;

extern Clock clock;

/* Minimize significant bits loss in the calculations below by using proper
 * calculations sequence.
 */
#define TASK_DELAY_MS(__ms) \
    ((u16)((u32)(ADK_MCU_FREQ / 1000) * (__ms) / ((u32)1024 * 256)))
#define TASK_DELAY_S(__s) \
    ((u16)((u32)(ADK_MCU_FREQ / 256) * (__s) / 1024 ))

#include "variant.h"

#include "strings.h"
#include "i2c.h"
#include "adc.h"
#include "rtc.h"
#include "display.h"
#include "text_writer.h"
#include "bitmap.h"
#include "bitmaps.h"
#include "lighting.h"
#include "pump.h"
#include "level_gauge.h"
#include "application.h"

#endif /* CPU_H_ */
