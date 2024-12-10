/*
 Name:		ESP32_S3_MP_BasicProject.ino
 Created:	12/6/2024 9:25:31 AM
 Author:	97254
*/

#include "TFT_eSPI.h"
#include "pngAux.h"
#include "FS.h"
#include "SD.h"
#include "SPI.h"

pngObject breakOutLogo,spriteImg,bgImg;

TFT_eSPI lcd = TFT_eSPI();
TFT_eSprite sampleSprite = TFT_eSprite(&lcd);
TFT_eSprite bgSprite = TFT_eSprite(&lcd);

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
	printf("ESP32-S3 Platform demo.\n");
	lcd.setRotation(1);
	lcd.setSwapBytes(true);

	sampleSprite.createSprite(36, 65);
	sampleSprite.setSwapBytes(true);
	bgSprite.createSprite(320, 67);
	bgSprite.setSwapBytes(true);

	SDspi.begin(10, 44, 43, 0xff);//RX-44,TX-43
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
	bool logoLoaded = false, imageLoaded = false;
	if (SDmountOK) 
	{
		logoLoaded = loadPngFromSDcard("/breakout_logo.png", &breakOutLogo, true, false);
		imageLoaded = (loadPngFromSDcard("/spaceship.png", &spriteImg, true, true) && loadPngFromSDcard("/starsBG.png", &bgImg, true, false));
	}
	if (logoLoaded)
	{
		lcd.pushImage(0, 0, 320, 240, (unsigned short*)breakOutLogo.data);
	}
	if (imageLoaded)
	{
		sampleSprite.pushImage(0, 0, 36, 65, (unsigned short*)spriteImg.data);
	}

	while (!A_PRESSED);
	while (A_PRESSED);

	lcd.fillScreen(RGB_TO_565(0,0,0));
	lcd.setTextColor(RGB_TO_565(50,255,0));
	lcd.setTextSize(4);
	lcd.drawString("Sprite Demo", 40, 80);
	
	int i;
	if (imageLoaded)
	{
		while (1)
		{
			for (i = 0; i < 280; i++)
			{
				bgSprite.pushImage(0, 0, 320, 67, (unsigned short*)bgImg.data);
				sampleSprite.pushToSprite(&bgSprite, i, 1, 0xffff);
				bgSprite.pushSprite(0, 140);
			}
			for (i = 280; i > 0; i--)
			{
				bgSprite.pushImage(0, 0, 320, 67, (unsigned short*)bgImg.data);
				sampleSprite.pushToSprite(&bgSprite, i, 1, 0xffff);
				bgSprite.pushSprite(0, 140);
			}
		}
	}
}