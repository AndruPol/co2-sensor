/*
 * cmds.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ch.h"
#include "hal.h"
#include "shell.h"
#include "chprintf.h"

#include "cmds.h"
#include "main.h"
#include "usbcfg.h"
#include "ssd1306.h"
#include "lcd.h"
#include "config.h"

#define	REBOOT	TRUE

void cmd_reboot(BaseSequentialStream *chp, int argc, char *argv[]);
void cmd_screen(BaseSequentialStream *chp, int argc, char *argv[]);
void cmd_buz(BaseSequentialStream *chp, int argc, char *argv[]);
void cmd_out(BaseSequentialStream *chp, int argc, char *argv[]);
void cmd_sysled(BaseSequentialStream *chp, int argc, char *argv[]);
void cmd_led(BaseSequentialStream *chp, int argc, char *argv[]);
void cmd_co2_en(BaseSequentialStream *chp, int argc, char *argv[]);
void cmd_temp_en(BaseSequentialStream *chp, int argc, char *argv[]);
void cmd_hum_en(BaseSequentialStream *chp, int argc, char *argv[]);
void cmd_co2(BaseSequentialStream *chp, int argc, char *argv[]);
void cmd_templow(BaseSequentialStream *chp, int argc, char *argv[]);
void cmd_temphigh(BaseSequentialStream *chp, int argc, char *argv[]);
void cmd_humlow(BaseSequentialStream *chp, int argc, char *argv[]);
void cmd_humhigh(BaseSequentialStream *chp, int argc, char *argv[]);
void cmd_buzint(BaseSequentialStream *chp, int argc, char *argv[]);
void cmd_outint(BaseSequentialStream *chp, int argc, char *argv[]);

const ShellCommand commands[] = {
#if REBOOT
  {"reboot", cmd_reboot},
#endif
  {"screen", cmd_screen},
  {"buz", cmd_buz},
  {"out", cmd_out},
  {"sysled", cmd_sysled},
  {"led", cmd_led},
  {"co2_en", cmd_co2_en},
  {"temp_en", cmd_temp_en},
  {"hum_en", cmd_hum_en},
  {"co2", cmd_co2},
  {"templow", cmd_templow},
  {"temphigh", cmd_temphigh},
  {"humlow", cmd_humlow},
  {"humhigh", cmd_humhigh},
  {"buzint", cmd_buzint},
  {"outint", cmd_outint},
  {NULL, NULL}
};

const ShellConfig cmd_cfg = {
  (BaseSequentialStream *) &SDU1,
  commands
};

/*
 * command functions start here
*/

/*
 * cmd screen | screen 0|1
*/
void cmd_screen(BaseSequentialStream *chp, int argc, char *argv[]) {
	(void)argv;
	(void)chp;

	if (argc == 0) {
		chprintf(chp, "Usage: screen [0|1]\r\n");
		chprintf(chp, "screen: %d\r\n", system_config.config.data.screen_en);
		return;
	}
	uint8_t p = atoi(argv[0]);
	if (p > 1) {
		chprintf(chp, "Usage: screen [0|1]\r\n");
		return;
	}
	system_config.config.data.screen_en = 0;
	if (p == 1) {
		system_config.config.data.screen_en = 1;
	}
	saveconfig();
	lcd_prepare();
}

/*
 * cmd buz | buz 0|1
*/
void cmd_buz(BaseSequentialStream *chp, int argc, char *argv[]) {
	(void)argv;
	(void)chp;

	if (argc == 0) {
		chprintf(chp, "Usage: buz [0|1]\r\n");
		chprintf(chp, "buz: %d\r\n", system_config.config.data.buzzer_en);
		return;
	}
	uint8_t p = atoi(argv[0]);
	if (p > 1) {
		chprintf(chp, "Usage: buz [0|1]\r\n");
		return;
	}
	system_config.config.data.buzzer_en = 0;
	if (p == 1)
		system_config.config.data.buzzer_en = 1;
	saveconfig();
}

/*
 * cmd out | out 0|1
*/
void cmd_out(BaseSequentialStream *chp, int argc, char *argv[]) {
	(void)argv;
	(void)chp;

	if (argc == 0) {
		chprintf(chp, "Usage: out [0|1]\r\n");
		chprintf(chp, "out: %d\r\n", system_config.config.data.output_en);
		return;
	}
	uint8_t p = atoi(argv[0]);
	if (p > 1) {
		chprintf(chp, "Usage: out [0|1]\r\n");
		return;
	}
	system_config.config.data.output_en = 0;
	if (p == 1)
		system_config.config.data.output_en = 1;
	saveconfig();
}

