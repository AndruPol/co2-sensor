/*
 * lcd.c
 *
 *  Created on: 05.11.2017
 *      Author: andru
 */


#include "ch.h"

#include "main.h"
#include "ssd1306.h"
#include "fonts.h"
#include "util/printfs.h"

static bool blink = false;
SSD1306_COLOR_t color_critical;

void lcd_init(void) {
	char line[30];

	SSD1306_Init();
	SSD1306_On();

	SSD1306_Fill(SSD1306_COLOR_BLACK);
	SSD1306_GotoXY(0, 16);
	sprintf(line, "CO2 meter v%s", FIRMWARE);
	SSD1306_Puts(line, &Font8x13, SSD1306_COLOR_WHITE);
	SSD1306_GotoXY(0, 32);
	sprintf(line, "made by @ndru");
	SSD1306_Puts(line, &Font8x13, SSD1306_COLOR_WHITE);
	SSD1306_Refresh();
}

void lcd_prepare(void) {
	SSD1306_Off();
	color_critical = SSD1306_COLOR_WHITE;
	if (system_config.config.data.screen_en == 1) {
		SSD1306_Fill(SSD1306_COLOR_BLACK);
		//	SSD1306_DrawLine(0, 14, 127, 14, SSD1306_COLOR_WHITE);
		SSD1306_DrawRectangle(0, 16, 128, 28, SSD1306_COLOR_WHITE);
		SSD1306_DrawRectangle(0, 44, 64, 20, SSD1306_COLOR_WHITE);
		SSD1306_DrawRectangle(64, 44, 64, 20, SSD1306_COLOR_WHITE);
		SSD1306_On();
		SSD1306_Refresh();
	}
}

void lcd_temperature(void) {
	SSD1306_COLOR_t color;
	char stemp[5] = "    ";

	if (system_config.config.data.screen_en == 0)
		return;

	// '~' - replaced to degree Celsius character in Font10x17
	if (dht.temp_state != UNKNOWN) {
		sprintf(stemp, "%2.1f~", (float) dht.temperature / 10);
	}
	switch (dht.temp_state) {
	case UNKNOWN:
		color = SSD1306_COLOR_WHITE;
		break;
	case CRITICAL:
		color = color_critical;
		break;
	case OK:
		color = SSD1306_COLOR_WHITE;
		break;
	default:
		color = SSD1306_COLOR_BLACK;
		break;
	}
	SSD1306_DrawFilledRectangle(1, 45, 62, 18, !color);
	SSD1306_DrawRectangle(0, 44, 64, 20, color);
	SSD1306_GotoXY(8, 46);
	SSD1306_Puts(stemp, &Font10x17, color);
}

void lcd_humidity(void) {
	SSD1306_COLOR_t color;
	char shum[5] = "    ";

	if (system_config.config.data.screen_en == 0)
		return;

	if (dht.hum_state != UNKNOWN) {
		sprintf(shum, "%2.1f%%", (float) dht.humidity / 10);
	}
	switch (dht.hum_state) {
	case UNKNOWN:
		color = SSD1306_COLOR_WHITE;
		break;
	case CRITICAL:
		color = color_critical;
		break;
	case OK:
		color = SSD1306_COLOR_WHITE;
		break;
	default:
		color = SSD1306_COLOR_BLACK;
		break;
	}
	SSD1306_DrawFilledRectangle(65, 45, 62, 18, !color);
	SSD1306_DrawRectangle(64, 44, 64, 20, color);
	SSD1306_GotoXY(72, 46);
	SSD1306_Puts(shum, &Font10x17, color);
}

