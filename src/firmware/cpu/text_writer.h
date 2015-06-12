/* This file is a part of 'hydroponics' project.
 * Copyright (c) 2012-2015, Artyom Lebedev <artyom.lebedev@gmail.com>
 * All rights reserved.
 * See COPYING file for copyright details.
 */

/** @file text_writer.h
 * TODO insert description here.
 */

#ifndef TEXT_WRITER_H_
#define TEXT_WRITER_H_

/** Helper class for text output to graphical display. */
class TextWriter {
public:
    /** Called when output finished. */
    typedef void (*DoneHandler)();

    void
    Poll();

    inline void
    Write(Display::Viewport vp, char *text, bool inversed = false,
          DoneHandler handler = nullptr)
    {
        Write(vp, text, inversed, handler, false);
    }

    inline void
    Write(Display::Viewport vp, const char *text, bool inversed = false,
          DoneHandler handler = nullptr)
    {
        Write(vp, text, inversed, handler, true);
    }

private:
    enum {
        MAX_REQUESTS = 8,
        FONT_WIDTH = 6,
        /** Width in pixels of space between characters in a word. */
        CHAR_SPACE_WIDTH = 1
    };

    struct Request {
        const char *text;
        DoneHandler handler;
        Display::Viewport vp;
        u8 isPgm:1,
           inversed:1,
           reserved:6;
    } __PACKED;

    Request reqQueue[MAX_REQUESTS];
    /** Current request index in the queue. */
    u8 curReq:3,
       curCharCol:3,
    /** Current column in the character. One additional column for space. */
       reserved:2,

    /** Next character to output. */
       curChar:7,
    /** Request processing in progress. */
       reqInProgress:1;


    void
    Write(Display::Viewport vp, const char *text, bool inversed,
          DoneHandler handler, bool isPgm);

    static bool
    _OutputHandler(u8 column, u8 page, u8 *data);

    bool
    OutputHandler(u8 column, u8 page, u8 *data);

    /** Start current request processing.
     *
     * @return True if request pending, false if no requests queued.
     */
    bool
    StartRequest();

    void
    NextRequest();

} __PACKED;

extern TextWriter textWriter;


#endif /* TEXT_WRITER_H_ */
