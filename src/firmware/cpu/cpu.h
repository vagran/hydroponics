/* This file is a part of 'hydroponics' project.
 * Copyright (c) 2015, Artyom Lebedev <artyom.lebedev@gmail.com>
 * All rights reserved.
 * See COPYING file for copyright details.
 */
#ifndef CPU_H_
#define CPU_H_

//XXX revise
/** Port for button. */
#define BUTTON_PORT   B
/** Pin for button. */
#define BUTTON_PIN    3

/** Port for level gauge trigger line. */
#define LVL_GAUGE_TRIG_PORT     D
/** Pin for level gauge trigger line. */
#define LVL_GAUGE_TRIG_PIN      7
/** Port for level gauge echo line. Limited to input capture unit pins. */
#define LVL_GAUGE_ECHO_PORT     B
/** Pin for level gauge echo line. */
#define LVL_GAUGE_ECHO_PIN      0

/** Frequency of clock counter (TCNT1). */
#define CLOCK_FREQ          (ADK_MCU_FREQ / 8)
/** TOP value for clock counter. */
#define CLOCK_MAX_VALUE     62499

/** Clock ticks frequency. */
#define TICK_FREQ       40
/** Clock tick period in ms. */
#define TICK_PERIOD_MS  (1000 / TICK_FREQ)

/** Scheduled task handler. Returns non-zero delay to reschedule task with or
 * zero to terminate the task.
 */
typedef u16 (*TaskHandler)();

typedef u8 TaskId;

/** Maximal number of tasks allowed to schedule. */
#define MAX_TASKS       10
#define INVALID_TASK    0xff

#define TASK_DELAY_MS(ms)   (ms / TICK_PERIOD_MS)
#define TASK_DELAY_S(s)     (s * TICK_FREQ)

TaskId
ScheduleTask(TaskHandler handler, u16 delay);

void
UnscheduleTask(TaskId id);

#endif /* CPU_H_ */
