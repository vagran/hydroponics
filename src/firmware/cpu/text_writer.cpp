/* This file is a part of 'hydroponics' project.
 * Copyright (c) 2012-2015, Artyom Lebedev <artyom.lebedev@gmail.com>
 * All rights reserved.
 * See COPYING file for copyright details.
 */

/** @file text_writer.cpp
 * TODO insert description here.
 */

#include "cpu.h"

#include "font.h"

using namespace adk;

TextWriter textWriter;

void
TextWriter::Poll()
{
    AtomicSection as;
    if (!reqInProgress) {
        StartRequest();
    }
}

void
TextWriter::Write(Display::Viewport vp, const char *text, bool inversed,
                  bool clear, bool fillVp, DoneHandler handler, bool isPgm)
{
    AtomicSection as;
    /* Find queue free slot. */
    u8 idx = curReq;
    while (true) {
        if (!reqQueue[idx].text) {
            /* Free slot found. */
            break;
        }
        idx++;
        if (idx >= SIZEOF_ARRAY(reqQueue)) {
            idx = 0;
        }
        if (idx == curReq) {
            /* No free slot. */
            return;
        }
    }
    Request &req = reqQueue[idx];
    req.vp = vp;
    req.text = text;
    req.inversed = inversed;
    req.clear = clear;
    req.fillVp = fillVp;
    req.handler = handler;
    req.isPgm = isPgm;
    if (idx == curReq) {
        /* Start output. */
        StartRequest();
    }
}

bool
TextWriter::_OutputHandler(u8 column, u8 page, u8 *data)
{
    return textWriter.OutputHandler(column, page, data);
}

bool
TextWriter::OutputHandler(u8 column, u8 page, u8 *data)
{
    Request &req = reqQueue[curReq];
    u8 _data;
    while (true) {
        if (fillingTail) {
            _data = 0;
            break;
        }
        if (curCharCol == 0 && (req.vp.maxCol - column + 1) < FONT_WIDTH) {
            /* Write next character on a new line if no more space in the
             * current line.
             */
            _data = 0;
            break;
        }
        if (curCharCol < FONT_WIDTH) {
            if (curChar >= 0x10 && !req.clear) {
                _data = pgm_read_byte(&fontData[curChar - 0x10][curCharCol]);
            } else {
                _data = 0;
            }
            curCharCol++;
            break;
        }
        if (curCharCol == FONT_WIDTH) {
            if (req.isPgm) {
                curChar = pgm_read_byte(req.text);
            } else {
                curChar = *req.text;
            }
            req.text++;
            if (!curChar) {
                if (req.fillVp) {
                    fillingTail = true;
                    _data = 0;
                    break;
                }
                NextRequest();
                return false;
            }
        }
        if (curCharCol < FONT_WIDTH + CHAR_SPACE_WIDTH &&
            column != req.vp.minCol) { /* Page break resets space. */

            _data = 0;
            curCharCol++;
            break;
        }
        curCharCol = 0;
    }
    if (req.inversed) {
        *data = ~_data;
    } else {
        *data = _data;
    }
    if (page == req.vp.maxPage && column == req.vp.maxCol) {
        NextRequest();
    }
    return true;
}

bool
TextWriter::StartRequest()
{
    while (true) {
        Request &req = reqQueue[curReq];
        if (!req.text) {
            return false;
        }
        if (req.isPgm) {
            curChar = pgm_read_byte(req.text);
        } else {
            curChar = *req.text;
        }
        req.text++;
        if (!curChar) {
            NextRequest();
            continue;
        }
        curCharCol = 0;
        fillingTail = false;
        reqInProgress = true;
        display.Output(req.vp, _OutputHandler);
        return true;
    }
}

void
TextWriter::NextRequest()
{
    if (reqQueue[curReq].handler) {
        reqQueue[curReq].handler();
    }
    reqQueue[curReq].text = 0;
    if (curReq == SIZEOF_ARRAY(reqQueue)) {
        curReq = 0;
    } else {
        curReq++;
    }
    curCharCol = 0;
    reqInProgress = false;
}