/*
 * cmd led | led 0|1
*/
void cmd_led(BaseSequentialStream *chp, int argc, char *argv[]) {
	(void)argv;
	(void)chp;

	if (argc == 0) {
		chprintf(chp, "Usage: led [0|1]\r\n");
		chprintf(chp, "led: %d\r\n", system_config.config.data.redled_en);
		return;
	}
	uint8_t p = atoi(argv[0]);
	if (p > 1) {
		chprintf(chp, "Usage: led [0|1]\r\n");
		return;
	}
	REDLED_OFF;
	system_config.config.data.redled_en = 0;
	if (p == 1)
		system_config.config.data.redled_en = 1;
	saveconfig();
}

/*
 * cmd sysled | sysled 0|1
*/
void cmd_sysled(BaseSequentialStream *chp, int argc, char *argv[]) {
	(void)argv;
	(void)chp;

	if (argc == 0) {
		chprintf(chp, "Usage: sysled [0|1]\r\n");
		chprintf(chp, "sysled: %d\r\n", system_config.config.data.sysled_en);
		return;
	}
	uint8_t p = atoi(argv[0]);
	if (p > 1) {
		chprintf(chp, "Usage: sysled [0|1]\r\n");
		return;
	}
	SYSLED_OFF;
	system_config.config.data.sysled_en = 0;
	if (p == 1)
		system_config.config.data.sysled_en = 1;
	saveconfig();
}

/*
 * cmd co2_en | co2_en 0|1
*/
void cmd_co2_en(BaseSequentialStream *chp, int argc, char *argv[]) {
	(void)argv;
	(void)chp;

	if (argc == 0) {
		chprintf(chp, "Usage: co2_en [0|1]\r\n");
		chprintf(chp, "co2_en: %d\r\n", system_config.config.data.co2_en);
		return;
	}
	uint8_t p = atoi(argv[0]);
	if (p > 1) {
		chprintf(chp, "Usage: co2_en [0|1]\r\n");
		return;
	}
	system_config.config.data.co2_en = 0;
	if (p == 1)
		system_config.config.data.co2_en = 1;
	saveconfig();
}

/*
 * cmd temp_en | temp_en 0|1
*/
void cmd_temp_en(BaseSequentialStream *chp, int argc, char *argv[]) {
	(void)argv;
	(void)chp;

	if (argc == 0) {
		chprintf(chp, "Usage: temp_en [0|1]\r\n");
		chprintf(chp, "tem_en: %d\r\n", system_config.config.data.dht_temp_en);
		return;
	}
	uint8_t p = atoi(argv[0]);
	if (p > 1) {
		chprintf(chp, "Usage: temp_en [0|1]\r\n");
		return;
	}
	system_config.config.data.dht_temp_en = 0;
	if (p == 1)
		system_config.config.data.dht_temp_en = 1;
	saveconfig();
}

/*
 * cmd hum_en | hum_en 0|1
*/
void cmd_hum_en(BaseSequentialStream *chp, int argc, char *argv[]) {
	(void)argv;
	(void)chp;

	if (argc == 0) {
		chprintf(chp, "Usage: hum_en [0|1]\r\n");
		chprintf(chp, "hum_en: %d\r\n", system_config.config.data.dht_hum_en);
		return;
	}
	uint8_t p = atoi(argv[0]);
	if (p > 1) {
		chprintf(chp, "Usage: hum_en [0|1]\r\n");
		return;
	}
	system_config.config.data.dht_hum_en = 0;
	if (p == 1)
		system_config.config.data.dht_hum_en = 1;
	saveconfig();
}

/*
 * cmd co2 | co2 warn crit
*/
void cmd_co2(BaseSequentialStream *chp, int argc, char *argv[]) {
	(void)argv;
	(void)chp;

	if (!(argc == 0 || argc == 2)) {
		chprintf(chp, "Usage: co2 [warn crit]\r\n");
		return;
	}
	if (argc == 0) {
		chprintf(chp, "co2: %4d %4d\r\n", system_config.co2warn, system_config.co2crit);
		return;
	}
	if (argc == 2) {
		uint16_t w = atoi(argv[0]);
		uint16_t c = atoi(argv[1]);
		system_config.co2warn = w;
		system_config.co2crit = c;
		saveconfig();
	}
}

