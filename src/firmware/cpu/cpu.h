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

/** Clock ticks frequency. */
#define TICK_FREQ       (ADK_MCU_FREQ / 1024 / 256)
/** Clock tick period in ms. */
#define TICK_PERIOD_MS  (1000 / TICK_FREQ)

/** Convert ticks to miliseconds. */
#define TICK_TO_MS(__t) ((u32)(__t) * 1024 * 256 / (ADK_MCU_FREQ / 1000))
/** Convert ticks to seconds. */
#define TICK_TO_S(__t) ((u32)(__t) * 1024 * 256 / ADK_MCU_FREQ)

/** Scheduled task handler. Returns non-zero delay to reschedule task with or
 * zero to terminate the task.
 */
typedef u16 (*TaskHandler)();

typedef u8 TaskId;

/** Maximal number of tasks allowed to schedule. */
#define MAX_TASKS       10
#define INVALID_TASK    0xff

/* Minimize significant bits loss in the calculations below by using proper
 * calculations sequence.
 */
#define TASK_DELAY_MS(__ms) \
    ((u16)((u32)(ADK_MCU_FREQ / 1000) * (__ms) / ((u32)1024 * 256)))
#define TASK_DELAY_S(__s) \
    ((u16)((u32)(ADK_MCU_FREQ / 256) * (__s) / 1024 ))

/** Schedule task for deferred execution.
 *
 * @return Task ID. @ref INVALID_TASK if task scheduling failed.
 */
TaskId
ScheduleTask(TaskHandler handler, u16 delay);

/** Cancel previously scheduled task. Use only for periodic tasks otherwise
 * race may occur (unscheduling other task which  occupied the same slot).
 */
void
UnscheduleTask(TaskId id);

#endif /* CPU_H_ */
