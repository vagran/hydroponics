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

/** I2C lines port and pins for internal pull-up resistors if used. Define
 * I2C_USE_PULLUP for enabling internal pull-up resistors.
 */
#ifndef I2C_SCL_PORT
#define I2C_SCL_PORT    C
#endif

#ifndef I2C_SCL_PIN
#define I2C_SCL_PIN     5
#endif

#ifndef I2C_SDA_PORT
#define I2C_SDA_PORT    C
#endif

#ifndef I2C_SDA_PIN
#define I2C_SDA_PIN     4
#endif

/** Maximal number of queued I2C transfer request. */
#ifndef I2C_REQ_QUEUE_SIZE
#define I2C_REQ_QUEUE_SIZE 8
#endif

#define I2C_REQ_QUEUE_PTR_BITS 4
#if (1 << I2C_REQ_QUEUE_PTR_BITS) < I2C_REQ_QUEUE_SIZE
#error I2C_REQ_QUEUE_SIZE is too big. Number of bits in queue pointer should \
    be increased.
#endif

class I2cBus {
public:
    enum TransferStatus {
        /** Do not call the transfer handler. Never seen by the handler. */
        NONE,
        /** SLA+W successfully transferred. Ready to transmit bytes. The handler
         * should call TransmitByte() function to schedule next byte.
         * transmission.
         */
        TRANSMIT_READY,
        /** SLA+R successfully transferred. Ready to receive bytes. Nack()
         * method can be called to send NACK on the first received byte.
         */
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
        /** Either arbitration lost, NACK on SLA+R or other failure. */
        RECEIVE_FAILED,
        /** NACK received when transmitting byte. */
        NACK,
        /** Byte was successfully received and NACK has been returned. */
        LAST_BYTE_RECEIVED,

        /** Special status for final read after transfer was closed by handler
         * but NACK was not sent. Never seen by handler.
         */
        READ_CLOSED
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
     * handler only. The current transfer is closed. For read transfer the
     * scheduled instant transfer will be deferred until NACK is sent.
     */
    void
    RequestInstantTransfer(u8 address, bool isTransmit, TransferHandler handler);

    /** Send NACK on next received byte. Should be called in transfer handler
     * only. Have effect in read transfer only.
     */
    void
    Nack()
    {
        if (state == State::READ) {
            nackPending = true;
        }
    }

    /** Specify next byte for transmission.  Should be called in transfer handler
     * only. Have effect in write transfer only. The transfer is closed if not
     * called in write transfer handler.
     *
     * @param data Byte to transmit.
     */
    void
    TransmitByte(u8 data)
    {
        if (state == State::WRITE) {
            pendingTransmitByte = data;
            transmitPending = true;
        }
    }

    /** Should be called by interrupt only. */
    void
    HandleInterrupt();
private:
    enum State {
        IDLE,
        SLA_W,
        SLA_R,
        SLA_W_SENT,
        SLA_R_SENT,
        READ,
        WRITE,
        /** Transfer closed by handler, NACK should be sent on next byte. */
        CLOSING_READ
    };

    /** Hardware unit status codes. */
    enum HwStatus {
        START_SENT =            0x08,
        REPEATED_START_SENT =   0x10,
        SLA_W_ACK =             0x18,
        SLA_W_NACK =            0x20,
        DATA_SENT_ACK =         0x28,
        DATA_SENT_NACK =        0x30,
        ARBITRATION_LOST =      0x38,
        SLA_R_ACK =             0x40,
        SLA_R_NACK =            0x48,
        DATA_RCVD_ACK =         0x50,
        DATA_RCVD_NACK =        0x58,

        NO_STATE =              0xf8
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
    /** Byte to transmit. */
    u8 pendingTransmitByte;
    /** Index of next element in the requests queue. */
    u8 queuePtr:I2C_REQ_QUEUE_PTR_BITS,
    /** Transfer at current queue position should be started by repeated start. */
       instantTransferPending:1,
       state:3,

   /** Send NACK on next received byte. */
       nackPending:1,
   /** Next transmission byte specified. */
       transmitPending:1;

    /** Check if hardware is currently idle. */
    inline bool
    IsHwIdle()
    {
        return (TWCR & _BV(TWINT)) || (TWSR == HwStatus::NO_STATE);
    }

    /** Send START condition. */
    inline void
    SendStart()
    {
        TWCR = (TWCR & ~(_BV(TWINT) | _BV(TWSTA) | _BV(TWSTO))) |
               (_BV(TWINT) | _BV(TWSTA));
    }

    /** Send STOP condition. */
    inline void
    SendStop()
    {
        TWCR = (TWCR & ~(_BV(TWINT) | _BV(TWSTA) | _BV(TWSTO))) |
               (_BV(TWINT) | _BV(TWSTO));
    }

    /** Send data byte. */
    inline void
    SendByte(u8 byte)
    {
        TWDR = byte;
        TWCR = (TWCR & ~(_BV(TWINT) | _BV(TWSTA) | _BV(TWSTO))) |
               _BV(TWINT);
    }

    inline void
    ReceiveByte()
    {
        TWCR = (TWCR & ~(_BV(TWINT) | _BV(TWSTA) | _BV(TWSTO) | _BV(TWEA))) |
                (_BV(TWINT) | (nackPending ? 0 : _BV(TWEA)));
        nackPending = false;
    }

    /** Close current trasfer. Should be called with interrupts disabled. */
    void
    CloseTransfer();

} __PACKED;

extern I2cBus i2cBus;

#endif /* I2C_H_ */
