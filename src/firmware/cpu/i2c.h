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

typedef enum {
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
} I2cStatus;

/** Check if a transfer is closed after the specified status code received. */
static inline u8
I2cIsClosingStatus(I2cStatus status)
{
    return status & CLOSE_MASK;
}

/** Handles I2C transfer steps.
 *
 * @param status Current status of the transfer. The transfer is closed after
 *      the handler invocation if I2cIsClosingStatus(status) is true.
 * @param data Received data byte. Valid with @ref BYTE_RECEIVED and @ref
 *      LAST_BYTE_RECEIVED status codes.
 * @return TRUE to continue the transfer if permitted, FALSE to end the transfer.
 */
typedef u8 (*I2cTransferHandler)(I2cStatus status, u8 data);

/** Enqueue new transfer.
 *
 * @param address Target device address. Seven least significant bits are used.
 * @param isTransmit Transmission if TRUE, receiving if FALSE.
 * @param handler Handles the transfer steps.
 * @return TRUE if the transfer enqueued, FALSE if failed.
 */
u8
I2cRequestTransfer(u8 address, u8 isTransmit, I2cTransferHandler handler);

/** Send NACK on next received byte. Should be called in transfer handler only. */
void
I2cNack();

/** Specify next byte for transmission.  Should be called in transfer handler
 * only.
 * @param data Byte to transmit.
 */
void
I2cTransmitByte(u8 data);

/** Initialize I2C driver. */
void
I2cInit();

/** Call from main loop. */
void
I2cPoll();

#endif /* I2C_H_ */
