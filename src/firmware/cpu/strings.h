/* This file is a part of 'hydroponics' project.
 * Copyright (c) 2015, Artyom Lebedev <artyom.lebedev@gmail.com>
 * All rights reserved.
 * See LICENSE file for copyright details.
 */

/** @file strings.h
 * Strings to place in program memory (flash).
 */

#ifndef STRINGS_H_
#define STRINGS_H_

/** Define string for placing in program memory. */
#define DEF_STR(__name, __text) \
    const char __name[sizeof(__text)] = __text;

class Strings {
public:
    /** Convert number to string. Applicable to time number only (two digits,
     * 0-59 range).
     */
    static void
    StrClockNum(u8 value, char *buf);

    DEF_STR(NoValue, "<no value>")
    DEF_STR(OK, "OK")
    DEF_STR(Cancel, "Cancel")
    DEF_STR(Fine, " Fine ")
    DEF_STR(LightControl, "Light control")
    DEF_STR(PumpControl, "Pump control")
    DEF_STR(LvlGaugeStatus, "Level gauge status")
    DEF_STR(LvlGaugeClbMin, "Level gauge min.")
    DEF_STR(LvlGaugeClbMax, "Level gauge max.")
    DEF_STR(LightSensorA, "Light sensor A")
    DEF_STR(LightSensorB, "Light sensor B")
    DEF_STR(FloodingPumpThrottle, "Pump throttle")
    DEF_STR(FloodingPumpBoostThrottle, "Pump boost throttle")
    DEF_STR(FloodingMinSunriseTime, "Min. sunrise time")
    DEF_STR(FloodingFirstFloodDelay, "First flood delay")
    DEF_STR(FloodingFloodDuration, "Flooding duration")
    DEF_STR(FloodingFloodPeriod, "Flooding period")
    DEF_STR(FloodingMaxSunsetTime, "Max. sunset time")
    DEF_STR(TimeSetup, "Setup time")

    DEF_STR(FlooderStatus_Idle, "Idle")
    DEF_STR(FlooderStatus_Flooding, "Flooding")
    DEF_STR(FlooderStatus_FloodWait, "Flooding delay")
    DEF_STR(FlooderStatus_FloodFinal, "Flooding final run")
    DEF_STR(FlooderStatus_Draining, "Draining")
    DEF_STR(FlooderStatus_Failure, "Failure")

    DEF_STR(FlooderError_LowWater, "Too low water for flooding")

    /* Menus */
    DEF_STR(MainMenu,
            "Return\0"
            "Manual control\0"
            "Calibration\0"
            "Setup\0"
            "Status\0")

    DEF_STR(ManualControlMenu,
            "Return\0"
            "Light\0"
            "Pump\0"
            "Start flooding\0")

    DEF_STR(CalibrationMenu,
            "Return\0"
            "Light\0"
            "Level gauge\0"
            "Temperature\0")

    DEF_STR(SetupMenu,
            "Return\0"
            "Time\0"
            "Flooding\0"
            "Lighting\0")

    DEF_STR(StatusMenu,
            "Return\0"
            "Level gauge\0"
            "Light sensor A\0"
            "Light sensor B\0"
            "Temperature\0")

    DEF_STR(LvlGaugeCalibrationMenu,
            "Return\0"
            "Min. value\0"
            "Max. value\0")

    DEF_STR(FloodingSetupMenu,
            "Return\0"
            "Pump throttle\0"
            "Pump boost throttle\0"
            "Min. sunrise time\0"
            "First flood delay\0"
            "Flood duration\0"
            "Flood period\0"
            "Max. sunset time\0")

} __PACKED;

extern const Strings strings PROGMEM;

#endif /* STRINGS_H_ */
