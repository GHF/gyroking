/*
 *  Copyright (C) 2012 Xo Wang, Aaron Fan
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
#include "chprintf.h"
#include "A4960.h"
#include "ADS1259.h"

#include <algorithm>

#include <stdint.h>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cmath>

Tortilla::Tortilla(A4960 &m1, A4960 &m2, ADS1259 &adc, ICUDriver *icup) :
        m1(m1), m2(m2), adc(adc), icup(icup), gyroBias(0), lastRate(0), currentRate(0), theta(0) {
}

void Tortilla::initGyroBias() {
    int32_t adcReading;
    int64_t sumReadings = 0;
    size_t numReadings = 0;
    for (size_t i = 0; i < 2000U; i++) {
        if (adc.read(adcReading)) {
            numReadings++;
            sumReadings += adcReading;
        }
        chThdSleepMilliseconds(1);
    }
    gyroBias = sumReadings / numReadings;
}

static constexpr float SCALE_RAW_TO_RADSEC = float(50000.0 * (M_PI / 180.0) / double(0x7FFFFFUL));

static constexpr size_t LOOP_DELAY_US = 1000;
static constexpr systime_t LOOP_DELAY = US2ST(LOOP_DELAY_US);
static constexpr float DT = LOOP_DELAY_US / 1000000.0;

void Tortilla::fastLoop() {
    initGyroBias();
    palClearPad(GPIOB, GPIOB_LEDY);

    systime_t ticks = chTimeNow();
    while (true) {
        int32_t adcReading;
        if (adc.read(adcReading)) {
            const float rate = SCALE_RAW_TO_RADSEC * (adcReading - gyroBias);

            lastRate = currentRate;
            currentRate = rate;
            theta += 0.5f * (currentRate + lastRate) * DT;
            theta = remainder(theta, M_PI);
        }

        palWritePad(GPIOC, GPIOC_LEDB, fabsf(theta) < 0.2f);

        ticks += LOOP_DELAY;
        chThdSleepUntil(ticks);
    }
}

constexpr char IO_START = '%';
constexpr size_t OUTPUT_BUFFER_SIZE = 30;
constexpr size_t COMMAND_DATA_BUFFER_SIZE = 27;

constexpr systime_t IO_TIMEOUT = MS2ST(100);
constexpr uint8_t IO_FREQ = 10;

constexpr char IO_INFO_ACK = 'k';
constexpr char IO_INFO_ROB_RPM = 'P';
constexpr char IO_INFO_GYRO_BIAS = 'G';
constexpr char IO_INFO_MOTOR_RPM = 'r';
constexpr char IO_INFO_MISC = 'C';

constexpr char IO_MOD_GYRO_BIAS = 'g';
constexpr char IO_MOD_TRANS_BIAS = 't';

constexpr char IO_CMD_MOV = 'm';
constexpr char IO_CMD_STOP = 's';

void Tortilla::ioLoop() {
    BaseChannel * const ioChan = (BaseChannel *) &BT_SERIAL;

    uint8_t new_char;
    uint8_t timeout_counter = 0;

    uint8_t command_length = 0; // set by LENGTH
    uint8_t command_code = 0; // set by OPCODE
    uint8_t command_data[COMMAND_DATA_BUFFER_SIZE]; // set by DATA
    uint8_t command_data_index = 0; // set by OPCODE

    enum {
        START, LENGTH, OPCODE, DATA, DONE
    } state;

    state = START;

    while (1) {
        const msg_t getResult = chIOGetTimeout(ioChan, IO_TIMEOUT);
        if (getResult == Q_TIMEOUT || getResult == Q_RESET) {
            // timeout happened, clear buffer, check how many timeouts we've gotten
            timeout_counter++;
            if (timeout_counter == IO_FREQ) {
                // 1 second has passed since successful communication with PC
                m1.setWidth(0);
                m2.setWidth(0);
                // failsafe stop
                timeout_counter = 0;
            }
            continue;
        }

        new_char = getResult;
        timeout_counter = 0;

        switch (state) {
        case START: {
            if (new_char == IO_START) {
                state = LENGTH;
            }
            break;
        }

        case LENGTH: {
            if (new_char >= 'a' && new_char <= 'z') {
                command_length = new_char - 'a';
                state = OPCODE;
            } else if (new_char == IO_START) {
                state = LENGTH;
            } else {
                state = START;
            }
            break;
        }

        case OPCODE: {
            if ((new_char >= 'a' && new_char <= 'z') || (new_char >= 'A' && new_char <= 'Z')) {
                command_code = new_char;
                state = DATA;
                command_data_index = 0;
            } else if (new_char == IO_START) {
                state = LENGTH;
            } else {
                state = START;
            }
            break;
        }

        case DATA: {
            if (new_char >= '0' && new_char <= '9') {
                command_data[command_data_index] = new_char - '0';
                command_data_index++;

                if (command_data_index == command_length || command_data_index >= COMMAND_DATA_BUFFER_SIZE) {
                    state = DONE;
                }
            } else if (new_char == IO_START) {
                state = LENGTH;
            } else {
                state = START;
            }
            break;
        }

        case DONE: {
            if (new_char == IO_START) {
                state = LENGTH;
            } else {
                state = START;
            }
            break;
        }

        default:
            state = START;
            break;
        }

        if (state == DONE) {
            char strBuffer[26];

            switch (command_code) {
            case IO_CMD_MOV: {
                const int16_t x_mov = command_data[0] * 100L + command_data[1] * 10L + command_data[2] - 500;
                const int16_t y_mov = command_data[3] * 100L + command_data[4] * 10L + command_data[5] - 500;
                const int16_t spin = command_data[6] * 100L + command_data[7] * 10L + command_data[8] - 500;

                const size_t numChars = snprintf(strBuffer, sizeof(strBuffer), "mov %d %d %d", x_mov, y_mov, spin);
                chprintf(ioChan, "%%%c%c%s", 'a' + numChars, IO_INFO_MISC, strBuffer);

                const bool reversed = spin < 0;
                const int throttle = std::min(abs(spin), 500);
                m1.setDirection(reversed);
                m1.setWidth(PWM_PERIOD * throttle / 500);
                m2.setDirection(reversed);
                m2.setWidth(PWM_PERIOD * throttle / 500);
                break;
            }

            case IO_CMD_STOP: {
                // stop
                break;
            }

            case IO_MOD_GYRO_BIAS: {
                const int16_t new_bias = command_data[0] * 1000L + command_data[1] * 100L + command_data[2] * 10L
                        + command_data[3] - 5000;
                break;
            }

            case IO_MOD_TRANS_BIAS: {
                const int16_t new_bias = command_data[0] * 100L + command_data[1] * 10L + command_data[2] - 180;
                break;
            }

            default:
                break;
            }

            const int calculated_rpm = 100;
            chprintf(ioChan, "%%%c%c%.4d", 'a' + 4, IO_INFO_ROB_RPM, calculated_rpm);
        }
    }
}
