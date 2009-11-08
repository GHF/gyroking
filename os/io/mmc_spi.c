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
 * @file mmc_spi.c
 * @brief MMC over SPI driver code
 * @addtogroup MMC_SPI
 * @{
 */

#include <ch.h>
#include <spi.h>
#include <mmc_spi.h>

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

void tmrfunc(void *p) {
  MMCDriver *mmcp = p;

  if (mmcp->mmc_cnt > 0) {
    if (mmcp->mmc_is_inserted()) {
      if (--mmcp->mmc_cnt == 0) {
        mmcp->mmc_state = MMC_INSERTED;
        chEvtBroadcastI(&mmcp->mmc_inserted_event);
      }
    }
    else
      mmcp->mmc_cnt = MMC_POLLING_INTERVAL;
  }
  else {
    if (!mmcp->mmc_is_inserted()) {
      mmcp->mmc_state = MMC_WAIT;
      mmcp->mmc_cnt = MMC_POLLING_INTERVAL;
      chEvtBroadcastI(&mmcp->mmc_removed_event);
    }
  }
  chVTSetI(&mmcp->mmc_vt, MS2ST(MMC_POLLING_DELAY), tmrfunc, mmcp);
}

/**
 * @brief Waits an idle condition.
 *
 * @param[in] mmcp      pointer to the @p MMCDriver object
 */
static void wait(MMCDriver *mmcp) {
  int i;
  uint8_t buf[4];

  for (i = 0; i < 16; i++) {
    spiReceive(mmcp->mmc_spip, 1, buf);
    if (buf[0] == 0xFF)
      break;
  }
  /* Looks like it is a long wait.*/
  while (TRUE) {
    spiReceive(mmcp->mmc_spip, 1, buf);
    if (buf[0] == 0xFF)
      break;
#ifdef MMC_NICE_WAITING
    /* Trying to be nice with the other threads.*/
    chThdSleep(1);
#endif
  }
}

/**
 * @brief Sends a command header.
 *
 * @param[in] mmcp      pointer to the @p MMCDriver object
 * @param cmd[in]       the command id
 * @param arg[in]       the command argument
 */
static void send_hdr(MMCDriver *mmcp, uint8_t cmd, uint32_t arg) {
  uint8_t buf[6];

  /* Wait for the bus to become idle if a write operation was in progress. */
  wait(mmcp);

  buf[0] = 0x40 | cmd;
  buf[1] = arg >> 24;
  buf[2] = arg >> 16;
  buf[3] = arg >> 8;
  buf[4] = arg;
  buf[5] = 0x95;        /* Valid for CMD0 ignored by other commands. */
  spiSend(mmcp->mmc_spip, 6, buf);
}

/**
 * @brief Receives a single byte response.
 *
 * @param[in] mmcp      pointer to the @p MMCDriver object
 *
 * @return The response as an @p uint8_t value.
 * @retval 0xFF         timed out.
 */
static uint8_t recvr1(MMCDriver *mmcp) {
  int i;
  uint8_t r1[1];

  for (i = 0; i < 9; i++) {
    spiReceive(mmcp->mmc_spip, 1, r1);
    if (r1[0] != 0xFF)
      return r1[0];
  }
  return 0xFF;
}

/**
 * @brief Sends a command an returns a single byte response.
 *
 * @param[in] mmcp      pointer to the @p MMCDriver object
 * @param cmd[in]       the command id
 * @param arg[in]       the command argument
 *
 * @return The response as an @p uint8_t value.
 * @retval 0xFF         timed out.
 */
static uint8_t send_command(MMCDriver *mmcp, uint8_t cmd, uint32_t arg) {
  uint8_t r1;

  spiSelect(mmcp->mmc_spip);
  send_hdr(mmcp, cmd, arg);
  r1 = recvr1(mmcp);
  spiUnselect(mmcp->mmc_spip);
  return r1;
}

/**
 * @brief Receives a 512 bytes block and ignores 2 CRC bytes.
 *
 * @param[in] mmcp      pointer to the @p MMCDriver object
 * @param[out] buf      pointer to the buffer
 *
 * @return The operation status.
 * @retval FALSE        the operation was successful.
 * @retval TRUE         the operation timed out.
 */
static bool_t get_data(MMCDriver *mmcp, uint8_t *buf) {
  int i;
  uint8_t ignored[2];

  for (i = 0; i < MMC_WAIT_DATA; i++) {
    spiReceive(mmcp->mmc_spip, 1, buf);
    if (buf[0] == 0xFE) {
      spiReceive(mmcp->mmc_spip, 512, buf);
      /* CRC ignored. */
      spiReceive(mmcp->mmc_spip, 2, ignored);
      return FALSE;
    }
  }
  /* Timeout.*/
  return TRUE;
}

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief MMC over SPI driver initialization.
 */
void mmcInit(void) {

}

/**
 * @brief Initializes an instance.
 *
 * @param[in] mmcp      pointer to the @p MMCDriver object
 */
void mmcObjectInit(MMCDriver *mmcp, SPIDriver *spip,
                   const SPIConfig *lscfg, const SPIConfig *hscfg,
                   mmcquery_t is_protected, mmcquery_t is_inserted) {

  mmcp->mmc_state = MMC_STOP;
  mmcp->mmc_config = NULL;
  mmcp->mmc_spip = spip;
  mmcp->mmc_lscfg = lscfg;
  mmcp->mmc_hscfg = hscfg;
  mmcp->mmc_is_protected = is_protected;
  mmcp->mmc_is_inserted = is_inserted;
  chEvtInit(&mmcp->mmc_inserted_event);
  chEvtInit(&mmcp->mmc_removed_event);
}

/**
 * @brief Configures and activates the MMC peripheral.
 *
 * @param[in] mmcp      pointer to the @p MMCDriver object
 * @param[in] config    pointer to the @p MMCConfig object
 */
void mmcStart(MMCDriver *mmcp, const MMCConfig *config) {

  chDbgCheck((mmcp != NULL) && (config != NULL), "mmcStart");

  chSysLock();
  chDbgAssert(mmcp->mmc_state == MMC_STOP, "mmcStart(), #1", "invalid state");
  mmcp->mmc_config = config;
  mmcp->mmc_state = MMC_WAIT;
  mmcp->mmc_cnt = MMC_POLLING_INTERVAL;
  chVTSetI(&mmcp->mmc_vt, MS2ST(MMC_POLLING_DELAY), tmrfunc, mmcp);
  chSysUnlock();
}

/**
 * @brief Deactivates the MMC peripheral.
 *
 * @param[in] mmcp      pointer to the @p MMCDriver object
 */
void mmcStop(MMCDriver *mmcp) {

  chDbgCheck(mmcp != NULL, "mmcStop");

  chSysLock();
  chDbgAssert((mmcp->mmc_state != MMC_UNINIT) &&
              (mmcp->mmc_state != MMC_RUNNING),
              "mmcStop(), #1",
              "invalid state");
  if (mmcp->mmc_state != MMC_STOP) {
    mmcp->mmc_state = MMC_STOP;
    chVTResetI(&mmcp->mmc_vt);
    spiStop(mmcp->mmc_spip);
  }
  chSysUnlock();
}

/**
 * @brief Performs the initialization procedure on the inserted card.
 * @details This function should be invoked when a card is inserted and
 *          brings the driver in the @p MMC_READY state where it is possible
 *          to perform read and write operations.
 * @note It is possible to invoke this function from the insertion event
 *       handler.
 *
 * @param[in] mmcp      pointer to the @p MMCDriver object
 *
 * @return The operation status.
 * @retval FALSE        the operation was successful and the driver is now
 *                      in the @p MMC_READY state.
 * @retval TRUE         the operation failed.
 */
bool_t mmcOpen(MMCDriver *mmcp) {
  unsigned i;

  /* Slow clock mode and 128 clock pulses.*/
  spiStart(mmcp->mmc_spip, mmcp->mmc_lscfg);

  /* SPI mode selection.*/
  i = 0;
  while (TRUE) {
    if (send_command(mmcp, MMC_CMDGOIDLE, 0) == 0x01)
      break;
    if (++i >= MMC_CMD0_RETRY)
      return TRUE;
    chThdSleepMilliseconds(10);
  }

  /* Initialization. */
  i = 0;
  while (TRUE) {
    uint8_t b = send_command(mmcp, MMC_CMDINIT, 0);
    if (b == 0x00)
      break;
    if (b != 0x01)
      return TRUE;
    if (++i >= MMC_CMD1_RETRY)
      return TRUE;
    chThdSleepMilliseconds(10);
  }

  /* Initialization complete, full speed. */
  spiStart(mmcp->mmc_spip, mmcp->mmc_hscfg);
  return FALSE;
}

/** @} */
