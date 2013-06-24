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
*/

#include "ch.h"
#include "hal.h"
#include <util/delay.h>
#include "chprintf.h"

// #define SPI_ROLE SPI_ROLE_SLAVE
#define SPI_ROLE SPI_ROLE_MASTER
#if (SPI_ROLE == SPI_ROLE_MASTER)
	#define MSG 'M'
#else
	#define MSG 'S'
#endif

void spicallback(SPIDriver *spip){
  chprintf((BaseSequentialStream *) &SD1,"CB\r\n");
}

/*
 * Application entry point.
 */
int main(void) {

/*
 * SPI interface configuration
 */
  static SPIConfig spicfg = {
    SPI_ROLE,
    IOPORT2,
    SPI1_SS,
    SPI_MODE_0,
    SPI_MSB_FIRST,
    SPI_SCK_FOSC_128,
    spicallback
  };
 
  /*
   * System initializations.
   * - HAL initialization, this also initializes the configured device drivers
   *   and performs the board-specific initializations.
   * - Kernel initialization, the main() function becomes a thread and the
   *   RTOS is active.
   */

  halInit();
  chSysInit();
  palSetPad(IOPORTSPI1, SPI1_SS);
  palClearPad(IOPORTSPI1, SPI1_SS);

  palSetGroupMode(IOPORTSPI1, 0x3f, 0, PAL_MODE_INPUT_PULLUP);
  palSetPadMode(IOPORTSPI1, SPI1_MISO, PAL_MODE_OUTPUT_PUSHPULL);

  spiStart(&SPID1, &spicfg);
  sdStart(&SD1, NULL);

  chprintf((BaseSequentialStream *) &SD1, "Start\r\n");

  while(1){
      uint8_t temp;
#if (SPI_ROLE == SPI_ROLE_MASTER)
      spi_lld_select(&SPID1);
#endif
      temp = spiPolledExchange(&SPID1, MSG);
      chprintf(&SD1,"temp1: %x SPCR: %x, SPSR: %x, SPDR: %c, PORTB %x, DDRB: %x\r\n",temp,SPCR,SPSR,SPDR,PORTB, DDRB);

#if (SPI_ROLE == SPI_ROLE_MASTER)
      spi_lld_unselect(&SPID1);
      chprintf((BaseSequentialStream *) &SD1, "Master\r\n");
#else
      chprintf((BaseSequentialStream *) &SD1, "Slave\r\n");
#endif
      chThdSleepMilliseconds(500);
  }
}
