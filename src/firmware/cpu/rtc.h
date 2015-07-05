/* This file is a part of 'hydroponics' project.
 * Copyright (c) 2012-2015, Artyom Lebedev <artyom.lebedev@gmail.com>
 * All rights reserved.
 * See COPYING file for copyright details.
 */

/** @file rtc.h */

#ifndef RTC_H_
#define RTC_H_

/** Real Time Clock based on DS3231. */
class Rtc {
public:

    enum {
        I2C_ADDRESS = 0b1101000
    };

    struct Time {
        u8 hour, min, sec;
    };

    Rtc();

    void
    Initialize();

    void
    Poll();

    /** Update cached values. */
    void
    Update();

    Time
    GetTime();

    void
    SetTime(Time time);

    /** Set sound status.
     *
     * @param f True to enable beeping.
     */
    void
    SetSound(bool f);

    /** Current temperature reading, fixed point, two LSB - fractional part. */
    i16
    GetTemperature();

private:
    /** Image of the chip registers. */
    struct Registers {
        u8  sec_lo:4,           /* 0x00 */
            sec_hi:3,
            :1,

            min_lo:4,           /* 0x01 */
            min_hi:3,
            :1,

            hour_lo:4,          /* 0x02 */
            hour_10:1,
            hour_20_ampm:1,
            _12_24:1,
            :1,

            dow:3,              /* 0x03 */
            :5,

            date_lo:4,          /* 0x04 */
            date_hi:2,
            :2,

            month_lo:4,         /* 0x05 */
            month_hi:1,
            :2,
            century:1,

            year_lo:4,          /* 0x06 */
            year_hi:4,

            /* Alarms are not used so the fields are not detailed. */
            alarm1_1:8,         /* 0x07 */
            alarm1_2:8,         /* 0x08 */
            alarm1_3:8,         /* 0x09 */
            alarm1_4:8,         /* 0x0A */
            alarm2_1:8,         /* 0x0B */
            alarm2_2:8,         /* 0x0C */
            alarm2_3:8,         /* 0x0D */

            a1ie:1,             /* 0x0E */
            a2ie:1,
            intcn:1,
            rs1:1,
            rs2:1,
            conv:1,
            bbsqw:1,
            eosc:1,

            a1f:1,              /* 0x0F */
            a2f:1,
            bsy:1,
            en32khz:1,
            :3,
            osf:1;

        i8  aging_offset,       /* 0x10 */

            temp_hi;            /* 0x11 */

        u8  :6,                 /* 0x12 */
            temp_lo:2;
    } __PACKED;

    /** Bit-mask for pending writes for the first 16 locations (temperature
     * word is excluded, it is read-only anyway.
     */
    u16 writeMask = 0;

    /** Current address in the RTC. */
    u8 curAddr:5,
    /** Full image read is pending. */
       readPending:1,
       readInProgress:1,
       writeInProgress:1;
    /** Temperature value from last read. */
    u16 temperature:10,
    /** Current address must be transferred before data reading. */
        addrTransferPending:1,
        failure:1,
        :4;

    Registers regs;

    void
    StartWrite();

    void
    StartRead();

    static bool
    _TransferHandler(I2cBus::TransferStatus status, u8 data);

    bool
    TransferHandler(I2cBus::TransferStatus status, u8 data);

    /** Set current address to next pending byte in write mask.
     *
     * @return True if address is not consequential.
     */
    bool
    NextWriteAddr();

} __PACKED;

extern Rtc rtc;

#endif /* RTC_H_ */
