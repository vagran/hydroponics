/* This file is a part of 'hydroponics' project.
 * Copyright (c) 2015, Artyom Lebedev <artyom.lebedev@gmail.com>
 * All rights reserved.
 * See LICENSE file for copyright details.
 */
#ifndef TIME_H_
#define TIME_H_

/** Time representation. */
struct Time {
    u8 hour, min;

    u16
    TotalMinutes() const
    {
        return hour * 60 + min;
    }

    operator bool() const
    {
        return hour != 0 || min != 0;
    }

    bool
    operator ==(Time t) const
    {
        return TotalMinutes() == t.TotalMinutes();
    }

    bool
    operator !=(Time t) const
    {
        return !((*this) == t);
    }

    bool
    operator <(Time t) const
    {
        return TotalMinutes() < t.TotalMinutes();
    }

    bool
    operator >(Time t) const
    {
        return TotalMinutes() > t.TotalMinutes();
    }

    bool
    operator <=(Time t) const
    {
        return !((*this) > t);
    }

    bool
    operator >=(Time t) const
    {
        return !((*this) < t);
    }

    /** Get time difference. */
    void
    operator -=(Time t)
    {
        u16 totalMin = TotalMinutes();
        u16 tTotalMin = t.TotalMinutes();
        if (tTotalMin >= totalMin) {
            /* No negative difference. */
            hour = 0;
            min = 0;
            return;
        }
        totalMin -= tTotalMin;
        hour = totalMin / 60;
        min = totalMin - static_cast<u16>(hour) * 60;
    }

    Time
    operator -(Time t) const
    {
        Time res = *this;
        res -= t;
        return res;
    }

    void
    operator +=(Time t)
    {
        u16 totalMin = TotalMinutes() + t.TotalMinutes();
        hour = totalMin / 60;
        min = totalMin - static_cast<u16>(hour) * 60;
    }

    Time
    operator +(Time t) const
    {
        Time res = *this;
        res += t;
        return res;
    }
} __PACKED;

#endif /* TIME_H_ */
