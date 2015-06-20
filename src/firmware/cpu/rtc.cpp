/* This file is a part of 'hydroponics' project.
 * Copyright (c) 2012-2015, Artyom Lebedev <artyom.lebedev@gmail.com>
 * All rights reserved.
 * See COPYING file for copyright details.
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
    readPending = true;
    Poll();
    while (true) {
        {
            AtomicSection as;
            if (!readPending) {
                break;
            }
        }
        _delay_ms(100);
    }
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
    //XXX
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
                *(reinterpret_cast<u8 *>(&regs) + curAddr) = data;
                curAddr++;
                if (curAddr == 0x12) {
                    i2cBus.Nack();
                }
            } else if (status == I2cBus::TransferStatus::LAST_BYTE_RECEIVED) {
                *(reinterpret_cast<u8 *>(&regs) + curAddr) = data;
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
        //XXX
    } else {
        return false;
    }
    return true;
}

Rtc::Time
Rtc::GetTime()
{
    AtomicSection as;
    u8 hour = regs.hour_low;
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
    return Time { hour, static_cast<u8>(regs.min_hi * 10 + regs.min_low),
                  static_cast<u8>(regs.sec_hi * 10 + regs.sec_low) };
}

void
Rtc::Update()
{
    AtomicSection as;
    readPending = true;
}
