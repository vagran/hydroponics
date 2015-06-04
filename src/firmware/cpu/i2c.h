/* This file is a part of 'hydroponics' project.
 * Copyright (c) 2012-2015, Artyom Lebedev <artyom.lebedev@gmail.com>
 * All rights reserved.
 * See COPYING file for copyright details.
 */

/** @file i2c.h
 * I2C bus driver.
 * XXX for now only master mode.
 */

#ifndef I2C_H_
#define I2C_H_

/** Maximal number of queued I2C transfer request. */
#ifndef I2C_REQ_QUEUE_SIZE
#define I2C_REQ_QUEUE_SIZE 4
#endif

#define I2C_REQ_QUEUE_PTR_BITS 4
#if (1 << I2C_REQ_QUEUE_PTR_BITS) < I2C_REQ_QUEUE_SIZE
#error I2C_REQ_QUEUE_SIZE is too big. Number of bits in queue pointer should \
    be increased.
#endif

class I2cBus {
public:
    enum TransferStatus {
        /** SLA+W successfully transferred. Ready to transmit bytes. */
        TRANSMIT_READY,
        /** SLA+R successfully transferred. Ready to receive bytes. */
        RECEIVE_READY,
        /** Byte was successfully transmitted, ready to transmit next byte. */
        BYTE_TRANSMITTED,
        /** Byte was successfully received, ready to receive next byte. */
        BYTE_RECEIVED,

        /* A transfer is closed on the codes below. */
        /** The bit which indicates that a transfer is closed after such status. */
        CLOSE_MASK = 0x80,
        /** Either arbitration lost, NACK on SLA+W or other failure. */
        TRANSMIT_FAILED = 0x80,
        /** Either arbitration lost, NACK on SLA+W or other failure. */
        RECEIVE_FAILED,
        /** NACK received when transmitting byte. */
        NACK,
        /** Byte was successfully received and NACK has been returned. */
        LAST_BYTE_RECEIVED
    };

    /** Handles I2C transfer steps.
     *
     * @param status Current status of the transfer. The transfer is closed after
     *      the handler invocation if I2cIsClosingStatus(status) is true.
     * @param data Received data byte. Valid with @ref BYTE_RECEIVED and @ref
     *      LAST_BYTE_RECEIVED status codes.
     * @return True to continue the transfer if permitted, false to end the transfer.
     */
    typedef bool (*TransferHandler)(TransferStatus status, u8 data);

    /** Initialize I2C driver. */
    I2cBus();

    /** Call from main loop. */
    void
    Poll();

    /** Check if a transfer is closed after the specified status code received. */
    static inline bool
    IsClosingStatus(TransferStatus status)
    {
        return status & CLOSE_MASK;
    }

    /** Enqueue new transfer.
     *
     * @param address Target device address. Seven least significant bits are used.
     * @param isTransmit Transmission if true, receiving if false.
     * @param handler Handles the transfer steps.
     * @return True if the transfer enqueued, false if failed.
     */
    bool
    RequestTransfer(u8 address, bool isTransmit, TransferHandler handler);

    /** Enqueue new transfer right after the current one. The transfer is guaranteed
     * to be executed with repeated start condition. Should be called in transfer
     * handler only.
     */
    void
    RequestInstantTransfer(u8 address, bool isTransmit, TransferHandler handler);

    /** Send NACK on next received byte. Should be called in transfer handler only. */
    void
    Nack();

    /** Specify next byte for transmission.  Should be called in transfer handler
     * only.
     * @param data Byte to transmit.
     */
    void
    TransmitByte(u8 data);

private:
    enum State {
        IDLE
    };

    /** Transfer request. */
    struct TransferReq {
        /** Null for free slot. */
        TransferHandler handler;
        /** Address packet byte. */
        u8 sla;
    } __PACKED;

    /** Pending requests queue. */
    TransferReq reqQueue[I2C_REQ_QUEUE_SIZE];
    /** Index of next element in the requests queue. */
    u8 queuePtr:I2C_REQ_QUEUE_PTR_BITS,
       instantTransferPending:1,
       state:3;

} __PACKED;

extern I2cBus i2cBus;

#endif /* I2C_H_ */
