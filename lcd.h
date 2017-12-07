/*
 * lcd.h
 *
 *  Created on: 05.11.2017
 *      Author: andru
 */

#ifndef LCD_H_
#define LCD_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "ssd1306.h"

extern SSD1306_COLOR_t color_critical;

void lcd_init(void);
void lcd_prepare(void);
void lcd_temperature(const int temp);
void lcd_humidity(const int hum);
void lcd_co2(const uint16_t co2);
void lcd_update(void);

#ifdef __cplusplus
}
#endif

#endif /* LCD_H_ */
