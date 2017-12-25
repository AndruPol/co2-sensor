## CO2, temperature, humidity sensor
CO2, temperature, humidity sensor with OLED on STM32F103 w ChibiOS/RT

* Display CO2 (MH-Z19), temperature, humidity (DHT22) values on OLED LCD 0.96" 128x64 SSD1306 SPI version
* Send values to PC via serial over USB
* LED warning/critical status
* Buzzer alarm on critical status
* Settings sensors on|off and warning|critical values from serial over USB
* Store settings between power off

Board pinout:
```
OLED SPI:
SPI1	Transmit Only Master	SPI1_MOSI	PA7
SPI1	Transmit Only Master	SPI1_SCK	PA5
SPI1	Hardware	        SPI1_NSS	PA4

USB:
USB	Device (FS)	USB_DM	PA11
USB	Device (FS)	USB_DP	PA12

MH-Z19:
TIM3	Input Capture direct mode	TIM3_CH1	PB4

BUZZER:
TIM2	PWM Generation CH1	TIM2_CH1	PA15

DHT22:
TIM4	Input Capture direct mode	TIM4_CH1	PB6

LED:
PA3	GPIO_Output

Board LED
PA2	GPIO_Output	

CR2032 battery connected to VBAT
```
Command list:

```
screen  - LCD on|off
buz     - buzzer alarm on|off
out     - send sensor values to PC on|off
sysled  - board LED on|off
led     - sensors status LED on|off
co2_en  - CO2 sensor polling on|off
temp_en - temperature polling on|off
hum_en  - humidity polling on|off
co2     - CO2 warning/critical set (example: c02 warn_val crit_val)
templow - temperature low warning/critical set (example: templow warn_val crit_val)
temphigh - temperature high warning/critical set (example: temphigh warn_val crit_val)
humlow  - humidity low warning/critical set (example: humlow warn_val crit_val)
humhigh - humidity high warning/critical set (example: humhigh warn_val crit_val)
buzint	- buzzer interval, sec
outint	- send sensor values to PC interval, sec
```
