/* This file is a part of 'hydroponics' project.
 * Copyright (c) 2015, Artyom Lebedev <artyom.lebedev@gmail.com>
 * All rights reserved.
 * See LICENSE file for copyright details.
 */

#ifndef ADC_H_
#define ADC_H_

class Adc {
public:
    Adc();

    void
    Poll();

    void
    ScheduleConversion(u8 channel);

    /** Prevent MCU from sleeping when ADC conversion in progress (since it will
     * probably stop I/O clock and will fail the conversion).
     */
    bool
    SleepEnabled();


    void
    HandleInterrupt();

private:
    enum {
        NUM_CHANNELS = 9
    };
    u16 pendingChannelsMask;

    u8 curChannel:4,
       inProgress:1,
       reserved:3;

    void
    StartConversion(u8 channel);

    void
    NextConversion();

} __PACKED;

/** Called when conversion complete. Add needed calls to this function. */
void
OnAdcResult(u8 channel, u16 result);

extern Adc adc;

#endif /* ADC_H_ */
