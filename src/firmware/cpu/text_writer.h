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

enum {
    FONT_WIDTH = 6
};

/** Helper class for text output to graphical display. */
class TextWriter {
public:
    /** Called when output finished. */
    typedef void (*DoneHandler)();

    void
    Poll();

    /** Write text to display.
     *
     * @param vp Viewport to write text in.
     * @param text Text to write.
     * @param inversed Inverse pixels.
     * @param fillVp Fill the specified viewport fully - even after all
     *      characters written.
     * @param handler Handler to call when done.
     */
    inline void
    Write(Display::Viewport vp, char *text, bool inversed = false,
          bool fillVp = false, DoneHandler handler = nullptr)
    {
        Write(vp, text, inversed, false, fillVp, handler, false);
    }

    inline void
    Write(Display::Viewport vp, const char *text, bool inversed = false,
          bool fillVp = false, DoneHandler handler = nullptr)
    {
        Write(vp, text, inversed, false, fillVp, handler, true);
    }

    /** Clear space occupied by the provided text. */
    inline void
    Clear(Display::Viewport vp, char *text, bool inversed = false,
          bool fillVp = false, DoneHandler handler = nullptr)
    {
        Write(vp, text, inversed, true, fillVp, handler, false);
    }

    inline void
    Clear(Display::Viewport vp, const char *text, bool inversed = false,
          bool fillVp = false, DoneHandler handler = nullptr)
    {
        Write(vp, text, inversed, true, fillVp, handler, true);
    }

private:
    enum {
        MAX_REQUESTS = 8,
        /** Width in pixels of space between characters in a word. */
        CHAR_SPACE_WIDTH = 1
    };

    struct Request {
        const char *text;
        DoneHandler handler;
        Display::Viewport vp;
        u8 isPgm:1,
           inversed:1,
           fillVp:1,
           clear:1,
           reserved:4;
    } __PACKED;

    Request reqQueue[MAX_REQUESTS];
    /** Current request index in the queue. */
    u8 curReq:3,
    /** Current column in the character. One additional column for space. */
       curCharCol:3,
    /** Text fully written, filling trailing viewport space. */
       fillingTail:1,
       reserved:1,

    /** Next character to output. */
       curChar:7,
    /** Request processing in progress. */
       reqInProgress:1;


    void
    Write(Display::Viewport vp, const char *text, bool inversed, bool clear,
          bool fillVp, DoneHandler handler, bool isPgm);

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
