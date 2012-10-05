/*
 ChibiOS/RT - Copyright (C) 2006,2007,2008,2009,2010,
 2011,2012 Giovanni Di Sirio.

 This file is part of ChibiOS/RT.

 ChibiOS/RT is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3 of the License, or
 (at your option) any later version.

 ChibiOS/RT is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.

 ---

 A special exception to the GPL can be applied should you wish to distribute
 a combined work that includes ChibiOS/RT, without being obliged to provide
 the source code for any proprietary components. See the file exception.txt
 for full details of how and when the exception can be applied.
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