void lcd_co2(void) {
	SSD1306_COLOR_t color;
	char sco2[5] = "    ";

	if (system_config.config.data.screen_en == 0)
		return;

	if (mhz19.state != UNKNOWN) {
		sprintf(sco2, "%4d", mhz19.co2);
	}
	switch (mhz19.state) {
	case UNKNOWN:
		color = SSD1306_COLOR_WHITE;
		break;
	case CRITICAL:
		color = color_critical;
		break;
	case OK:
		color = SSD1306_COLOR_WHITE;
		break;
	default:
		color = SSD1306_COLOR_BLACK;
		break;
	}
	SSD1306_DrawFilledRectangle(1, 17, 126, 26, !color);
	SSD1306_DrawRectangle(0, 16, 128, 28, color);
	if (!mhz19_pwrup.active) {
		SSD1306_GotoXY(20, 18);
		SSD1306_Puts(sco2, &Font16x24, color);
		if (mhz19.state != UNKNOWN) {
			SSD1306_GotoXY(90, 18);
			SSD1306_Puts("ppm", &Font10x17, color);
		}
	}
}

void lcd_update(void) {
	if (system_config.config.data.screen_en == 0)
		return;

	SSD1306_DrawFilledRectangle(0, 0, 128, 15, SSD1306_COLOR_BLACK);
	SSD1306_GotoXY(0, 1);
	if (system_config.config.data.co2_en)
		SSD1306_Putc(SYM_CO2, &Symbol15x15, SSD1306_COLOR_WHITE);
	else
		SSD1306_Putc(SYM_SPACE, &Symbol15x15, SSD1306_COLOR_WHITE);
	if (system_config.config.data.dht_temp_en)
		SSD1306_Putc(SYM_TEMP, &Symbol15x15, SSD1306_COLOR_WHITE);
	else
		SSD1306_Putc(SYM_SPACE, &Symbol15x15, SSD1306_COLOR_WHITE);
	if (system_config.config.data.dht_hum_en)
		SSD1306_Putc(SYM_HUM, &Symbol15x15, SSD1306_COLOR_WHITE);
	else
		SSD1306_Putc(SYM_SPACE, &Symbol15x15, SSD1306_COLOR_WHITE);
	if (system_config.config.data.buzzer_en)
		SSD1306_Putc(SYM_SOUND, &Symbol15x15, SSD1306_COLOR_WHITE);
	else
		SSD1306_Putc(SYM_SPACE, &Symbol15x15, SSD1306_COLOR_WHITE);
	if (system_config.config.data.redled_en)
		SSD1306_Putc(SYM_LED, &Symbol15x15, SSD1306_COLOR_WHITE);
	else
		SSD1306_Putc(SYM_SPACE, &Symbol15x15, SSD1306_COLOR_WHITE);
	if (system_config.config.data.output_en)
		SSD1306_Putc(SYM_OUTPUT, &Symbol15x15, SSD1306_COLOR_WHITE);
	else
		SSD1306_Putc(SYM_SPACE, &Symbol15x15, SSD1306_COLOR_WHITE);
	if (usb_active())
		SSD1306_Putc(SYM_USB, &Symbol15x15, SSD1306_COLOR_WHITE);
	else
		SSD1306_Putc(SYM_SPACE, &Symbol15x15, SSD1306_COLOR_WHITE);

	// blinking figure on right up corner
	SSD1306_GotoXY(110, 1);
	if (blink) {
		SSD1306_Putc(SYM_ON, &Symbol15x15, SSD1306_COLOR_WHITE);
	} else {
		SSD1306_Putc(SYM_OFF, &Symbol15x15, SSD1306_COLOR_WHITE);
	}
	blink = !blink;

	if (mhz19_pwrup.active) {
		char s[4] = "";
		SSD1306_DrawFilledRectangle(1, 17, 126, 26, SSD1306_COLOR_BLACK);
		SSD1306_DrawRectangle(0, 16, 128, 28, SSD1306_COLOR_WHITE);
		sprintf(s, "%d", mhz19_pwrup.delay);
		SSD1306_GotoXY(12, 26);
		SSD1306_Puts("pls wait..", &Font8x13, SSD1306_COLOR_WHITE);
		SSD1306_Puts(s, &Font8x13, SSD1306_COLOR_WHITE);
	} else {
		lcd_co2();
	}
	lcd_temperature();
	lcd_humidity();

    color_critical = !color_critical;
	SSD1306_Refresh();
}
