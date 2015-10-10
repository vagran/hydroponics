/* This file is a part of 'hydroponics' project.
 * Copyright (c) 2012-2015, Artyom Lebedev <artyom.lebedev@gmail.com>
 * All rights reserved.
 * See LICENSE file for copyright details.
 */

/** @file rtc.cpp */

#include "cpu.h"

using namespace adk;

Rtc rtc;

Rtc::Rtc()
{
    /* Invalid address to force synchronization on the first access. */
    curAddr = 0x1f;
    readInProgress = false;
    writeInProgress = false;
    failure = false;
}

void
Rtc::Initialize()
{
    regs.a1ie = false;
    regs.a2ie = false;
    regs.intcn = true;
    /* 4kHz */
    regs.rs1 = 1;
    regs.rs2 = 0;
    regs.conv = 1;
    regs.bbsqw = false;
    regs.eosc = false;

    writeMask = 1 << 0xe;
    readPending = true;

    while (true) {
        sei();
        Poll();
        i2cBus.Poll();
        cli();
        {
            AtomicSection as;
            if (!readPending && !readInProgress && !writeInProgress) {
                break;
            }
        }
    }
}

void
Rtc::SetSound(bool f)
{
    AtomicSection as;
    regs.intcn = !f;
    writeMask |= 1 << 0x0e;
}

i16
Rtc::GetTemperature()
{
    AtomicSection as;
    return (static_cast<i16>(regs.temp_hi) << 2) | regs.temp_lo;
}

void
Rtc::Poll()
{
    AtomicSection as;
    if (readInProgress || writeInProgress) {
        return;
    }
    if (writeMask) {
        StartWrite();
    } else if (readPending) {
        readPending = false;
        StartRead();
    }
}

void
Rtc::StartWrite()
{
    writeInProgress = true;
    curAddr = 0;
    NextWriteAddr();
    i2cBus.RequestTransfer(I2C_ADDRESS, true, _TransferHandler);
}

void
Rtc::StartRead()
{
    readInProgress = true;
    if (curAddr != 0) {
        addrTransferPending = true;
        curAddr = 0;
    }
    i2cBus.RequestTransfer(I2C_ADDRESS, addrTransferPending, _TransferHandler);
}

bool
Rtc::_TransferHandler(I2cBus::TransferStatus status, u8 data)
{
    return rtc.TransferHandler(status, data);
}

bool
Rtc::TransferHandler(I2cBus::TransferStatus status, u8 data)
{
    if (readInProgress) {
        if (addrTransferPending) {
            if (status == I2cBus::TransferStatus::TRANSMIT_READY) {
                i2cBus.TransmitByte(curAddr);
            } else if (status == I2cBus::TransferStatus::BYTE_TRANSMITTED) {
                i2cBus.RequestInstantTransfer(I2C_ADDRESS, false, _TransferHandler);
                addrTransferPending = false;
            } else {
                readInProgress = false;
                failure = true;
                return false;
            }
        } else {
            if (status == I2cBus::TransferStatus::RECEIVE_READY) {
                /* Do nothing */
            } else if (status == I2cBus::TransferStatus::BYTE_RECEIVED) {
                /* Do not overwrite pending write bytes. */
                if (curAddr >= 0x10 || !(writeMask & (1 << curAddr))) {
                    reinterpret_cast<u8 *>(&regs)[curAddr] = data;
                }
                curAddr++;
                if (curAddr == 0x12) {
                    i2cBus.Nack();
                }
            } else if (status == I2cBus::TransferStatus::LAST_BYTE_RECEIVED) {
                reinterpret_cast<u8 *>(&regs)[curAddr] = data;
                curAddr++;
                if (curAddr > 0x12) {
                    curAddr = 0;
                }
                readInProgress = false;
                return false;
            } else {
                readInProgress = false;
                failure = true;
                return false;
            }
        }
    } else if (writeInProgress) {
        if (status == I2cBus::TransferStatus::TRANSMIT_READY) {
            i2cBus.TransmitByte(curAddr);
        } else if (status == I2cBus::TransferStatus::BYTE_TRANSMITTED) {
            if (writeMask) {
                if (NextWriteAddr()) {
                    i2cBus.RequestInstantTransfer(I2C_ADDRESS, true, _TransferHandler);
                } else {
                    i2cBus.TransmitByte(reinterpret_cast<u8 *>(&regs)[curAddr]);
                    writeMask &= ~(1 << curAddr);
                    curAddr++;
                }
            } else {
                writeInProgress = false;
                return false;
            }
        } else {
            writeInProgress = false;
            failure = true;
            return false;
        }
    } else {
        return false;
    }
    return true;
}

Rtc::Time
Rtc::GetTime()
{
    AtomicSection as;
    u8 hour = regs.hour_lo;
    if (regs.hour_10) {
        hour += 10;
    }
    if (regs._12_24) {
        if (regs.hour_20_ampm) {
            hour += 12;
        }
    } else if (regs.hour_20_ampm) {
        hour += 20;
    }
    return Time { hour, static_cast<u8>(regs.min_hi * 10 + regs.min_lo),
                  static_cast<u8>(regs.sec_hi * 10 + regs.sec_lo) };
}

void
Rtc::SetTime(Time time)
{
    AtomicSection as;
    u8 hour = time.hour;
    if (hour >= 20) {
        regs.hour_20_ampm = 1;
        regs.hour_10 = 0;
        hour -= 20;
    } else {
        regs.hour_20_ampm = 0;
        if (hour >= 10) {
            regs.hour_10 = 1;
            hour -= 10;
        } else {
            regs.hour_10 = 0;
        }
    }
    regs.hour_lo = hour;
    regs._12_24 = 0;

    u8 min = time.min;
    regs.min_hi = min / 10;
    min -= regs.min_hi * 10;
    regs.min_lo = min;

    u8 sec = time.sec;
    regs.sec_hi = sec / 10;
    sec -= regs.sec_hi * 10;
    regs.sec_lo = sec;

    writeMask |= (1 << 0x00) | (1 << 0x01) | (1 << 0x02);
}

u8
Rtc::GetDayOfWeek()
{
    return regs.dow;
}

void
Rtc::Update()
{
    AtomicSection as;
    readPending = true;
}

bool
Rtc::NextWriteAddr()
{
    bool incremented = false;
    while (true) {
        for (;curAddr < 0x10; curAddr++) {
            if (writeMask & (1 << curAddr)) {
                return incremented;
            }
            incremented = true;
        }
        curAddr = 0;
    }
}
