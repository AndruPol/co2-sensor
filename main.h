/*
 * main.h
 *
 *  Created on: 16.10.2017
 *      Author: andru
 */

#ifndef MAIN_H_
#define MAIN_H_

#define FIRMWARE		"1.0"	// версия прошивки
#define MAGIC			0xAE68	// magic word

#define CO2_WARN		1000	// CO2 warning
#define CO2_CRIT		1500	// CO2 critical
#define TEMPLOW_WARN	18		// temperature low warning
#define TEMPLOW_CRIT	15		// temperature low critical
#define TEMPHIGH_WARN	27		// temperature high warning
#define TEMPHIGH_CRIT	30		// temperature high critical
#define HUMLOW_WARN		35		// humidity low warning
#define HUMLOW_CRIT		25		// humidity low critical
#define HUMHIGH_WARN	70		// humidity high warning
#define HUMHIGH_CRIT	80		// humidity high critical

#define USE_IWDG		TRUE	// WDG use

#define SYSLED_ON		palSetPad(GPIOA, GPIOA_SYSLED)
#define SYSLED_OFF		palClearPad(GPIOA, GPIOA_SYSLED)
#define SYSLED_TOGGLE	palTogglePad(GPIOA, GPIOA_SYSLED)

#define REDLED_ON		palSetPad(GPIOA, GPIOA_REDLED)
#define REDLED_OFF		palClearPad(GPIOA, GPIOA_REDLED)
#define REDLED_TOGGLE	palTogglePad(GPIOA, GPIOA_REDLED)

#include "dht.h"
#include "mhz19.h"

typedef enum {
	OK,
	WARNING,
	CRITICAL,
	UNKNOWN,
} state_t;

typedef struct _config_t config_t;
struct _config_t {
	uint16_t co2_en : 1;		// check CO2 sensor
	uint16_t dht_temp_en : 1;	// check DHT sensor temperature
	uint16_t dht_hum_en : 1;	// check DHT sensor humidity
	uint16_t screen_en : 1;		// enable OLED screen
	uint16_t output_en : 1;		// enable output over serial
	uint16_t buzzer_en : 1;		// enable buzzer alarm
	uint16_t sysled_en : 1;		// enable system led
	uint16_t redled_en : 1;		// enable alarm led
};

typedef union _sysconfig sysconfig_t;
union _sysconfig {
	uint16_t raw;
	config_t data;
};

typedef struct _limit_t limit_t;
struct _limit_t {
	int8_t warn;	// warning setup
	int8_t crit;	// critical setup
};

typedef union _dht_limit dht_limit_t;
union _dht_limit {
	uint16_t raw;
	limit_t data;
};

typedef struct _system_config system_config_t;
struct _system_config {
	sysconfig_t config;	// configuration
	uint16_t co2warn;		// CO2 warning
	uint16_t co2crit;		// CO2 critical
	dht_limit_t templow;	// temperature low limit
	dht_limit_t temphigh;	// temperature high limit
	dht_limit_t humlow;		// humidity low limit
	dht_limit_t humhigh;	// humidity high limit
};

typedef struct _backup_reg backup_reg_t;
struct _backup_reg {
    uint16_t data;			// STM32F103 16bit
    uint16_t na;
};

typedef struct _backup_data backup_data_t;
struct _backup_data {
	backup_reg_t magic; 	// magic word
	backup_reg_t config; 	// system config
	backup_reg_t co2warn; 	// CO2 warning
	backup_reg_t co2crit; 	// CO2 critical
	backup_reg_t templow; 	// temperature low limit
	backup_reg_t temphigh; 	// temperature high limit
	backup_reg_t humlow; 	// humidity low limit
	backup_reg_t humhigh; 	// humidity high limit
};

typedef struct _delay delay_t;
struct _delay {
    bool active;
    uint8_t delay;
};

extern system_config_t system_config;
extern volatile backup_data_t *backup_data;

extern state_t co2_state;
extern state_t temp_state;
extern state_t hum_state;
extern delay_t mhz19_pwrup;

bool usb_active(void);

#ifdef __cplusplus
}
#endif

#endif /* MAIN_H_ */
