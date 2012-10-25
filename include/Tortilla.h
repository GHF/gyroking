/*
 *  Copyright (C) 2012 Xo Wang
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to
 *  deal in the Software without restriction, including without limitation the
 *  rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 *  sell copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL XO
 *  WANG BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 *  AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 *  CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *  Except as contained in this notice, the name of Xo Wang shall not be
 *  used in advertising or otherwise to promote the sale, use or other dealings
 *  in this Software without prior written authorization from Xo Wang.
 */

#ifndef TORTILLA_H_
#define TORTILLA_H_

#define NORETURN __attribute__((noreturn))
#define ALIGNED(x) __attribute__((aligned(x)))

#define BT_SERIAL (SD2)

#define ADC_SPI (SPID1)

#define M1_SPI (SPID2)
#define M2_SPI (SPID3)

#define PWM_FREQ 20000
#define PWM_PERIOD (STM32_TIMCLK1 / PWM_FREQ)

#define M1_PWM (PWMD3)
#define M1_PWM_CHAN (3)
#define M2_PWM (PWMD3)
#define M2_PWM_CHAN (2)

#define TIMING_ICU (ICUD2)

class A4960;
class ADS1259;
struct ICUDriver;

class Tortilla {
public:
    Tortilla(A4960 &m1, A4960 &m2, ADS1259 &adc, ICUDriver *icup, SerialDriver *sdp);

    NORETURN void ioLoop();
    NORETURN void fastLoop();
    NORETURN void watchLoop();

protected:
    A4960 &m1;
    A4960 &m2;
    ADS1259 &adc;
    ICUDriver * const icup;
    SerialDriver * const sdp;

    int32_t gyroBias;
    float lastRate;
    float currentRate;
    float theta;

    int throttle;
    bool reversed;
    float joyAngle;
    float joyMag;
    float gyroTrim;
    int translationTrim;

    void initGyroBias();
};

#endif /* TORTILLA_H_ */
