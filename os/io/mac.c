/*
    ChibiOS/RT - Copyright (C) 2006-2007 Giovanni Di Sirio.

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
*/

/**
 * @file mac.c
 * @brief MAC Driver code.
 * @addtogroup MAC
 * @{
 */

#include <ch.h>
#include <mac.h>

/**
 * @brief Transmit descriptors counter semaphore.
 */
static Semaphore tdsem;

/**
 * @brief MAC Driver initialization.
 */
void macInit(void) {

  chSemInit(&tdsem, MAC_TRANSMIT_DESCRIPTORS);
  mac_lld_init();
}

/**
 * @brief MAC address setup.
 *
 * @param[in] p pointer to a six bytes buffer containing the MAC address. If
 *            this parameter is set to @p NULL then a system default MAC is
 *            used.
 *
 * @note This function should be invoked after the @p macInit() and before
 *       @p macStart() else the result is unspecified (performed or ignored).
 */
void macSetAddress(uint8_t *p) {

  mac_lld_set_address(p);
}

/**
 * @brief Startsthe I/O activity and enters a low power mode.
 */
void macStart(void) {

  mac_lld_start();
}

/**
 * @brief Stops the I/O activity.
 */
void macStop(void) {

  max_lld_stop();
  chSemReset(&tdsem, MAC_TRANSMIT_DESCRIPTORS);
}

/**
 * @brief Allocates a transmission descriptor.
 * @details One of the available transmission descriptors is locked and
 *          returned. If a descriptor is not currently available then the
 *          invoking thread is queued until one is freed.
 *
 * @param[in] time the number of ticks before the operation timeouts,
 *             the following special values are allowed:
 *             - @a TIME_IMMEDIATE immediate timeout.
 *             - @a TIME_INFINITE no timeout.
 *             .
 * @return A pointer to a @p MACTransmitDescriptor structure or @p NULL if
 *         the operation timed out or the driver went in stop mode.
 */
MACTransmitDescriptor *macGetTransmitDescriptor(systime_t time) {
  MACTransmitDescriptor *tdp;

  chSysLock();

  if (chSemWaitTimeoutS(&tdsem, time) == RDY_OK)
    tdp = max_lld_get_transmit_descriptor();
  else
    tdp = NULL;

  chSysUnlock();
  return tdp;
}

/**
 * @brief Releases a transmit descriptor and starts the transmission of the
 *        enqueued data as a single frame.
 *
 * @param[in] tdp the pointer to the @p MACTransmitDescriptor structure
 */
void macReleaseTransmitDescriptor(MACTransmitDescriptor *tdp) {

  chSysLock();

  mac_lld_release_transmit_descriptor(tdp);

  chSysUnlock();
}

/**
 * @brief Enqueues data to a @p MACTransmitDescriptor.
 *
 * @param[in] tdp the pointer to the @p MACTransmitDescriptor structure
 * @param buf pointer to the data buffer
 * @param size size of the data to be enqueued
 */
void macAddTransmitData(MACTransmitDescriptor *tdp,
                        uint8_t *buf,
                        size_t size) {

  mac_lld_add_transmit_data(tdp, buf, size);
}

/** @} */