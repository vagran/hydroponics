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
#ifdef I2C_USE_PULLUP
    AVR_BIT_SET8(AVR_REG_PORT(I2C_SCL_PORT), _BV(I2C_SCL_PIN));
    AVR_BIT_SET8(AVR_REG_PORT(I2C_SDA_PORT), _BV(I2C_SDA_PIN));
#endif
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
        break;

    case State::SLA_W:
        if (hwStatus != HwStatus::START_SENT &&
            hwStatus != HwStatus::REPEATED_START_SENT) {

            status = TransferStatus::TRANSMIT_FAILED;
            break;
        }
        state = State::SLA_W_SENT;
        SendByte(req.sla);
        break;

    case State::SLA_R_SENT:
        if (hwStatus != HwStatus::SLA_R_ACK) {
            status = TransferStatus::RECEIVE_FAILED;
            break;
        }
        status = TransferStatus::RECEIVE_READY;
        state = State::READ;
        break;

    case State::CLOSING_READ:
        status = TransferStatus::READ_CLOSED;
        break;

    case State::READ:
        if (hwStatus != HwStatus::DATA_RCVD_ACK &&
            hwStatus != HwStatus::DATA_RCVD_NACK) {

            status = TransferStatus::RECEIVE_FAILED;
            break;
        }
        rcvd = TWDR;
        if (hwStatus == HwStatus::DATA_RCVD_NACK) {
            status = TransferStatus::LAST_BYTE_RECEIVED;
        } else {
            status = TransferStatus::BYTE_RECEIVED;
        }
        break;

    case State::SLA_W_SENT:
        if (hwStatus != HwStatus::SLA_W_ACK) {
            status = TransferStatus::TRANSMIT_FAILED;
            break;
        }
        status = TransferStatus::TRANSMIT_READY;
        state = State::WRITE;
        break;

    case State::WRITE:
        if (hwStatus != HwStatus::DATA_SENT_ACK &&
            hwStatus != HwStatus::DATA_SENT_NACK) {

            status = TransferStatus::TRANSMIT_FAILED;
            break;
        }
        if (hwStatus == HwStatus::DATA_SENT_NACK) {
            status = TransferStatus::NACK;
        } else {
            status = TransferStatus::BYTE_TRANSMITTED;
        }
        break;

    }

    if (status != TransferStatus::NONE) {
        bool ret = false;
        if (status != TransferStatus::READ_CLOSED) {
            ret = req.handler(status, rcvd);
        }
        if (instantTransferPending &&
            (state != State::READ || hwStatus == HwStatus::DATA_RCVD_NACK)) {

            /* Send repeated start. */
            if (reqQueue[queuePtr].sla & 1) {
                state = State::SLA_R;
            } else {
                state = State::SLA_W;
            }
            SendStart();
            instantTransferPending = false;
        } else if (!ret || IsClosingStatus(status)) {
            if (!ret && !IsClosingStatus(status) &&
                (state == State::READ || state == State::SLA_R_SENT)) {

                /* Closed by handler without NACK sent, receive one more byte
                 * and send NACK.
                 */
                state = State::CLOSING_READ;
                Nack();
                ReceiveByte();
            } else {
                CloseTransfer();
            }
        } else {
            if (state == State::READ) {
                ReceiveByte();
            } else if (state == State::WRITE) {
                if (transmitPending) {
                    transmitPending = false;
                    SendByte(pendingTransmitByte);
                } else {
                    /* No transmission data specified by the handler, close the
                     * transfer.
                     */
                    CloseTransfer();
                }
            }
        }
    }
    scheduler.SchedulePoll();
}

void
I2cBus::CloseTransfer()
{
    TransferReq &req = reqQueue[queuePtr];
    req.handler = 0;
    if (queuePtr == I2C_REQ_QUEUE_SIZE - 1) {
        queuePtr = 0;
    } else {
        queuePtr++;
    }
    if (state != State::SLA_R && state != State::SLA_W) {
        SendStop();
    }
    state = State::IDLE;
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
