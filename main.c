/*
    ChibiOS/RT - Copyright (C) 2006-2013 Giovanni Di Sirio

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#include <string.h>
#include <stdlib.h>

#include "ch.h"
#include "hal.h"
#include "chprintf.h"
#include "usbcfg.h"

#include "main.h"
#include "config.h"
#include "buzzer.h"
#include "dht.h"
#include "mhz19.h"
#include "cmds.h"
#include "lcd.h"
#include "util/printfs.h"

#define LEDINT			MS2ST(500)
#define LEDPRIO			(NORMALPRIO)
static THD_WORKING_AREA(waLEDThread, 164);

#define SCANINT			MS2ST(5000)
#define SCANPRIO		(NORMALPRIO)
static THD_WORKING_AREA(waScanThread, 256);

#define USB_WRITE_MS	100

static binary_semaphore_t updsem;
volatile backup_data_t *backup_data = (volatile backup_data_t *)(BKP_BASE + 0x0004);
system_config_t system_config;
delay_t mhz19_pwrup = {
	.active = true,
	.delay = MHZ19_PWRON
};

static state_t led = UNKNOWN;
dht_sensor_t dht;
mhz19_sensor_t mhz19;

uint8_t outint = 1;
uint8_t buzint = 1;

#define SHELL_WA_SIZE   THD_WORKING_AREA_SIZE(2048)
static const ShellConfig shell_cfg = {
  (BaseSequentialStream *)&SDU1,
  commands
};

/*===========================================================================*/
/* IWDG related.                                                             */
/*===========================================================================*/
#if USE_IWDG
static const WDGConfig wdgcfg = {
	STM32_IWDG_PR_64,
	STM32_IWDG_RL(3000),
};
#endif

/*
 *  LED thread
*/
static THD_FUNCTION(LEDThread, arg) {
	(void)arg;
	palSetPadMode(GPIOA, GPIOA_REDLED, PAL_MODE_OUTPUT_PUSHPULL);
	REDLED_OFF;
	chRegSetThreadName("LEDThd");
	while (TRUE) {
		systime_t time = chVTGetSystemTimeX();
		time += LEDINT;
		if (system_config.config.data.redled_en == 0) {
			REDLED_OFF;
			goto next;
		}
		if (led == WARNING) {
			REDLED_ON;
		} else if (led == CRITICAL) {
			REDLED_TOGGLE;
		} else {
			REDLED_OFF;
		}
next:
		chThdSleepUntil(time);
	}
}

/*
 *  sensors polling thread
*/
static THD_FUNCTION(ScanThread, arg) {
	(void)arg;

	chRegSetThreadName("ScanThd");
	while (TRUE) {
		systime_t time = chVTGetSystemTimeX();
		time += SCANINT;
		// read sensors
		if (system_config.config.data.co2_en == 1 && !mhz19_pwrup.active) {
			uint16_t co2;
			mhz19.error = mhz19_read(&co2);
			chBSemWait(&updsem);
			mhz19.state = UNKNOWN;
			mhz19.co2 = 0;
			if (mhz19.error == MHZ19_OK) {
				mhz19.co2 = co2;
				if (mhz19.co2 > system_config.co2warn)
					mhz19.state = WARNING;
				if (mhz19.co2 > system_config.co2crit)
					mhz19.state = CRITICAL;
				if (mhz19.state == UNKNOWN)
					mhz19.state = OK;
			}
			chBSemSignal(&updsem);
		}

		if (system_config.config.data.dht_temp_en == 1 || system_config.config.data.dht_hum_en == 1) {
			int16_t temperature;
			uint16_t humidity;
			dht.error = dht_read(&temperature, &humidity);
			chBSemWait(&updsem);
			dht.temp_state = UNKNOWN;
			dht.temperature = 0;
			if (dht.error == DHT_OK && system_config.config.data.dht_temp_en == 1) {
				dht.temperature = temperature;
				if (dht.temperature < system_config.templow.data.warn * 10 || dht.temperature > system_config.temphigh.data.warn * 10)
					dht.temp_state = WARNING;
				if (dht.temperature < system_config.templow.data.crit * 10 || dht.temperature > system_config.temphigh.data.crit * 10)
					dht.temp_state = CRITICAL;
				if (dht.temp_state == UNKNOWN)
					dht.temp_state = OK;
			}
			dht.hum_state = UNKNOWN;
			dht.humidity = 0;
			if (dht.error == DHT_OK && system_config.config.data.dht_hum_en == 1) {
				dht.humidity = humidity;
				if (dht.humidity < system_config.humlow.data.warn * 10 || dht.humidity > system_config.humhigh.data.warn * 10)
					dht.hum_state = WARNING;
				if (dht.humidity < system_config.humlow.data.crit * 10 || dht.humidity > system_config.humhigh.data.crit * 10)
					dht.hum_state = CRITICAL;
				if (dht.hum_state == UNKNOWN)
					dht.hum_state = OK;
			}
			chBSemSignal(&updsem);
		}

		if (system_config.config.data.redled_en == 1) {
			if (mhz19.state == CRITICAL || dht.temp_state == CRITICAL || dht.hum_state == CRITICAL)
				led = CRITICAL;
			else if (mhz19.state == WARNING || dht.temp_state == WARNING || dht.hum_state == WARNING)
				led = WARNING;
			else
				led = OK;
		}
		chThdSleepUntil(time);
	} //while
}


