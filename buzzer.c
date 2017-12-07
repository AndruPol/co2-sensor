/*
 * buzzer.c
 *
 *  Created on: 20.11.2017
 *      Author: andru
 */

#define PWM_DRV         PWMD2
#define PWM_CH          0
#define PWM_GPIO        GPIOA
#define PWM_PIN         GPIOA_BUZZER

#include "ch.h"
#include "hal.h"

#define BUZZERINT		MS2ST(500)
#define BUZZERPRIO		(NORMALPRIO+1)
static THD_WORKING_AREA(waBuzzerThread, 196);

static thread_t *BuzzerThread_p = NULL;
static uint16_t period;
static bool once;

/*===========================================================================*/
/* PWM related.                                                                  */
/*===========================================================================*/
static const PWMConfig pwmcfg = {
  1000000,                                  /* 1MHz PWM clock frequency. */
  100,                                      /* Initial PWM period 1ms.   */
  NULL,
  {
   {PWM_OUTPUT_ACTIVE_HIGH, NULL},
   {PWM_OUTPUT_DISABLED, NULL},
   {PWM_OUTPUT_DISABLED, NULL},
   {PWM_OUTPUT_DISABLED, NULL}
  },
  0,
  0
};

void pwm_on(void) {
    pwmChangePeriod(&PWM_DRV, period);
	pwmEnableChannel(&PWM_DRV, PWM_CH, PWM_PERCENTAGE_TO_WIDTH(&PWM_DRV, 5000));
}

void pwm_off(void) {
	pwmDisableChannel(&PWM_DRV, PWM_CH);
}

/*
 *  buzzer thread
*/
static THD_FUNCTION(BuzzerThread, arg) {
	(void)arg;
	chRegSetThreadName("BuzzerThd");
	while (!chThdShouldTerminateX()) {
		systime_t time = chVTGetSystemTimeX();
		time += BUZZERINT;
		pwm_on();
		chThdSleepUntil(time);
		pwm_off();
		if (once) break;
		time += BUZZERINT;
		chThdSleepUntil(time);
	}
	BuzzerThread_p = NULL;
	chThdExit((msg_t) 0);
}

void buzzer_on(uint16_t _period, bool _once) {
	period = _period;
	once = _once;
    if (!BuzzerThread_p)
    	BuzzerThread_p = chThdCreateStatic(waBuzzerThread, sizeof(waBuzzerThread), BUZZERPRIO, BuzzerThread, NULL);
}


void buzzer_off(void) {
	if (BuzzerThread_p) {
		chThdTerminate(BuzzerThread_p);
		chThdWait(BuzzerThread_p);
		BuzzerThread_p = NULL;
	}
}

/* PWM init */
void buzzer_init(void) {
	pwmStart(&PWM_DRV, &pwmcfg);
	AFIO->MAPR |= AFIO_MAPR_TIM2_REMAP;
	palSetPadMode(PWM_GPIO, PWM_PIN, PAL_MODE_STM32_ALTERNATE_PUSHPULL);
}
