/*
 Name:		ESP32_S3_MP_EmptyProject.ino
 Created:	12/17/2024 8:50:19 PM
 Author:	97254
*/

#include "TFT_eSPI.h"
#include "pngAux.h"
#include "FS.h"
#include "SD.h"
#include "SPI.h"

TFT_eSPI lcd = TFT_eSPI();

#define RGB_TO_565(R,G,B) ((unsigned short)(((G >> 3) | ((B << 3) & 0xE0)) << 8) | ((R & 0xF8) | (B >> 5)))

SPIClass SDspi(FSPI);
bool SDmountOK;

#define SOUNDPIN				11 
#define PIN_START_SELECT_BUTTON 3
#define PIN_RIGHT_LEFT_JOY		2
#define PIN_DOWN_UP_JOY			1 
#define PIN_A_B_BUTTON			4
#define WHEEL_PIN				5

#define JOY_UPPER_LIMIT		3500
#define JOY_MID_LOW			1800
#define	JOY_MID_HIGH		2300

#define TINYJOYPAD_LEFT		((analogRead(PIN_RIGHT_LEFT_JOY) > JOY_UPPER_LIMIT))
#define TINYJOYPAD_RIGHT	(((analogRead(PIN_RIGHT_LEFT_JOY) > JOY_MID_LOW) && (analogRead(PIN_RIGHT_LEFT_JOY) < JOY_MID_HIGH)))
#define TINYJOYPAD_DOWN		(((analogRead(PIN_DOWN_UP_JOY) > JOY_MID_LOW) && (analogRead(PIN_DOWN_UP_JOY) < JOY_MID_HIGH)))
#define TINYJOYPAD_UP		((analogRead(PIN_DOWN_UP_JOY) > JOY_UPPER_LIMIT))
#define A_PRESSED			((analogRead(PIN_A_B_BUTTON) > JOY_UPPER_LIMIT))	
#define B_PRESSED			(((analogRead(PIN_A_B_BUTTON) > JOY_MID_LOW) && (analogRead(PIN_A_B_BUTTON) < JOY_MID_HIGH)))
#define START_PRESSED		((analogRead(PIN_START_SELECT_BUTTON) > JOY_UPPER_LIMIT))	
#define SELECT_PRESSED		(((analogRead(PIN_START_SELECT_BUTTON) > JOY_MID_LOW) && (analogRead(PIN_START_SELECT_BUTTON) < JOY_MID_HIGH)))


// the setup function runs once when you press reset or power the board
void setup()
{
	Serial.begin(115200);

	pinMode(SOUNDPIN, OUTPUT);
	pinMode(PIN_START_SELECT_BUTTON, ANALOG);
	pinMode(PIN_RIGHT_LEFT_JOY, ANALOG);
	pinMode(PIN_DOWN_UP_JOY, ANALOG);
	pinMode(PIN_A_B_BUTTON, ANALOG);
	pinMode(WHEEL_PIN, ANALOG);

	if (psramInit())
		printf("PSRAM init OK\n");
	printf("PSRAM size: %d\n", ESP.getFreePsram());
	printf("PSRAMfound = %d\n", psramFound());

	lcd.init();
	printf("ESP32-S3 A-sync web server demo\n");
	lcd.setRotation(1);
	lcd.setSwapBytes(true);

	SDspi.begin(10, 44, 43, 18);//RX-44,TX-43
	if (!SD.begin(4, SDspi, 10000000, "/sd", 5))
	{
		printf("Failed to mount SD card!\n");
		SDmountOK = false;
	}
	else
	{
		printf("SD card mounted on HSPI.\n");
		SDmountOK = true;
	}
}

// the loop function runs over and over again until power down or reset
void loop()
{
	lcd.fillScreen(RGB_TO_565(0, 0, 0));
	lcd.setTextColor(RGB_TO_565(50, 255, 0));
	lcd.setTextSize(4);
	lcd.drawString("Empty Project", 5, 80);
	while (1);
}