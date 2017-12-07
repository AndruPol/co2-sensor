/*
 * buzzer.h
 *
 *  Created on: 20.11.2017
 *      Author: andru
 */

#ifndef BUZZER_H_
#define BUZZER_H_

#define NOTE_ALT 	600	// ~600Hz

/* C++ detection */
#ifdef __cplusplus
extern C {
#endif

void buzzer_init(void);
void buzzer_on(uint16_t _period, bool _once);
void buzzer_off(void);

/* C++ detection */
#ifdef __cplusplus
}
#endif

#endif /* BUZZER_H_ */
