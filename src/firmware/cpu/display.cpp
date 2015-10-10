/* This file is a part of 'hydroponics' project.
 * Copyright (c) 2015, Artyom Lebedev <artyom.lebedev@gmail.com>
 * All rights reserved.
 * See LICENSE file for copyright details.
 */

/** @file display.cpp */

#include "cpu.h"

using namespace adk;

Display display;

void
Display::Initialize()
{
    isSleeping = false;
    curVp.maxCol = 127;
    curVp.maxPage = 7;
    curColumn = curVp.minCol;
    curPage = curVp.minPage;
    state = State::INITIALIZING;
    HandleInitialization();
}

void
Display::SetSleep(bool f)
{
    AtomicSection as;
    if (f && !isSleeping) {
        isSleeping = true;
        SendCommand(Command::DISPLAY_OFF);
    } else if (!f && isSleeping) {
        isSleeping = false;
        SendCommand(Command::DISPLAY_ON);
    }
}

void
Display::Output(Viewport vp, GraphicsProvider provider)
{
    AtomicSection as;
    /* Find queue free slot. */
    u8 idx = curOutReq;
    while (true) {
        if (!outQueue[idx].provider) {
            /* Free slot found. */
            break;
        }
        idx++;
        if (idx >= MAX_OUT_REQS) {
            idx = 0;
        }
        if (idx == curOutReq) {
            /* No free slot. */
            return;
        }
    }
    outQueue[idx].vp = vp;
    outQueue[idx].provider = provider;
    scheduler.SchedulePoll();
}

void
Display::Poll()
{
    AtomicSection as;
    if (state == State::INITIALIZING) {
        HandleInitialization();
    } else if (state == State::READY) {
        if (!outInProgress && outQueue[curOutReq].provider) {
            outInProgress = true;
            i2cBus.RequestTransfer(DISPLAY_ADDRESS, true, OutputTransferHandler);
        }
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
        /* Set maximal frequency. */
        SendCommand(Command::SET_CLOCK_DIV, 0xf0);
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
    case 16:
        SendCommand(Command::SET_COLUMN_ADDRESS, curVp.minCol, curVp.maxCol);
        break;
    case 17:
        SendCommand(Command::SET_PAGE_ADDRESS, curVp.minPage, curVp.maxPage);
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
Display::OutputTransferHandler(I2cBus::TransferStatus status, u8)
{
    return display.HandleOutputTransfer(status);
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

bool
Display::HandleOutputTransfer(I2cBus::TransferStatus status)
{
    if (status != I2cBus::TransferStatus::TRANSMIT_READY &&
        status != I2cBus::TransferStatus::BYTE_TRANSMITTED) {

        /* Output failure. */
        FinishOutputRequest();
        return false;
    }

    if (outReqComplete) {
        /* Viewport fully covered. */
        FinishOutputRequest();
        return false;
    }

    if (outVpCtrlSent) {
        i2cBus.TransmitByte(outVpCmd);
        outVpCtrlSent = false;
        return true;
    }

    OutputReq &req = outQueue[curOutReq];

    /* Set viewport of necessary. */
    while (outVpState < OutVpState::DONE) {
        u16 cmd = 0xffff;
        switch (outVpState) {
        case OutVpState::NONE:
            if (curVp.minCol == req.vp.minCol &&
                curVp.maxCol == req.vp.maxCol &&
                curColumn == req.vp.minCol) {

                outVpState = OutVpState::PAGE_CMD;
                break;
            }
            outVpState = OutVpState::COL_CMD;
            /* FALL THROUGH */
        case OutVpState::COL_CMD:
            cmd = Command::SET_COLUMN_ADDRESS;
            outVpState = OutVpState::COL_MIN;
            break;
        case OutVpState::COL_MIN:
            cmd = req.vp.minCol;
            curVp.minCol = cmd;
            curColumn = cmd;
            outVpState = OutVpState::COL_MAX;
            break;
        case OutVpState::COL_MAX:
            cmd = req.vp.maxCol;
            curVp.maxCol = cmd;
            outVpState = OutVpState::PAGE_CMD;
            break;
        case OutVpState::PAGE_CMD:
            if (curVp.minPage == req.vp.minPage &&
                curVp.maxPage == req.vp.maxPage &&
                curPage == req.vp.minPage) {

                outVpState = OutVpState::DONE;
                break;
            }
            outVpState = OutVpState::PAGE_MIN;
            cmd = Command::SET_PAGE_ADDRESS;
            break;
        case OutVpState::PAGE_MIN:
            cmd = req.vp.minPage;
            curVp.minPage = cmd;
            curPage = cmd;
            outVpState = OutVpState::PAGE_MAX;
            break;
        case OutVpState::PAGE_MAX:
            cmd = req.vp.maxPage;
            curVp.maxPage = cmd;
            outVpState = OutVpState::DONE;
            break;
        }

        if (cmd != 0xffff) {
            i2cBus.TransmitByte(0x80);
            outVpCtrlSent = true;
            outVpCmd = cmd;
            return true;
        }
    }

    if (!outDataCtrlSent) {
        outDataCtrlSent = true;
        i2cBus.TransmitByte(0x40);
        return true;
    }

    u8 data;
    if (!req.provider(curColumn, curPage, &data)) {
        /* Request finished. */
        FinishOutputRequest();
        return false;
    }
    i2cBus.TransmitByte(data);
    if (curColumn == curVp.maxCol) {
        curColumn = curVp.minCol;
        if (curPage == curVp.maxPage) {
            /* Viewport fully covered. */
            curPage = curVp.minPage;
            outReqComplete = true;
        } else {
            curPage++;
        }
    } else {
        curColumn++;
    }
    return true;
}

bool
Display::FinishOutputRequest()
{
    outQueue[curOutReq].provider = 0;
    if (curOutReq == SIZEOF_ARRAY(outQueue)) {
        curOutReq = 0;
    } else {
        curOutReq++;
    }
    outVpState = OutVpState::NONE;
    outVpCtrlSent = false;
    outInProgress = false;
    outDataCtrlSent = false;
    outReqComplete = false;
    return outQueue[curOutReq].provider;
}

static bool
ClearOutputHandler(u8, u8, u8 *data)
{
    *data = 0;
    return true;
}

void
Display::Clear(Viewport vp)
{
    Output(vp, ClearOutputHandler);
}
