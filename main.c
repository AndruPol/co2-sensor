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
static THD_WORKING_AREA(waLEDThread, 196);

#define SCANINT			MS2ST(5000)
#define SCANPRIO		(NORMALPRIO)
static THD_WORKING_AREA(waScanThread, 512);

#define USB_WRITE_MS	100

volatile backup_data_t *backup_data = (volatile backup_data_t *)(BKP_BASE + 0x0004);
system_config_t system_config;
dht_sensor_t dht;
mhz19_sensor_t mhz19;
delay_t mhz19_pwrup = {
	.active = true,
	.delay = MHZ19_PWRON
};

static state_t led = UNKNOWN;
state_t co2_state = UNKNOWN;
state_t temp_state = UNKNOWN;
state_t hum_state = UNKNOWN;

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
	STM32_IWDG_RL(2000),
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
		co2_state = UNKNOWN;
		mhz19.co2 = 0;
		if (system_config.config.data.co2_en == 1 && !mhz19_pwrup.active) {
			mhz19.state = mhz19_read(&mhz19.co2);
			if (mhz19.state == MHZ19_OK) {
				if (mhz19.co2 > system_config.co2warn)
					co2_state = WARNING;
				if (mhz19.co2 > system_config.co2crit)
					co2_state = CRITICAL;
				if (co2_state == UNKNOWN)
					co2_state = OK;
			}
		}
		temp_state = UNKNOWN;
		hum_state = UNKNOWN;
		dht.temperature = 0;
		dht.humidity = 0;
		if (system_config.config.data.dht_temp_en == 1 || system_config.config.data.dht_hum_en == 1) {
			dht.state = dht_read(&dht.temperature, &dht.humidity);
			if (dht.state == DHT_OK && system_config.config.data.dht_temp_en == 1) {
				if (dht.temperature < system_config.templow.data.warn * 10 || dht.temperature > system_config.temphigh.data.warn * 10)
					temp_state = WARNING;
				if (dht.temperature < system_config.templow.data.crit * 10 || dht.temperature > system_config.temphigh.data.crit * 10)
					temp_state = CRITICAL;
				if (temp_state == UNKNOWN)
					temp_state = OK;
			}
			if (dht.state == DHT_OK && system_config.config.data.dht_hum_en == 1) {
				if (dht.humidity < system_config.humlow.data.warn * 10 || dht.humidity > system_config.humhigh.data.warn * 10)
					hum_state = WARNING;
				if (dht.humidity < system_config.humlow.data.crit * 10 || dht.humidity > system_config.humhigh.data.crit * 10)
					hum_state = CRITICAL;
				if (hum_state == UNKNOWN)
					hum_state = OK;
			}
		}
		lcd_co2(mhz19.co2);
		lcd_temperature(dht.temperature);
		lcd_humidity(dht.humidity);
		if (system_config.config.data.output_en == 1 && SDU1.config->usbp->state == USB_ACTIVE) {
			char stemp[5] = "0.0", shum[5] = "0.0";
			if (system_config.config.data.dht_temp_en == 1)
				sprintf(stemp, "%2.1f", (float) dht.temperature / 10);
			if (system_config.config.data.dht_hum_en == 1)
				sprintf(shum, "%2.1f", (float) dht.humidity / 10);
			char buf[24];
			sprintf(buf, "#%d;%d;%d;%s;%d;%s\r\n",
					co2_state, system_config.config.data.co2_en == 1 ? mhz19.co2 : 0,
					temp_state, stemp,
					hum_state, shum
					);
			chnWriteTimeout(&SDU1, (void *)buf, strlen(buf), MS2ST(USB_WRITE_MS));
		}
		if (system_config.config.data.redled_en == 1) {
			if (co2_state == CRITICAL || temp_state == CRITICAL || hum_state == CRITICAL)
				led = CRITICAL;
			else if (co2_state == WARNING || temp_state == WARNING || hum_state == WARNING)
				led = WARNING;
			else
				led = OK;
		}
		if (system_config.config.data.buzzer_en == 1) {
			if (co2_state == CRITICAL || temp_state == CRITICAL || hum_state == CRITICAL)
				buzzer_on(NOTE_ALT, true);
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

    lcd_update();
    color_critical = !color_critical;

    chThdSleepUntil(time);
  }
}
