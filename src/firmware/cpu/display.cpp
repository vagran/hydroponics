/* This file is a part of 'hydroponics' project.
 * Copyright (c) 2012-2015, Artyom Lebedev <artyom.lebedev@gmail.com>
 * All rights reserved.
 * See COPYING file for copyright details.
 */

/** @file display.cpp */

#include "cpu.h"

using namespace adk;

Display display;

void
Display::Initialize()
{
    state = State::INITIALIZING;
    HandleInitialization();
}

void
Display::Poll()
{
    AtomicSection as;
    if (state == State::INITIALIZING) {
        HandleInitialization();
    }
}

void
Display::HandleInitialization()
{
    if (cmdInProgress) {
        return;
    }
    switch (initCounter) {
    case 0:
        SendCommand(Command::DISPLAY_OFF);
        break;
    case 1:
        SendCommand(Command::SET_CLOCK_DIV, 0x80);
        break;
    case 2:
        SendCommand(Command::SET_MULTIPLEX, 0x3f);
        break;
    case 3:
        SendCommand(Command::SET_DISPLAY_OFFSET, 0);
        break;
    case 4:
        SendCommand(Command::SET_START_LINE);
        break;
    case 5:
        SendCommand(Command::CHARGE_PUMP, CHARGE_PUMP_ENABLE);
        break;
    case 6:
        SendCommand(Command::SET_MEMORY_MODE, MEM_MODE_HORIZONTAL);
        break;
    case 7:
        SendCommand(Command::SET_SEG_REMAP_127);
        break;
    case 8:
        SendCommand(Command::SET_COM_SCAN_DIR_REV);
        break;
    case 9:
        SendCommand(Command::SET_COM_PINS, COM_PINS | COM_PINS_ALTERNATIVE);
        break;
    case 10:
        SendCommand(Command::SET_CONTRAST, 0xcf);
        break;
    case 11:
        SendCommand(Command::SET_PRECHARGE_PERIOD, 0xf1);
        break;
    case 12:
        SendCommand(Command::SET_VCOMH_DESELECT_LVL, 0x40);
        break;
    case 13:
        SendCommand(Command::ENTIRE_DISPLAY_RAM);
        break;
    case 14:
        SendCommand(Command::NORMAL_DISPLAY);
        break;
    case 15:
        SendCommand(Command::DISPLAY_ON);
        break;
    default:
        state = State::READY;
    }
    initCounter++;
}

bool
Display::CommandTransferHandler(I2cBus::TransferStatus status, u8)
{
    return display.HandleCommandTransfer(status);
}

bool
Display::HandleCommandTransfer(I2cBus::TransferStatus status)
{
    if (status == I2cBus::TransferStatus::TRANSMIT_READY ||
        status == I2cBus::TransferStatus::BYTE_TRANSMITTED) {

        if (cmdSize) {
            if (!controlSent) {
                controlSent = true;
                i2cBus.TransmitByte(0x80);
            } else {
                cmdSize--;
                i2cBus.TransmitByte(cmdBuf[cmdSize]);
                controlSent = false;
            }
            return true;
        } else {
            /* Last byte transmitted. */
            cmdInProgress = false;
        }
        return false;
    }
    if (state == State::INITIALIZING) {
        state = State::FAILURE;
    }
    cmdSize = 0;
    cmdInProgress = false;
    return false;
}
