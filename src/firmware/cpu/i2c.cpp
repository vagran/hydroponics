/* This file is a part of 'hydroponics' project.
 * Copyright (c) 2012-2015, Artyom Lebedev <artyom.lebedev@gmail.com>
 * All rights reserved.
 * See COPYING file for copyright details.
 */

/** @file i2c.c */

#include <adk.h>
#include "i2c.h"

typedef enum {
    S_IDLE
} I2cState;

/** Transfer request. */
typedef struct __PACKED {
    /** Address packet byte. */
    u8 sla;
    /** Null for free slot. */
    I2cTransferHandler handler;
} I2cTranfserReq;

/** Pending requests queue. */
static I2cTranfserReq reqQueue[I2C_REQ_QUEUE_SIZE];

#define REQ_QUEUE_PTR_BITS 4
#if (1 << REQ_QUEUE_PTR_BITS) < I2C_REQ_QUEUE_SIZE
#error I2C_REQ_QUEUE_SIZE is too big. Number of bits in queue pointer should \
    be increased.
#endif

/** State variables. */
static struct {
    /** Index of next element in the requests queue. */
    u8 queuePtr:REQ_QUEUE_PTR_BITS,
       instantTransferPending:1,
       state:3;
} g_i2c;

I2cBus::I2cBus()
{
    /* 400kHz clock. */
    TWBR = 17;
    TWCR = _BV(TWIE) | _BV(TWEN);
}

void
I2cBus::Poll()
{
    u8 sreg = SREG;
    cli();
    if (g_i2c.state == S_IDLE) {
        //XXX
    }
    SREG = sreg;
}

ISR(TWI_vect)
{

}

u8
I2cRequestTransfer(u8 address, u8 isTransmit, I2cTransferHandler handler)
{
    u8 sreg = SREG;
    cli();
    /* Find queue free slot. */
    u8 idx = g_i2c.queuePtr;
    while (TRUE) {
        if (!reqQueue[idx].handler) {
            /* Free slot found. */
            break;
        }
        idx++;
        if (idx >= I2C_REQ_QUEUE_SIZE) {
            idx = 0;
        }
        if (idx == g_i2c.queuePtr) {
            /* No free slot. */
            SREG = sreg;
            return FALSE;
        }
    }
    reqQueue[idx].handler = handler;
    reqQueue[idx].sla = (address << 1) | (isTransmit ? 0 : 1);
    SREG = sreg;
    return TRUE;
}

void
I2cRequestInstantTransfer(u8 address, u8 isTransmit, I2cTransferHandler handler)
{
    u8 sreg = SREG;
    cli();
    u8 idx = g_i2c.queuePtr;
    reqQueue[idx].handler = handler;
    reqQueue[idx].sla = (address << 1) | (isTransmit ? 0 : 1);
    g_i2c.instantTransferPending = TRUE;
    SREG = sreg;
}
