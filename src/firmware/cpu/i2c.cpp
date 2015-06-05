/* This file is a part of 'hydroponics' project.
 * Copyright (c) 2012-2015, Artyom Lebedev <artyom.lebedev@gmail.com>
 * All rights reserved.
 * See COPYING file for copyright details.
 */

/** @file i2c.c */

#include "cpu.h"

using namespace adk;

I2cBus i2cBus;

I2cBus::I2cBus()
{
    /* 400kHz clock. */
    TWBR = 17;
    TWCR = _BV(TWIE) | _BV(TWEN);
}

void
I2cBus::Poll()
{
    AtomicSection as;
    if (state == State::IDLE && IsHwIdle()) {
        TransferReq &req = reqQueue[queuePtr];
        if (!req.handler) {
            return;
        }
        if (req.sla & 1) {
            state = State::SLA_R;
        } else {
            state = State::SLA_W;
        }
        SendStart();
    }
}

void
I2cBus::HandleInterrupt()
{
    u8 hwStatus = TWSR;
    TransferReq &req = reqQueue[queuePtr];
    u8 rcvd = 0;
    TransferStatus status = TransferStatus::NONE;

    if (!req.handler) {
        return;
    }

    switch (state) {
    case State::SLA_R:
        if (hwStatus != HwStatus::START_SENT &&
            hwStatus != HwStatus::REPEATED_START_SENT) {

            status = TransferStatus::RECEIVE_FAILED;
            break;
        }
        state = State::SLA_R_SENT;
        SendByte(req.sla);
        //XXX
        break;
    case State::SLA_W:
        if (hwStatus != HwStatus::START_SENT &&
            hwStatus != HwStatus::REPEATED_START_SENT) {

            status = TransferStatus::TRANSMIT_FAILED;
            break;
        }
        state = State::SLA_W_SENT;
        SendByte(req.sla);
        //XXX
        break;
    //XXX
    }

    if (status != TransferStatus::NONE) {
        bool ret __UNUSED = req.handler(status, rcvd);
        if (IsClosingStatus(status)) {
            req.handler = 0;
            if (queuePtr == I2C_REQ_QUEUE_SIZE - 1) {
                queuePtr = 0;
            } else {
                queuePtr++;
            }
        } else {
            //XXX check pending transmission or repeated start
        }
    }
    scheduler.SchedulePoll();
}

ISR(TWI_vect)
{
    i2cBus.HandleInterrupt();
}

bool
I2cBus::RequestTransfer(u8 address, bool isTransmit, TransferHandler handler)
{
    AtomicSection as;
    /* Find queue free slot. */
    u8 idx = queuePtr;
    while (true) {
        if (!reqQueue[idx].handler) {
            /* Free slot found. */
            break;
        }
        idx++;
        if (idx >= I2C_REQ_QUEUE_SIZE) {
            idx = 0;
        }
        if (idx == queuePtr) {
            /* No free slot. */
            return false;
        }
    }
    reqQueue[idx].handler = handler;
    reqQueue[idx].sla = (address << 1) | (isTransmit ? 0 : 1);
    return true;
}

void
I2cBus::RequestInstantTransfer(u8 address, bool isTransmit, TransferHandler handler)
{
    AtomicSection as;
    u8 idx = queuePtr;
    reqQueue[idx].handler = handler;
    reqQueue[idx].sla = (address << 1) | (isTransmit ? 0 : 1);
    instantTransferPending = true;
}
