/* This file is a part of 'hydroponics' project.
 * Copyright (c) 2012-2015, Artyom Lebedev <artyom.lebedev@gmail.com>
 * All rights reserved.
 * See COPYING file for copyright details.
 */

/** @file bitmap.h
 * TODO insert description here.
 */

#ifndef BITMAP_H_
#define BITMAP_H_

/** Bitmap descriptor. */
struct Bitmap {
    /** Pointer to data array if in data memory. Offset of data array relatively
     * to Bitmaps class instance start address if in program memory.
     */
    const u8 *data;
    /** Number of pages in the bitmap. */
    u8 numPages,
    /** Number of columns in the bitmap. */
       numColumns;
} __PACKED;

/** Helper class for outputting bitmaps to the graphical display. */
class BitmapWriter {
public:

    /** Called when output finished. */
    typedef void (*DoneHandler)();

    void
    Poll();

    inline void
    Write(u8 column, u8 page, Bitmap *bitmap, bool inversed = false,
          DoneHandler handler = nullptr)
    {
        Write(column, page, bitmap, inversed, handler, false);
    }

    inline void
    Write(u8 column, u8 page, const Bitmap *bitmap, bool inversed = false,
          DoneHandler handler = nullptr)
    {
        Write(column, page, bitmap, inversed, handler, true);
    }

    /** Clear space occupied by the provided bitmap. */
    inline void
    Clear(u8 column, u8 page, Bitmap *bitmap, bool inversed = false,
          DoneHandler handler = nullptr)
    {
        Write(column, page, bitmap, inversed, handler, false, true);
    }

    inline void
    Clear(u8 column, u8 page, const Bitmap *bitmap, bool inversed = false,
          DoneHandler handler = nullptr)
    {
        Write(column, page, bitmap, inversed, handler, true, true);
    }

private:
    enum {
        MAX_REQUESTS = 8
    };


    struct Request {
        /** Right column for drawing the bitmap. */
        u8 col:7,
        /** True if bitmap in program memory. */
           isPgm:1,

        /** Top page for drawing the bitmap. */
           page:3,
           inversed: 1,
        /** Clear space instead of drawing bitmap. */
           clear:1,
           reserved:3;

        const Bitmap *bmp;
        DoneHandler handler;
    } __PACKED;

    Request reqQueue[MAX_REQUESTS];

    /** Current request index in the queue. */
    u8 curReq:3,
    /** Request processing in progress. */
       reqInProgress:1,
       curBmpHeight:3,
       reserved:1,

       curBmpWidth:7,
       reserved2:1,

       curBmpCroppedWidth:7,
       reserved3:1,

       curBmpCroppedHeight:3,
       reserved4:5;
    const u8 *curBmpData;

    void
    Write(u8 column, u8 page, const Bitmap *bitmap, bool inversed,
          DoneHandler handler, bool isPgm, bool clear = false);

    /** Start current request processing.
     *
     * @return True if request pending, false if no requests queued.
     */
    bool
    StartRequest();

    void
    NextRequest();

    static bool
    _OutputHandler(u8 column, u8 page, u8 *data);

    bool
    OutputHandler(u8 column, u8 page, u8 *data);

} __PACKED;

extern BitmapWriter bitmapWriter;

#endif /* BITMAP_H_ */
