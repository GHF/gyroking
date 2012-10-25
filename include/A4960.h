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

#ifndef A4960_H_
#define A4960_H_

#include "ch.h"
#include "hal.h"

class A4960 {
public:
    A4960(SPIDriver *spip, PWMDriver *pwmp, pwmchannel_t channel);

    void setMode(bool enable, bool reverse = false) {
        uint16_t cfg = config[0x7];
        cfg |= enable ? 0x1 : 0x0; // RUN bit
        cfg |= reverse ? 0x2 : 0x0; // DIR bit
        writeReg(0x7, cfg);
//        writeReg(0x7, reverse ? (config[0x7] | 0x2) : (config[0x7]));
    }

    void setWidth(pwmcnt_t width) {
        pwmEnableChannel(pwmp, channel, width);
    }

    uint16_t writeReg(const uint8_t addr, const uint16_t data);
    uint16_t readReg(const uint8_t addr);

protected:
    SPIDriver * const spip;
    PWMDriver * const pwmp;
    const pwmchannel_t channel;

    static const uint16_t config[8];
};

#endif /* A4960_H_ */
