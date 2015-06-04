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
    if (state == State::IDLE) {
        //XXX
    }
}

ISR(TWI_vect)
{

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
