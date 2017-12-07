/*
 * config.c
 *
 *  Created on: 02 нояб. 2017 г.
 *      Author: andru
 */

#include "ch.h"
#include "hal.h"

#include "main.h"

void defaultconfig(void) {
	config_t cfg = {
		.co2_en = 1,
		.dht_temp_en = 1,
		.dht_hum_en = 1,
		.screen_en = 1,
		.output_en = 1,
		.buzzer_en = 1,
		.sysled_en = 1,
		.redled_en = 1,
	};

	system_config.config.data = cfg;
	system_config.co2warn = CO2_WARN;
	system_config.co2crit = CO2_CRIT;
	system_config.templow.data.warn = TEMPLOW_WARN;
	system_config.templow.data.crit = TEMPLOW_CRIT;
	system_config.temphigh.data.warn = TEMPHIGH_WARN;
	system_config.temphigh.data.crit = TEMPHIGH_CRIT;
	system_config.humlow.data.warn = HUMLOW_WARN;
	system_config.humlow.data.crit = HUMLOW_CRIT;
	system_config.humhigh.data.warn = HUMHIGH_WARN;
	system_config.humhigh.data.crit = HUMHIGH_CRIT;
}

bool readconfig(void) {
    if (*(&backup_data->magic.data) != MAGIC) {
    	return false;
    }

    system_config.config.raw = *(&backup_data->config.data);
    system_config.co2warn = *(&backup_data->co2warn.data);
    system_config.co2crit = *(&backup_data->co2crit.data);
    system_config.templow.raw = *(&backup_data->templow.data);
    system_config.temphigh.raw = *(&backup_data->temphigh.data);
    system_config.humlow.raw = *(&backup_data->humlow.data);
    system_config.humhigh.raw = *(&backup_data->humhigh.data);
    return true;
}

void saveconfig(void) {
    *(&backup_data->magic.data) =  MAGIC;
    *(&backup_data->config.data) =  system_config.config.raw;
    *(&backup_data->co2warn.data) = system_config.co2warn;
    *(&backup_data->co2crit.data) = system_config.co2crit;
    *(&backup_data->templow.data) = system_config.templow.raw;
    *(&backup_data->temphigh.data) = system_config.temphigh.raw;
    *(&backup_data->humlow.data) = system_config.humlow.raw;
    *(&backup_data->humhigh.data) = system_config.humhigh.raw;
}
