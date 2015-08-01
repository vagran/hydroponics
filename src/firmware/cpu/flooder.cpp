/* This file is a part of 'hydroponics' project.
 * Copyright (c) 2012-2015, Artyom Lebedev <artyom.lebedev@gmail.com>
 * All rights reserved.
 * See COPYING file for copyright details.
 */

/** @file flooder.cpp */

#include "cpu.h"

using namespace adk;

Flooder flooder;

u8 EEMEM Flooder::eePumpThrottle = 70,
         Flooder::eePumpBoostThrottle = 120;
Time EEMEM Flooder::eeMinSunriseTime {7, 0},
           Flooder::eeFirstFloodDelay{0, 30},
           Flooder::eeFloodDuration{0, 1},
           Flooder::eeFloodPeriod{24, 0},
           Flooder::eeMaxSunsetTime{22, 0};

Flooder::Flooder()
{
    isDaylight = false;
    sunsetSeen = false;
    dayOfWeek = rtc.GetDayOfWeek();
    status = Status::IDLE;
}

void
Flooder::Initialize()
{
    SchedulePoll();
    scheduler.ScheduleTask(_SchedulePoll, SCHEDULE_POLL_PERIOD);
}

Time
Flooder::GetMinSunriseTime()
{
    Time t;
    eeprom_read_block(&t, &eeMinSunriseTime, sizeof(t));
    return t;
}

void
Flooder::SetMinSunriseTime(Time t)
{
    eeprom_update_block(&t, &eeMinSunriseTime, sizeof(t));
}

Time
Flooder::GetFirstFloodDelay()
{
    Time t;
    eeprom_read_block(&t, &eeFirstFloodDelay, sizeof(t));
    return t;
}

void
Flooder::SetFirstFloodDelay(Time t)
{
    eeprom_update_block(&t, &eeFirstFloodDelay, sizeof(t));
}

Time
Flooder::GetFloodDuration()
{
    Time t;
    eeprom_read_block(&t, &eeFloodDuration, sizeof(t));
    return t;
}

void
Flooder::SetFloodDuration(Time t)
{
    eeprom_update_block(&t, &eeFloodDuration, sizeof(t));
}

Time
Flooder::GetFloodPeriod()
{
    Time t;
    eeprom_read_block(&t, &eeFloodPeriod, sizeof(t));
    return t;
}

void
Flooder::SetFloodPeriod(Time t)
{
    eeprom_update_block(&t, &eeFloodPeriod, sizeof(t));
}

Time
Flooder::GetMaxSunsetTime()
{
    Time t;
    eeprom_read_block(&t, &eeMaxSunsetTime, sizeof(t));
    return t;
}

void
Flooder::SetMaxSunsetTime(Time t)
{
    eeprom_update_block(&t, &eeMaxSunsetTime, sizeof(t));
}

const char *
Flooder::GetStatusString()
{
    switch (status) {
    case Status::IDLE:
        return strings.FlooderStatus_Idle;
    case Status::FLOODING:
        return strings.FlooderStatus_Flooding;
    case Status::FLOOD_WAIT:
        return strings.FlooderStatus_FloodWait;
    case Status::FLOOD_FINAL:
        return strings.FlooderStatus_FloodFinal;
    case Status::DRAINING:
        return strings.FlooderStatus_Draining;
    case Status::FAILURE:
        return strings.FlooderStatus_Failure;
    }
    return strings.NoValue;
}

const char *
Flooder::GetErrorString()
{
    switch (errorCode) {
    case ErrorCode::LOW_WATER:
        return strings.FlooderError_LowWater;
    }
    return strings.NoValue;
}

void
Flooder::StartFlooding()
{
    AtomicSection as;
    if (status != Status::IDLE) {
        return;
    }
    lastWaterLevel = lvlGauge.GetValue();
    if ((lastTopVolume == 0 &&
         lastWaterLevel < static_cast<u16>(MIN_START_WATER) * 255 / 100) ||
        (lastTopVolume != 0 && lastWaterLevel < lastTopVolume + 0x10)) {

        status = Status::FAILURE;
        errorCode = ErrorCode::LOW_WATER;
        return;
    }
    status = Status::FLOODING;
    startLevel = lastWaterLevel;
    floodWaitDone = false;
    siphonReached = false;
    extendedPollDelay = true;

    minLevel = startLevel;
    minLevelUpdated = 0;
    minLevelStayed = 0;
    maxLevel = startLevel;
    maxLevelUpdated = 0;
    maxLevelStayed = 0;

    pump.SetLevel(eeprom_read_byte(&eePumpThrottle));
    scheduler.ScheduleTask(_FloodPoll, POLL_PERIOD);
}

u8
Flooder::GetPumpThrottle()
{
    return eeprom_read_byte(&eePumpThrottle);
}

void
Flooder::SetPumpThrottle(u8 value)
{
    eeprom_update_byte(&eePumpThrottle, value);
}

u8
Flooder::GetPumpBoostThrottle()
{
    return eeprom_read_byte(&eePumpBoostThrottle);
}

void
Flooder::SetPumpBoostThrottle(u8 value)
{
    eeprom_update_byte(&eePumpBoostThrottle, value);
}

