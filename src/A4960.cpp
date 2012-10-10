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

#include "ch.h"
#include "hal.h"

#include "Tortilla.h"
#include "A4960.h"
#include "chprintf.h"

#include <stdint.h>

const uint16_t A4960::config[8] = {
        (0x0 << 10) | (0x6 << 6) | (0x4), // 50us comm blank time, 2.4us blank time, 200ns deadtime
        (0xf << 6) | (0x20), // Vri = 100% Vref, Vdsth = 800mV
        (0x08), // 22.8us current control off time
        (0x0 << 8) | (0x7 << 4) | (0x6), // current limited, 50% current for hold, 58ms hold time
        (0x3 << 4) | (0x5), // 1ms min comm time, 40ms start comm time
        (0x9 << 8) | (0xf << 4) | (0x0), // 16.875deg phase adv, 100% ramp current, 0.2ms ramp rate
        (0x0), // fault detection all on
        (0x0 << 10) | (0x3 << 7) | (0x0 << 6) | (0x0 << 4) | (0x1 << 3) | (0x0 << 2) | (0x0 << 1) | (0x1)
        // auto BEMF hyst, 3.2us zx det window, no stop on fail, DIAG pin = fault, restart on loss of sync, brake off, forward, run
};

A4960::A4960(SPIDriver *spip, PWMDriver *pwmp, pwmchannel_t channel) :
        spip(spip), pwmp(pwmp), channel(channel) {
    for (size_t addr = 0; addr < 8U; addr++) {
        writeReg(addr, config[addr]);
        chThdSleepMilliseconds(1);
    }
    pwmEnableChannel(pwmp, channel, 0);
}

uint16_t A4960::writeReg(const uint8_t addr, const uint16_t data) {
    const uint16_t txData = (uint16_t(addr & 0x7) << 13) | 0x1000 | (data & 0xfff);
    uint16_t rxData;
    spiAcquireBus(spip);
    spiSelect(spip);
    spiExchange(spip, 1, &txData, &rxData);
    spiUnselect(spip);
    spiReleaseBus(spip);

    return rxData;
}

uint16_t A4960::readReg(const uint8_t addr) {
    const uint16_t txData = (uint16_t(addr & 0x7) << 13);
    uint16_t rxData;
    spiAcquireBus(spip);
    spiSelect(spip);
    spiExchange(spip, 1, &txData, &rxData);
    spiUnselect(spip);
    spiReleaseBus(spip);

    return rxData;
}
