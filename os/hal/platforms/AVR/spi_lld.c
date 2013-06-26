/*
    ChibiOS/RT - Copyright (C) 2006,2007,2008,2009,2010,
                 2011,2012,2013 Giovanni Di Sirio.

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
 * @file    AVR/spi_lld.c
 * @brief   AVR SPI subsystem low level driver source.
 *
 * @addtogroup SPI
 * @{
 */

#include "ch.h"
#include "hal.h"

#if HAL_USE_SPI || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/**
 * @brief   SPI1 driver identifier.
 */
#if AVR_SPI_USE_SPI1 || defined(__DOXYGEN__)
SPIDriver SPID1;
#endif

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

#if AVR_SPI_USE_SPI1 || defined(__DOXYGEN__)
/**
 * @brief   SPI event interrupt handler.
 *
 * @notapi
 */
#if 0
CH_IRQ_HANDLER(SPI_STC_vect) {
  CH_IRQ_PROLOGUE();

  SPIDriver *spip = &SPID1;
  bool_t done;

  /* spi_lld_exchange */
  if (spip->rxbuf && spip->txbuf) { // tx and rx
    spip->rxbuf[spip->rxidx++] = SPDR;  // receive
    if (spip->rxidx == spip->rxbytes) { // rx done
      _spi_isr_code(spip);
    } else {
      /* BUG: read beyond end of tx register possible */
      SPDR = spip->txbuf[spip->txidx++]; // transmit
    }
  /* spi_lld_send */
  } else if (spip->txbuf) { // tx only
    if (spip->txidx < spip->txbytes) { 
      SPDR = spip->txbuf[spip->txidx++]; // transmit
    } else { // tx done
      _spi_isr_code(spip);
    }
  /* spi_lld_receive */
  } else if (spip->rxbuf) { // rx only
    spip->rxbuf[spip->rxidx++] = SPDR;  // receive
    if (spip->rxidx < spip->rxbytes) {
      /* must keep clocking SCK */
      SPDR = 0;
    } else { // rx done
      _spi_isr_code(spip);
    }
  /* spi_lld_ignore */
  } else {
    /* BUG: txidx is never incremented */
    if (spip->txidx < spip->txbytes) {
      SPDR = 0;
    } else {
      _spi_isr_code(spip);
    }
  }

  CH_IRQ_EPILOGUE();
}
#endif
CH_IRQ_HANDLER(SPI_STC_vect) {
  CH_IRQ_PROLOGUE();

  SPIDriver *spip = &SPID1;

  /* spi_lld_exchange */
  /* spi_lld_receive */
  if (spip->rxbuf && spip->rxidx < spip->rxbytes) {
    spip->rxbuf[spip->rxidx++] = SPDR;  // receive
  }

  /* spi_lld_exchange */
  /* spi_lld_send */
  /* spi_lld_ignore */
  if (spip->txidx < spip->txbytes) {
    if (spip->txbuf) { 
      SPDR = spip->txbuf[spip->txidx++]; // send
    } else {
      SPDR = 0; spip->txidx++; // dummy send
    }
  }

  /* spi_lld_send */
  else if (spip->rxidx < spip->rxbytes) { /* rx not done */
    SPDR = 0; // dummy send to keep the clock going
  }

  /* rx done and tx done */
  if (spip->rxidx >= spip->rxbytes && spip->txidx >= spip->txbytes) { 
    _spi_isr_code(spip);
  }

  CH_IRQ_EPILOGUE();
}
#endif /* AVR_SPI_USE_SPI1 */

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Low level SPI driver initialization.
 *
 * @notapi
 */
void spi_lld_init(void) {

#if AVR_SPI_USE_SPI1
  /* Driver initialization.*/
  spiObjectInit(&SPID1);
#endif /* AVR_SPI_USE_SPI1 */
}

/**
 * @brief   Configures and activates the SPI peripheral.
 *
 * @param[in] spip      pointer to the @p SPIDriver object
 *
 * @notapi
 */