u16
Flooder::FloodPoll()
{
    u8 newLevel = lvlGauge.GetValue();

    if (newLevel < minLevel) {
        minLevel = newLevel;
        if (minLevelUpdated < 0xff) {
            minLevelUpdated++;
        }
        minLevelStayed = 0;
    } else {
        if (minLevelStayed < 0xff) {
            minLevelStayed++;
        }
        minLevelUpdated = 0;
    }

    if (newLevel > maxLevel) {
        maxLevel = newLevel;
        if (maxLevelUpdated < 0xff) {
            maxLevelUpdated++;
        }
        maxLevelStayed = 0;
    } else {
        if (maxLevelStayed < 0xff) {
            maxLevelStayed++;
        }
        maxLevelUpdated = 0;
    }

    if (status == Status::FLOODING) {

        if (minLevelStayed > 5 &&
            ((lastTopVolume != 0 && newLevel < startLevel - lastTopVolume / 3) ||
             (newLevel < startLevel - 0x10))) {

            siphonReached = true;
            siphonLevel = minLevel;
            lastTopVolume = startLevel - siphonLevel;
            if (floodWaitDone) {
                status = Status::FLOOD_FINAL;
                minLevel = newLevel;
                minLevelUpdated = 0;
                minLevelStayed = 0;
                extendedPollDelay = true;
                pump.SetLevel(eeprom_read_byte(&eePumpBoostThrottle));
            } else {
                status = Status::FLOOD_WAIT;
                floodWaitDone = true;
                pump.SetLevel(0);
                floodDelayTime = rtc.GetTime().GetTime();
            }
        }

        if (!floodWaitDone && lastTopVolume != 0 &&
            newLevel <= startLevel - (static_cast<u16>(lastTopVolume) * 15 / 16)) {

            status = Status::FLOOD_WAIT;
            floodWaitDone = true;
            pump.SetLevel(0);
            floodDelayTime = rtc.GetTime().GetTime();
        }

    } else if (status == Status::FLOOD_WAIT) {

        Time curTime = rtc.GetTime().GetTime();
        Time delay = GetFloodDuration();
        if (curTime >= floodDelayTime + delay) {
            minLevel = newLevel;
            minLevelUpdated = 0;
            minLevelStayed = 0;
            extendedPollDelay = true;
            if (siphonReached) {
                status = Status::FLOOD_FINAL;
                pump.SetLevel(eeprom_read_byte(&eePumpBoostThrottle));
            } else {
                status = Status::FLOODING;
                pump.SetLevel(eeprom_read_byte(&eePumpThrottle));
            }
        }

    } else if (status == Status::FLOOD_FINAL) {

        if (newLevel > minLevel + lastTopVolume / 5) {
            status = Status::DRAINING;
            maxLevel = newLevel;
            maxLevelStayed = 0;
            maxLevelUpdated = 0;
            pump.SetLevel(0);
        }

    } else if (status == Status::DRAINING) {

        if (maxLevelStayed > 3) {
            status = Status::IDLE;
            lastWaterLevel = newLevel;
            lastFloodTime = rtc.GetTime().GetTime();
            return 0;
        }

    } else {
        return 0;
    }

    lastWaterLevel = newLevel;
    if (extendedPollDelay) {
        extendedPollDelay = false;
        return POLL_PERIOD * 3;
    }
    return POLL_PERIOD;
}

u16
Flooder::_FloodPoll()
{
    return flooder.FloodPoll();
}

u16
Flooder::SchedulePoll()
{
    Time curTime = rtc.GetTime().GetTime();
    u8 newDow = rtc.GetDayOfWeek();
    if (newDow != dayOfWeek) {
        dayOfWeek = newDow;
        sunsetSeen = false;
    }

    /* Detect sunrise. */
    if (!isDaylight && !sunsetSeen) {
        //XXX use light sensors
        Time minSunriseTime = GetMinSunriseTime();
        if (curTime >= minSunriseTime) {
            isDaylight = true;
            isAmbientDaylight = true;
            lastSunriseTime = curTime;
            lastFloodTime = Time{0, 0};
        }
    }

    /* Detect sunset. */
    if (isDaylight) {
        //XXX use light sensors
        Time maxSunsetTime = GetMaxSunsetTime();
        if (curTime >= maxSunsetTime) {
            isDaylight = false;
            isAmbientDaylight = false;
            sunsetSeen = true;
            lastSunsetTime = curTime;
            lastFloodTime = Time{0, 0};
        }
    }

    if (status == Status::IDLE) {
        Time floodTime = GetNextFloodTime();
        if (floodTime && floodTime <= curTime) {
            StartFlooding();
        }
    }

    return SCHEDULE_POLL_PERIOD;
}

u16
Flooder::_SchedulePoll()
{
    return flooder.SchedulePoll();
}

Time
Flooder::GetNextFloodTime()
{
    if (!isDaylight) {
        return Time{0, 0};
    }
    if (!lastFloodTime) {
        return lastSunriseTime + GetFirstFloodDelay();
    }
    Time t = lastFloodTime + GetFloodPeriod();
    if (t > GetMaxSunsetTime()) {
        return Time{0, 0};
    }
    return t;
}

u8
Flooder::GetTopPotWaterLevel()
{
    if (startLevel == 0) {
        return 0;
    }
    if (lastTopVolume == 0) {
        return 0xff - lastWaterLevel;
    }
    if (lastWaterLevel <= startLevel - lastTopVolume) {
        return 0xff;
    }
    u16 inv =
        static_cast<u16>(lastWaterLevel - (startLevel - lastTopVolume)) * 0xff /
        lastTopVolume;
    if (inv >= 0xff) {
        return 0;
    }
    return 0xff - inv;
}
