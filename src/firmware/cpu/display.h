/* This file is a part of 'hydroponics' project.
 * Copyright (c) 2012-2015, Artyom Lebedev <artyom.lebedev@gmail.com>
 * All rights reserved.
 * See COPYING file for copyright details.
 */

/** @file display.h
 * Graphical display support.
 */

#ifndef DISPLAY_H_
#define DISPLAY_H_

#ifndef DISPLAY_ADDRESS
#define DISPLAY_ADDRESS 0x3c
#endif

/** Graphical display based on SSD1306 controller. Communication via I2C bus. */
class Display {
public:

    void
    Initialize();

    void
    Poll();

private:
    enum {
        /** Maximal number of bytes in a command. Long commands (scrolling setup)
         * currently are not supported.
         */
        MAX_CMD_SIZE = 3,
        /** Second byte for CHARGE_PUMP command. */
        CHARGE_PUMP_ENABLE =    0x14,
        CHARGE_PUMP_DISABLE =   0x10,
        /** Second byte for SET_MEMORY_MODE command. */
        MEM_MODE_HORIZONTAL =   0x0,
        MEM_MODE_VERTICAL =     0x1,
        MEM_MODE_PAGE =         0x2,
        /** Second byte for SET_COM_PINS command. COM_PINS can be or'ed with the
         * option values.
         */
        COM_PINS =              0x2,
        COM_PINS_ALTERNATIVE =  0x10,
        COM_PINS_REMAP =        0x20,
    };

    enum Command {
        DISPLAY_OFF =           0xae,
        DISPLAY_ON =            0xaf,
        SET_CLOCK_DIV =         0xd5,
        SET_MULTIPLEX =         0xa8,
        SET_DISPLAY_OFFSET =    0xd3,
        SET_START_LINE =        0x40,
        CHARGE_PUMP =           0x8d,
        SET_MEMORY_MODE =       0x20,
        SET_SEG_REMAP_0 =       0xa0,
        SET_SEG_REMAP_127 =     0xa1,
        SET_COM_SCAN_DIR_FWD =  0xc0,
        SET_COM_SCAN_DIR_REV =  0xc8,
        SET_COM_PINS =          0xda,
        SET_CONTRAST =          0x81,
        SET_PRECHARGE_PERIOD =  0xd9,
        SET_VCOMH_DESELECT_LVL =0xdb,
        NORMAL_DISPLAY =        0xa6,
        INVERSE_DISPLAY =       0xa7,
        ENTIRE_DISPLAY_ON =     0xa5,
        ENTIRE_DISPLAY_RAM =    0xa4
    };

    enum State {
        /** After power on. */
        INITIAL,
        /** Initialization in progress. */
        INITIALIZING,
        /** Ready for work. */
        READY,
        /** Hardware failure, not operational. */
        FAILURE,
    };

    /** Command bytes. Stored in reversed order. */
    u8 cmdBuf[MAX_CMD_SIZE];
    /** Current state. */
    u8 state:4,
    /** Counter for initialization sequence. */
       initCounter:5,
    /** Number of bytes in command buffer. */
       cmdSize:2,
    /** Control byte was sent before next command byte. */
       controlSent:1,
    /** Command is being transmitted. */
       cmdInProgress:1;

    static bool
    CommandTransferHandler(I2cBus::TransferStatus status, u8 data);

    /** Queue command sending.
     * @param bytes Up to MAX_CMD_SIZE bytes of command data.
     */
    template <typename... TByte>
    void
    SendCommand(TByte... bytes)
    {
        cmdSize = sizeof...(bytes);
        controlSent = false;
        cmdInProgress = true;
        SetCmdByte(sizeof...(bytes) - 1, bytes...);
        i2cBus.RequestTransfer(DISPLAY_ADDRESS, true,
                               CommandTransferHandler);
    }

    template <typename... TByte>
    inline void
    SetCmdByte(int idx, u8 byte, TByte... bytes)
    {
        cmdBuf[idx] = byte;
        SetCmdByte(idx - 1, bytes...);
    }

    inline void
    SetCmdByte(int, u8 byte)
    {
        cmdBuf[0] = byte;
    }

    void
    HandleInitialization();

    bool
    HandleCommandTransfer(I2cBus::TransferStatus status);

} __PACKED;

extern Display display;


#endif /* DISPLAY_H_ */
