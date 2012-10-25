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
#include "ADS1259.h"
#include "chprintf.h"

// heartbeat thread
//static WORKING_AREA(waHeartbeat, 128);
//NORETURN static void threadHeartbeat(void *arg) {
//    (void) arg;
//    chRegSetThreadName("heartbeat");
//    while (TRUE) {
//        palTogglePad(GPIOB, GPIOB_LED2);
//        chThdSleepMilliseconds(1000);
//    }
//    chThdExit(0);
//}

static WORKING_AREA(waIO, 8192);
NORETURN static void threadIO(void *tortilla) {
    chRegSetThreadName("IO");
    static_cast<Tortilla *>(tortilla)->ioLoop();
    chThdExit(0);
}

int main(void) {
    halInit();
    chSysInit();

    chThdSleepMilliseconds(200);

    // input capture & high-res timer
    const ICUConfig icuConfig = { ICU_INPUT_ACTIVE_HIGH, 1000000, nullptr, nullptr, nullptr, nullptr, nullptr };
    icuStart(&TIMING_ICU, &icuConfig);
    icuEnable(&TIMING_ICU);

    // serial setup
    const SerialConfig btSerialConfig = { 921600, 0, USART_CR2_STOP1_BITS, USART_CR3_CTSE | USART_CR3_RTSE };
    sdStart(&BT_SERIAL, &btSerialConfig);

    // PWM setup
    const PWMConfig mPWMConfig = { STM32_TIMCLK1, PWM_PERIOD, nullptr, {
            { PWM_OUTPUT_DISABLED, nullptr },
            { PWM_OUTPUT_DISABLED, nullptr },
            { PWM_OUTPUT_ACTIVE_HIGH, nullptr },
            { PWM_OUTPUT_ACTIVE_HIGH, nullptr } }, 0, };
    pwmStart(&M1_PWM, &mPWMConfig);

    // SPI setup
    // speed = pclk/8 = 5.25MHz
    const SPIConfig m1SPIConfig = { NULL, GPIOC, GPIOC_M1_NSS, SPI_CR1_DFF | SPI_CR1_BR_1 };
    const SPIConfig m2SPIConfig = { NULL, GPIOD, GPIOD_M2_NSS, SPI_CR1_DFF | SPI_CR1_BR_1 };
    const SPIConfig adcSPIConfig = { NULL, GPIOA, GPIOA_ADC_NSS, SPI_CR1_BR_2 | SPI_CR1_CPHA };
    spiStart(&M1_SPI, &m1SPIConfig);
    spiStart(&M2_SPI, &m2SPIConfig);
    spiStart(&ADC_SPI, &adcSPIConfig);

    // motor setup
    A4960 m1(&M1_SPI, &M1_PWM, M1_PWM_CHAN);
    A4960 m2(&M2_SPI, &M2_PWM, M2_PWM_CHAN);

    // ADC setup
    ADS1259 adc(&ADC_SPI);

    // initialize control structure
    Tortilla tortilla(m1, m2, adc, &TIMING_ICU, &BT_SERIAL);

    // start slave threads
//    chThdCreateStatic(waHeartbeat, sizeof(waHeartbeat), IDLEPRIO, tfunc_t(threadHeartbeat), nullptr);
    chThdCreateStatic(waIO, sizeof(waIO), LOWPRIO, tfunc_t(threadIO), &tortilla);

    // done with setup
    palClearPad(GPIOC, GPIOC_LEDB);
    palClearPad(GPIOB, GPIOB_LED2);
    tortilla.fastLoop();
}
