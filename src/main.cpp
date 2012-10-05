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
#include "chprintf.h"

#define NORETURN __attribute__((noreturn))
#define ALIGNED(x) __attribute__((aligned(x)))

// heartbeat thread
static WORKING_AREA(waHeartbeat, 128);
NORETURN static void threadHeartbeat(void *arg) {
    (void) arg;
    chRegSetThreadName("heartbeat");
    palClearPad(GPIOB, GPIOB_LEDY);
    while (TRUE) {
        palTogglePad(GPIOB, GPIOB_LEDY);
        palTogglePad(GPIOC, GPIOC_LEDB);
        chThdSleepMilliseconds(1000);
    }
    chThdExit(0);
}

static BaseChannel * const debugChan = (BaseChannel *) &SD2;

static WORKING_AREA(waDebugEcho, 128);
NORETURN static void threadDebugEcho(void *arg) {
    (void) arg;
    chRegSetThreadName("debugEcho");
    while (TRUE) {
        const uint8_t ch = chIOGet(debugChan);
        chIOPut(debugChan, ch);
    }
    chThdExit(0);
}

int main(void) {
    halInit();
    chSysInit();

    const SerialConfig serialConfig = { 921000, 0, USART_CR2_STOP1_BITS | USART_CR2_LINEN, USART_CR3_CTSE
            | USART_CR3_RTSE };
    sdStart(&SD2, &serialConfig);

    chThdCreateStatic(waHeartbeat, sizeof(waHeartbeat), IDLEPRIO, tfunc_t(threadHeartbeat), nullptr);
    chThdCreateStatic(waDebugEcho, sizeof(waDebugEcho), LOWPRIO, tfunc_t(threadDebugEcho), nullptr);

    while (TRUE) {
        chThdSleepMilliseconds(1000);
    }
}