void spi_lld_start(SPIDriver *spip) {

  uint8_t dummy;

  /* Configures the peripheral.*/

  if (spip->state == SPI_STOP) {
    /* Enables the peripheral.*/
#if AVR_SPI_USE_SPI1
    if (&SPID1 == spip) {
      /* enable SPI clock */
/* Enable SPI power using Power Reduction Register */
#if defined(PRR0)
      PRR0 &= ~(1 << PRSPI);
#elif defined(PRR)
      PRR &= ~(1 << PRSPI);
#endif

      /* SPI interrupt enable, SPI enable, Master mode */
      // SPCR |= ((1 << SPE) | (1 << MSTR));
      SPCR |= ((1 << SPE) | (1 << SPIE));
      SPCR &= ~(1 << MSTR);

      switch (spip->config->role) {
      case SPI_ROLE_SLAVE:
        SPCR &= ~(1 << MSTR);
        DDR_SPI1 |=  (1 << SPI1_MISO);                                      /* output */
        DDR_SPI1 &= ~((1 << SPI1_MOSI) | (1 << SPI1_SCK) | (1 << SPI1_SS)); /* input  */
        break;
      case SPI_ROLE_MASTER: /* fallthrough */
      default:
        SPCR |= (1 << MSTR);
        DDR_SPI1 |=  ((1 << SPI1_MOSI) | (1 << SPI1_SCK)); /* output */
        DDR_SPI1 &= ~(1 << SPI1_MISO);                    /* input  */
        spip->config->ssport->dir |= (1 << spip->config->sspad);
      }

      switch (spip->config->bitorder) {
      case SPI_LSB_FIRST:
        SPCR |= (1 << DORD);
        break;
      case SPI_MSB_FIRST: /* fallthrough */
      default:
        SPCR &= ~(1 << DORD);
        break;
      }

      SPCR &= ~((1 << CPOL) | (1 << CPHA));
      switch (spip->config->mode) {
      case SPI_MODE_1:
        SPCR |= (1 << CPHA);
        break;
      case SPI_MODE_2:
        SPCR |= (1 << CPOL);
        break;
      case SPI_MODE_3:
        SPCR |= ((1 << CPOL) | (1 << CPHA));
        break;
      case SPI_MODE_0: /* fallthrough */
      default: break;
      }

      SPCR &= ~((1 << SPR1) | (1 << SPR0));
      SPSR &= ~(1 << SPI2X);
      switch (spip->config->clockrate) {
      case SPI_SCK_FOSC_2:
        SPSR |= (1 << SPI2X);
        break;
      case SPI_SCK_FOSC_8:
        SPSR |= (1 << SPI2X);
        SPCR |= (1 << SPR0);
        break;
      case SPI_SCK_FOSC_16:
        SPCR |= (1 << SPR0);
        break;
      case SPI_SCK_FOSC_32:
        SPSR |= (1 << SPI2X);
        SPCR |= (1 << SPR1);
        break;
      case SPI_SCK_FOSC_64:
        SPCR |= (1 << SPR1);
        break;
      case SPI_SCK_FOSC_128:
        SPCR |= ((1 << SPR1) | (1 << SPR0));
        break;
      case SPI_SCK_FOSC_4: /* fallthrough */
      default: break;
      }

      /* dummy reads before enabling interrupt */
      dummy = SPSR;
      dummy = SPDR;
      (void) dummy; /* suppress warning about unused variable */
      SPCR |= (1 << SPIE);
    }
#endif /* AVR_SPI_USE_SPI1 */
  }
}

/**
 * @brief   Deactivates the SPI peripheral.
 *
 * @param[in] spip      pointer to the @p SPIDriver object
 *
 * @notapi
 */
void spi_lld_stop(SPIDriver *spip) {

  if (spip->state == SPI_READY) {
    /* Resets the peripheral.*/

    /* Disables the peripheral.*/
#if AVR_SPI_USE_SPI1
    if (&SPID1 == spip) {
      SPCR &= ((1 << SPIE) | (1 << SPE));
      spip->config->ssport->dir &= ~(1 << spip->config->sspad);
    }
/* Disable SPI power using Power Reduction Register */
#if defined(PRR0)
      PRR0 |= (1 << PRSPI);
#elif defined(PRR)
      PRR |= (1 << PRSPI);
#endif
#endif /* AVR_SPI_USE_SPI1 */
  }
}

/**
 * @brief   Asserts the slave select signal and prepares for transfers.
 *
 * @param[in] spip      pointer to the @p SPIDriver object
 *
 * @notapi
 */
void spi_lld_select(SPIDriver *spip) {

  spip->config->ssport->out &= ~(1 << spip->config->sspad);

}

/**
 * @brief   Deasserts the slave select signal.
 * @details The previously selected peripheral is unselected.
 *
 * @param[in] spip      pointer to the @p SPIDriver object
 *
 * @notapi
 */
void spi_lld_unselect(SPIDriver *spip) {

  spip->config->ssport->out |= (1 << spip->config->sspad);

}

/**
 * @brief   Ignores data on the SPI bus.
 * @details This asynchronous function starts the transmission of a series of
 *          idle words on the SPI bus and ignores the received data.
 * @post    At the end of the operation the configured callback is invoked.
 *
 * @param[in] spip      pointer to the @p SPIDriver object
 * @param[in] n         number of words to be ignored
 *
 * @notapi
 */