/*
 * cmd templow | templow warn crit
*/
void cmd_templow(BaseSequentialStream *chp, int argc, char *argv[]) {
	(void)argv;
	(void)chp;

	if (!(argc == 0 || argc == 2)) {
		chprintf(chp, "Usage: templow [warn crit]\r\n");
		return;
	}
	if (argc == 0) {
		chprintf(chp, "templow: %2d %2d\r\n", system_config.templow.data.warn, system_config.templow.data.crit);
		return;
	}
	if (argc == 2) {
		uint16_t w = atoi(argv[0]);
		uint16_t c = atoi(argv[1]);
		system_config.templow.data.warn = w;
		system_config.templow.data.crit = c;
		saveconfig();
	}
}

/*
 * cmd temphigh | temphigh warn crit
*/
void cmd_temphigh(BaseSequentialStream *chp, int argc, char *argv[]) {
	(void)argv;
	(void)chp;

	if (!(argc == 0 || argc == 2)) {
		chprintf(chp, "Usage: temphigh [warn crit]\r\n");
		return;
	}
	if (argc == 0) {
		chprintf(chp, "temphigh: %2d %2d\r\n", system_config.temphigh.data.warn, system_config.temphigh.data.crit);
		return;
	}
	if (argc == 2) {
		uint16_t w = atoi(argv[0]);
		uint16_t c = atoi(argv[1]);
		system_config.temphigh.data.warn = w;
		system_config.temphigh.data.crit = c;
		saveconfig();
	}
}

/*
 * cmd humlow | humlow warn crit
*/
void cmd_humlow(BaseSequentialStream *chp, int argc, char *argv[]) {
	(void)argv;
	(void)chp;

	if (!(argc == 0 || argc == 2)) {
		chprintf(chp, "Usage: humlow [warn crit]\r\n");
		return;
	}
	if (argc == 0) {
		chprintf(chp, "humlow: %2d %2d\r\n", system_config.humlow.data.warn, system_config.humlow.data.crit);
		return;
	}
	if (argc == 2) {
		uint16_t w = atoi(argv[0]);
		uint16_t c = atoi(argv[1]);
		system_config.humlow.data.warn = w;
		system_config.humlow.data.crit = c;
		saveconfig();
	}
}

/*
 * cmd humhigh | humhigh warn crit
*/
void cmd_humhigh(BaseSequentialStream *chp, int argc, char *argv[]) {
	(void)argv;
	(void)chp;

	if (!(argc == 0 || argc == 2)) {
		chprintf(chp, "Usage: humhigh [warn crit]\r\n");
		return;
	}
	if (argc == 0) {
		chprintf(chp, "humhigh: %2d %2d\r\n", system_config.humhigh.data.warn, system_config.humhigh.data.crit);
		return;
	}
	if (argc == 2) {
		uint16_t w = atoi(argv[0]);
		uint16_t c = atoi(argv[1]);
		system_config.humhigh.data.warn = w;
		system_config.humhigh.data.crit = c;
		saveconfig();
	}
}

/*
 * cmd buzint | buzint N
*/
void cmd_buzint(BaseSequentialStream *chp, int argc, char *argv[]) {
	(void)argv;
	(void)chp;

	if (argc == 0) {
		chprintf(chp, "Usage: buzint N (N=1..255)\r\n");
		chprintf(chp, "buzint: %d\r\n", system_config.timeint.data.buzint);
		return;
	}
	uint8_t p = atoi(argv[0]);
	if (p == 0) {
		chprintf(chp, "Usage: buzint N (N=1..255)\r\n");
		return;
	}
	system_config.timeint.data.buzint = p;
	saveconfig();
	buzint = 1;
}

/*
 * cmd outint | outint N
*/
void cmd_outint(BaseSequentialStream *chp, int argc, char *argv[]) {
	(void)argv;
	(void)chp;

	if (argc == 0) {
		chprintf(chp, "Usage: outint N (N=5..255)\r\n");
		chprintf(chp, "outint: %d\r\n", system_config.timeint.data.outint);
		return;
	}
	uint8_t p = atoi(argv[0]);
	if (p < 5) {
		chprintf(chp, "Usage: outint N (N=5..255)\r\n");
		return;
	}
	system_config.timeint.data.outint = p;
	saveconfig();
	outint = 1;
}

/*
 * cmd reboot
*/
void cmd_reboot(BaseSequentialStream *chp, int argc, char *argv[]){
  (void)argv;
  (void)chp;

  if (argc > 0) {
    chprintf(chp, "Usage: reboot\r\n");
    return;
  }

  chThdSleepMilliseconds(100);
  NVIC_SystemReset();
}
