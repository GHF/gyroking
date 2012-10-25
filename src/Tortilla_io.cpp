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

#include <algorithm>

#include <stdint.h>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cmath>

constexpr char IO_START = '%';
constexpr size_t OUTPUT_BUFFER_SIZE = 30;
constexpr size_t COMMAND_DATA_BUFFER_SIZE = 27;

constexpr systime_t IO_TIMEOUT = MS2ST(100);
constexpr uint8_t IO_FREQ = 15;

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
    BaseChannel * const ioChan = (BaseChannel *) sdp;

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
                throttle = 0;
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
                const int16_t x_mov = command_data[0] * 100L + command_data[1] * 10L + command_data[2] - 500L;
                const int16_t y_mov = command_data[3] * 100L + command_data[4] * 10L + command_data[5] - 500L;
                const int16_t spin = command_data[6] * 100L + command_data[7] * 10L + command_data[8] - 500L;

                const size_t numChars = snprintf(strBuffer,
                        sizeof(strBuffer),
                        "t: %ld",
                        int32_t(theta * (180.0 / M_PI)));
                chprintf(ioChan, "%%%c%c%s", 'a' + numChars, IO_INFO_MISC, strBuffer);

                const bool enabled = !(spin == 0 || spin == -1);
                reversed = (spin < 0);
                m1.setMode(enabled, reversed);
                m2.setMode(enabled, reversed);
                throttle = PWM_PERIOD * std::min(::abs(spin) - reversed, 499) / 499;

                joyAngle = ::remainderf(::atan2f(x_mov, y_mov), M_TWOPI);
                const float scaledJoyAngle = ::sqrtf(x_mov * x_mov + y_mov * y_mov) * float(1.0 / 500.0);
                joyMag = std::max(std::min(scaledJoyAngle, 1.f), -1.f);

                break;
            }

            case IO_CMD_STOP: {
                throttle = 0;
                m1.setMode(true);
                m2.setMode(true);
                break;
            }

            case IO_MOD_GYRO_BIAS: {
                const int16_t new_gyro_bias = command_data[0] * 1000L + command_data[1] * 100L + command_data[2] * 10L
                        + command_data[3] - 5000L;
                gyroTrim = new_gyro_bias * float(M_PI / 180.0);
//                const size_t numChars = snprintf(strBuffer,
//                        sizeof(strBuffer),
//                        "trim: %ld",
//                        int32_t(gyroTrim * (180.0 / M_PI)));
//                chprintf(ioChan, "%%%c%c%s", 'a' + numChars, IO_INFO_MISC, strBuffer);
                break;
            }

            case IO_MOD_TRANS_BIAS: {
                const int16_t new_trans_bias = command_data[0] * 100L + command_data[1] * 10L + command_data[2] - 180L;
                translationTrim = new_trans_bias;
                break;
            }

            default:
                break;
            }

            const int calculated_rpm = ::abs(currentRate * float(60.0 * 0.5 / M_PI));
            chprintf(ioChan, "%%%c%c%.4d", 'a' + 4, IO_INFO_ROB_RPM, calculated_rpm);
        }
    }
}
