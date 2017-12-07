/*
 * config.h
 *
 *  Created on: 02 нояб. 2017 г.
 *      Author: andru
 */

#ifndef CONFIG_H_
#define CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif

void defaultconfig(void);
bool readconfig(void);
bool saveconfig(void);

#ifdef __cplusplus
}
#endif

#endif /* CONFIG_H_ */