/*===========================================================================*/
/* Main and generic code.                                                    */
/*===========================================================================*/

// called on kernel panic
void halt(void){
	port_disable();
	SYSLED_ON;			// system LED on
	while(TRUE)
	{
	}
}

// return usb state
bool usb_active(void) {
	return SDU1.config->usbp->state == USB_ACTIVE;
}

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
  thread_t *shelltp = NULL;

  halInit();
  chSysInit();

  palSetPadMode(GPIOA, GPIOA_SYSLED, PAL_MODE_OUTPUT_PUSHPULL);
  SYSLED_OFF;

  // read or set default config in backup regs
  if (!readconfig()) {
	  defaultconfig();
	  saveconfig();
  }

  // PA15 used by TIM2 CH1
  AFIO->MAPR |= AFIO_MAPR_SWJ_CFG_JTAGDISABLE;

  chBSemObjectInit(&updsem, FALSE);

  buzzer_init();
  dht_init();
  mhz19_init();
  lcd_init();

  sduObjectInit(&SDU1);
  sduStart(&SDU1, &serusbcfg);

  /*
   * Activates the USB driver and then the USB bus pull-up on D+.
   * Note, a delay is inserted in order to not have to disconnect the cable
   * after a reset.
   */
  usbDisconnectBus(serusbcfg.usbp);
  chThdSleepMilliseconds(1500);
  usbStart(serusbcfg.usbp, &usbcfg);
  usbConnectBus(serusbcfg.usbp);

  lcd_prepare();

  mhz19.state = UNKNOWN;
  dht.temp_state = UNKNOWN;
  dht.hum_state = UNKNOWN;

  // Creates LED thread.
  chThdCreateStatic(waLEDThread, sizeof(waLEDThread), LEDPRIO, LEDThread, NULL);
  // Creates sensor polling thread.
  chThdCreateStatic(waScanThread, sizeof(waScanThread), SCANPRIO, ScanThread, NULL);

  shellInit();

  // IWDG start
#if USE_IWDG
  wdgStart(&WDGD1, &wdgcfg);
#endif

  /*
   * Normal main() thread activity, in this demo it does nothing except
   * sleeping in a loop and listen for events.
  */
  while (TRUE) {
	systime_t time = chVTGetSystemTimeX();
	time += MS2ST(1000);

	if (system_config.config.data.sysled_en == 1)
		SYSLED_TOGGLE;

	// reset watchdog
#if USE_IWDG
	wdgReset(&WDGD1);
#endif

    if (!shelltp && SDU1.config->usbp->state == USB_ACTIVE)
      shelltp = shellCreate(&shell_cfg, SHELL_WA_SIZE, NORMALPRIO);
    else if (chThdTerminatedX(shelltp)) {
      chThdRelease(shelltp);    /* Recovers memory of the previous shell.   */
      shelltp = NULL;           /* Triggers spawning of a new shell.        */
    }

	// MH-Z19 power up delay
	if (mhz19_pwrup.active && --mhz19_pwrup.delay == 0)
		mhz19_pwrup.active = false;

	if (system_config.config.data.buzzer_en == 1) {
		chBSemWait(&updsem);
		if (mhz19.state == CRITICAL || dht.temp_state == CRITICAL || dht.hum_state == CRITICAL) {
			if (--buzint == 0) {
				buzzer_on(NOTE_ALT, true);
				buzint = system_config.timeint.data.buzint;
			}
		}
		chBSemSignal(&updsem);
	}

	if (system_config.config.data.output_en == 1 && SDU1.config->usbp->state == USB_ACTIVE) {
		chBSemWait(&updsem);
		if (--outint == 0) {
			char stemp[5] = "0.0", shum[5] = "0.0";
			if (system_config.config.data.dht_temp_en == 1)
				sprintf(stemp, "%2.1f", (float) dht.temperature / 10);
			if (system_config.config.data.dht_hum_en == 1)
				sprintf(shum, "%2.1f", (float) dht.humidity / 10);
			char buf[24];
			sprintf(buf, "#%d;%d;%d;%s;%d;%s\r\n",
					mhz19.state, system_config.config.data.co2_en == 1 ? mhz19.co2 : 0,
					dht.temp_state, stemp,
					dht.hum_state, shum
					);
			chnWriteTimeout(&SDU1, (void *)buf, strlen(buf), MS2ST(USB_WRITE_MS));
			outint = system_config.timeint.data.outint;
		}
		chBSemSignal(&updsem);
	}

	chBSemWait(&updsem);
    lcd_update();
	chBSemSignal(&updsem);

    chThdSleepUntil(time);
  }
}