void spi_lld_ignore(SPIDriver *spip, size_t n) {

  spip->rxbuf = spip->txbuf = NULL;
  spip->txbytes = n;
  spip->txidx = 0;

  SPDR = 0;
}

/**
 * @brief   Exchanges data on the SPI bus.
 * @details This asynchronous function starts a simultaneous transmit/receive
 *          operation.
 * @post    At the end of the operation the configured callback is invoked.
 * @note    The buffers are organized as uint8_t arrays for data sizes below or
 *          equal to 8 bits else it is organized as uint16_t arrays.
 *
 * @param[in] spip      pointer to the @p SPIDriver object
 * @param[in] n         number of words to be exchanged
 * @param[in] txbuf     the pointer to the transmit buffer
 * @param[out] rxbuf    the pointer to the receive buffer
 *
 * @notapi
 */
void spi_lld_exchange(SPIDriver *spip, size_t n,
                      const void *txbuf, void *rxbuf) {

  spip->rxbuf = rxbuf;
  spip->txbuf = txbuf;
  spip->txbytes = spip->rxbytes = n;
  spip->txidx = spip->rxidx = 0;

  SPDR = spip->txbuf[spip->txidx++];
}

/**
 * @brief   Sends data over the SPI bus.
 * @details This asynchronous function starts a transmit operation.
 * @post    At the end of the operation the configured callback is invoked.
 * @note    The buffers are organized as uint8_t arrays for data sizes below or
 *          equal to 8 bits else it is organized as uint16_t arrays.
 *
 * @param[in] spip      pointer to the @p SPIDriver object
 * @param[in] n         number of words to send
 * @param[in] txbuf     the pointer to the transmit buffer
 *
 * @notapi
 */
void spi_lld_send(SPIDriver *spip, size_t n, const void *txbuf) {

  spip->rxbuf = NULL;
  spip->txbuf = txbuf;
  spip->txbytes = n;
  spip->txidx = 0;

  SPDR = spip->txbuf[spip->txidx++];
}

/**
 * @brief   Receives data from the SPI bus.
 * @details This asynchronous function starts a receive operation.
 * @post    At the end of the operation the configured callback is invoked.
 * @note    The buffers are organized as uint8_t arrays for data sizes below or
 *          equal to 8 bits else it is organized as uint16_t arrays.
 *
 * @param[in] spip      pointer to the @p SPIDriver object
 * @param[in] n         number of words to receive
 * @param[out] rxbuf    the pointer to the receive buffer
 *
 * @notapi
 */
void spi_lld_receive(SPIDriver *spip, size_t n, void *rxbuf) {

  spip->txbuf = NULL;
  spip->rxbuf = rxbuf;
  spip->rxbytes = n;
  spip->rxidx = 0;

  /* Write dummy byte to start communication */
  SPDR = 0;
}

/**
 * @brief   Exchanges one frame using a polled wait.
 * @details This synchronous function exchanges one frame using a polled
 *          synchronization method. This function is useful when exchanging
 *          small amount of data on high speed channels, usually in this
 *          situation is much more efficient just wait for completion using
 *          polling than suspending the thread waiting for an interrupt.
 *
 * @param[in] spip      pointer to the @p SPIDriver object
 * @param[in] frame     the data frame to send over the SPI bus
 * @return              The received data frame from the SPI bus.
 */
#if 0
uint16_t spi_lld_polled_exchange(SPIDriver *spip, uint16_t frame) {

  uint16_t spdr = 0;
  uint8_t dummy;

  /* disable interrupt */
  SPCR &= ~(1 << SPIE);

  SPDR = frame >> 8;
  while (!(SPSR & (1 << SPIF))) ;
  spdr = SPDR << 8;

  SPDR = frame & 0xFF;
  while (!(SPSR & (1 << SPIF))) ;
  spdr |= SPDR;

  dummy = SPSR;
  dummy = SPDR;
  (void) dummy; /* suppress warning about unused variable */
  SPCR |= (1 << SPIE);

  return spdr;
}
#endif
uint8_t spi_lld_polled_exchange(SPIDriver *spip, uint8_t frame) {

  uint8_t spdr = 0;
  uint8_t dummy;

  /* disable interrupt */
  SPCR &= ~(1 << SPIE);

  SPDR = frame;
  while (!(SPSR & (1 << SPIF))) ;
  spdr = SPDR;

  dummy = SPSR;
  dummy = SPDR;
  (void) dummy; /* suppress warning about unused variable */
  SPCR |= (1 << SPIE);

  return spdr;
}

#endif /* HAL_USE_SPI */

/** @} */
