/*
 *  Copyright (C) 2011--2012 Xo Wang
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

#include "ads1259.h"

const uint8_t ADS1259::config[2] = { 0xD0, 0x05 }; // CONFIG2 set data rate 1200SPS
const uint8_t ADS1259::OP_START = 0x08;

ADS1259::ADS1259(SPIDriver *spip) :
        spip(spip) {
    writeReg(0x1, 2, config); // configure
    start(); // start conversions
}

void ADS1259::writeReg(uint8_t addr, uint8_t n, const uint8_t *data) {
    const uint8_t opcodes[2] = { uint8_t(0x40 + (addr & 0xF)), uint8_t((n - 1) & 0xF) };
    spiAcquireBus(spip);
    spiSelect(spip);
    spiSend(spip, 2, opcodes);
    spiSend(spip, n, data);
    spiUnselect(spip);
    spiReleaseBus(spip);
}

void ADS1259::start(void) {
    spiAcquireBus(spip);
    spiSelect(spip);
    chThdSleepMicroseconds(100);
    spiSend(spip, 1, &OP_START);
    spiUnselect(spip);
    spiReleaseBus(spip);
}

bool ADS1259::read(int32_t &reading) {
    spiAcquireBus(spip);
    spiSelect(spip);
    uint8_t data[4];
    spiReceive(spip, 4, data);
    spiUnselect(spip);
    spiReleaseBus(spip);

    reading = ((int8_t(data[0]) >> 7) << 24) + (data[0] << 16) + (data[1] << 8) + data[2];
    const uint8_t checksum = (data[0] + data[1] + data[2] + 0x9b) & 0x7f;
    return (checksum == data[3]);
}
