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
#include "chprintf.h"



/*
 * Application entry point.
 */
int main(void) {

  /*
   * System initializations.
   * - HAL initialization, this also initializes the configured device drivers
   *   and performs the board-specific initializations.
   * - Kernel initialization, the main() function becomes a thread and the
   *   RTOS is active.
   */
  halInit();
  chSysInit();

  /*
   * Activates the serial driver 1 using the driver default configuration.
   */
  
  palSetPadMode(IOPORT2, 0, PAL_MODE_OUTPUT_PUSHPULL);
  
  sdStart(&SD1, NULL);

  #define ADC_GRP1_NUM_CHANNELS   3
  #define ADC_GRP1_BUF_DEPTH      10
    DDRD |= _BV(DDD5);  
    
  static adcsample_t samples[ADC_GRP1_NUM_CHANNELS * ADC_GRP1_BUF_DEPTH];
  static const ADCConversionGroup adcgrpcfg = {
    FALSE,
    ADC_GRP1_NUM_CHANNELS,
    NULL,
    0b00000111                        /*   enabled channels                */
  };

  ADCConfig adccfg = {
    ANALOG_REFERENCE_AVCC /* use the AVCC pin as the ADC reference voltage */
  };

  adcStart(&ADCD1, &adccfg);
  
  palSetGroupMode(IOPORTADC, 0b00000111, 0, PAL_MODE_INPUT_PULLUP);
  

  while(1)
  {
	int i;
    adcConvert(&ADCD1, &adcgrpcfg, samples, ADC_GRP1_BUF_DEPTH);
      
    chprintf((BaseSequentialStream *) &SD1,"Result:\r\n");
    for(i=0; i < (ADC_GRP1_BUF_DEPTH * ADC_GRP1_NUM_CHANNELS); i++) {
      if((i % ADC_GRP1_NUM_CHANNELS) == 0) chprintf((BaseSequentialStream *) &SD1,"\r\n");
      chprintf((BaseSequentialStream *) &SD1," %x", samples[i]);
    }
     
    chprintf((BaseSequentialStream *) &SD1,"\r\n");
    chThdSleepMilliseconds(500);
	palTogglePad(IOPORT2, 0);
  }
}
