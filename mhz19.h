/*
 * mhz19.h - MH-Z19 CO2 sensor driver
 *
 *  Created on: 03.11.2017
 *      Author: andru
 */

#ifndef MHZ19_H_
#define MHZ19_H_

#define MHZ19_PWRON		120

typedef enum {
        MHZ19_OK,
		MHZ19_IRQ_TIMEOUT,
		MHZ19_TIMEOUT,
		MHZ19_ERROR,
} mhz19_error_t;


#ifdef __cplusplus
extern "C" {
#endif

void mhz19_init(void);
mhz19_error_t mhz19_read(uint16_t *co2);

#ifdef __cplusplus
}
#endif

#endif /* MHZ19_H_ */
