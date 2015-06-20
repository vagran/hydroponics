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

private:
    /** Image of the chip registers. */
    struct Registers {
        u8 sec_low:4,
           sec_hi:3,
           :1,

           min_low:4,
           min_hi:3,
           :1,

           hour_low:4,
           hour_10:1,
           hour_20_ampm:1,
           _12_24:1,
           :1;

        u8 tmp[16];//XXX
    } __PACKED;

    /** Bit-mask for pending writes for the first 16 locations (temperature
     * word is excluded, it is read-only anyway.
     */
    u16 writeMask;

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

} __PACKED;

extern Rtc rtc;

#endif /* RTC_H_ */
