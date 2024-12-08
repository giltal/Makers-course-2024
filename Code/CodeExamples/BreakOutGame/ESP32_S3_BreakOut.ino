/*
 Name:		ESP32_S3_BreakOut.ino
 Created:	6/27/2024 12:08:22
 Author:	GilTal
*/

#include "Math.h"
//#include "graphics.h"
#include "TFT_eSPI.h"
#include "pngAux.h"
#include "FS.h"
#include "SD.h"
#include "SPI.h"

pngObject breakOutLogo;

TFT_eSPI lcd = TFT_eSPI();
TFT_eSprite batSprite = TFT_eSprite(&lcd);


#define RGB_TO_565(R,G,B) ((unsigned short)(((G >> 3) | ((B << 3) & 0xE0)) << 8) | ((R & 0xF8) | (B >> 5)))

#define BLACK_COLOR		RGB_TO_565(0,0,0)
#define WHITE_COLOR		RGB_TO_565(255,255,255)
#define RED_COLOR		RGB_TO_565(255,0,0)
#define LIGHT_RED_COLOR	RGB_TO_565(255,50,50)
#define GREEN_COLOR		RGB_TO_565(0,255,0)
#define BLUE_COLOR		RGB_TO_565(0,0,255)
#define CYAN_COLOR		RGB_TO_565(0,255,255)
#define MAGENTA_COLOR	RGB_TO_565(255,0,255)
#define YELLOW_COLOR	RGB_TO_565(255,255,0)
#define ORANGE_COLOR	RGB_TO_565(255,165,0)


#define BAT_OFFSET		5
#define BAT_WIDTH		40
#define BAT_HEIGHT		7
#define BALL_SIZE		3

// function declarations
boolean checkCollision(int x1, int y1, int width1, int height1, int x2, int y2, int width2, int height2);

float xCalM = 0.0, yCalM = 0.0;
float xCalC = 0.0, yCalC = 0.0;
float xPos = 0.0, yPos = 0.0;
float xPosLast = 0.0, yPosLast = 0.0;
float xVel = 0.0, yVel = 0.0;
int8_t topBorder = 20;
int8_t batWidth = BAT_WIDTH;
int8_t batHeight = BAT_HEIGHT;
int16_t batX = 0, batY = 0, batOffset = BAT_OFFSET;
int8_t ballSize = BALL_SIZE;
int playerLives = 3, prevPlayerLives = 3;
int playerScore = 0, prevPlayerScore = 0;
int gameState = 1;
int level = 1, prevLevel = 1;

int16_t tftWidth = 0, tftHeight = 0;

class ScreenPoint {
public:
	int16_t x;
	int16_t y;

	ScreenPoint() {
		// default contructor
	}

	ScreenPoint(int16_t xIn, int16_t yIn) {
		x = xIn;
		y = yIn;
	}
};

class Block {
public:
	int x;
	int y;
	int width;
	int height;
	unsigned int colour;
	int score;
	boolean isActive;

	// default constructor
	Block() {}

	Block(int xpos, int ypos, int bwidth, int bheight, int bcol, int bscore) {
		x = xpos;
		y = ypos;
		width = bwidth;
		height = bheight;
		colour = bcol;
		score = bscore;
		isActive = true;
		drawBlock();
	}

	void drawBlock() {
		lcd.fillRect(x, y, width, height, colour);
	}

	void removeBlock() {
		lcd.fillRect(x, y, width, height, BLACK_COLOR);
		isActive = false;
	}

	boolean isHit(float x1, float y1, int w1, int h1) {
		if (checkCollision((int)round(x1), (int)round(y1), w1, h1, x, y, width, height)) {
			return true;
		}
		else {
			return false;
		}
	}
};

Block blocks[5][16];

ScreenPoint getScreenCoords(int16_t x, int16_t y) {
	int16_t xCoord = round((x * xCalM) + xCalC);
	int16_t yCoord = round((y * yCalM) + yCalC);
	if (xCoord < 0) xCoord = 0;
	if (xCoord >= tftWidth) xCoord = tftWidth - 1;
	if (yCoord < 0) yCoord = 0;
	if (yCoord >= tftHeight) yCoord = tftHeight - 1;
	return(ScreenPoint(xCoord, yCoord));
}

#define WHEEL_PIN 5

#define PIN_START_SELECT_BUTTON 3
#define PIN_RIGHT_LEFT_JOY		2
#define PIN_DOWN_UP_JOY			1 
#define PIN_A_B_BUTTON			4
#define WHEEL_PIN				5
#define SOUNDPIN				11

#define TINYJOYPAD_LEFT		((analogRead(PIN_RIGHT_LEFT_JOY) > 4000))
#define TINYJOYPAD_RIGHT	(((analogRead(PIN_RIGHT_LEFT_JOY) > 2000) && (analogRead(PIN_RIGHT_LEFT_JOY) < 2100)))
#define TINYJOYPAD_DOWN		(((analogRead(PIN_DOWN_UP_JOY) > 2000) && (analogRead(PIN_DOWN_UP_JOY) < 2100)))
#define TINYJOYPAD_UP		((analogRead(PIN_DOWN_UP_JOY) > 4000))
#define A_PRESSED			((analogRead(PIN_A_B_BUTTON) > 4000))	
#define B_PRESSED			(((analogRead(PIN_A_B_BUTTON) > 2000) && (analogRead(PIN_A_B_BUTTON) < 2100)))
#define START_PRESSED		((analogRead(PIN_START_SELECT_BUTTON) > 4000))	
#define SELECT_PRESSED		(((analogRead(PIN_START_SELECT_BUTTON) > 2000) && (analogRead(PIN_START_SELECT_BUTTON) < 2100)))

SPIClass SDspi(FSPI);
bool SDmountOK;

void setup() {
	Serial.begin(115200);

	pinMode(SOUNDPIN, OUTPUT);
	pinMode(PIN_START_SELECT_BUTTON, ANALOG);
	pinMode(PIN_RIGHT_LEFT_JOY, ANALOG);
	pinMode(PIN_DOWN_UP_JOY, ANALOG);
	pinMode(PIN_A_B_BUTTON, ANALOG);
	pinMode(WHEEL_PIN, ANALOG);

	//lcd.init(st7789_320x240x16, &gameConsAccessor, 6, 7, 8, 40000000);
	lcd.init();
	printf("ESP32-S3 Game\n");
	lcd.setRotation(1);
	lcd.setSwapBytes(true);
	tftWidth = lcd.width();// lcd.getXSize();
	tftHeight = lcd.height();//lcd.getYSize();

	batY = tftHeight - batHeight - batOffset;

	batSprite.createSprite(320, batHeight);
	batSprite.setSwapBytes(true);

	if (psramInit())
		printf("PSRAM init OK\n");
	printf("PSRAM size: %d\n", ESP.getFreePsram());

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

	printf("Game start!\n");
}

void initGame() {

	lcd.fillScreen(BLACK_COLOR);
	lcd.drawFastHLine(0, topBorder - 1, tftWidth, BLUE_COLOR);
	lcd.setTextSize(2);
	lcd.setTextColor(WHITE_COLOR, 0);
	
	lcd.drawString("SCORE:", 0, 1);
	lcd.drawString("LIVES:", (tftWidth / 2) - 20, 1);
	lcd.drawString("LEVEL:", tftWidth - 90, 1);

	batY = tftHeight - batHeight - batOffset;
	playerLives = 3;
	playerScore = 0;
	level = 0;

	drawLives();
	drawScore();
	drawLevel();

	initGameBoard();
}

void initGameBoard() {
	int row, col;
	int colour, score;
	clearOldBallPos();
	xPosLast = xPos = 0;
	yPosLast = yPos = 90;
	xVel = 2;
	yVel = 2 + (level);

	for (row = 0; row < 5; row++) {
		for (col = 0; col < 16; col++) {
			switch (row) {
			case 0:
				colour = LIGHT_RED_COLOR;
				score = 60;
				break;
			case 1:
				colour = BLUE_COLOR;
				score = 50;
				break;
			case 2:
			case 3:
				colour = MAGENTA_COLOR;
				score = 30;
				break;
			case 4:
			case 5:
				colour = YELLOW_COLOR;
				score = 10;
				break;
			}
			blocks[row][col] = Block(col * 20, (row * 10) + 30, 19, 9, colour, score);
		}
	}
}

void clearOldBallPos() {
	lcd.fillCircle(round(xPosLast), round(yPosLast), ballSize, BLACK_COLOR);
}

void moveBall() {
	float newX, newY;
	newX = xPos + xVel;
	newY = yPos + yVel;
	if (newX < (float)ballSize) {
		newX = (float)ballSize;
		xVel = -xVel;
	}
	if (newX > (float)(tftWidth - ballSize - 1)) {
		newX = (float)(tftWidth - ballSize - 1);
		xVel = -xVel;
	}
	if (newY < topBorder + (float)ballSize) {
		newY = topBorder + (float)ballSize;
		yVel = -yVel;
	}
	if ((round(newX) != round(xPosLast)) || (round(newY) != round(yPosLast))) {
		// draw ball
		clearOldBallPos();
		lcd.fillCircle(round(newX), round(newY), ballSize, WHITE_COLOR);
		xPosLast = newX;
		yPosLast = newY;
	}
	xPos = newX;
	yPos = newY;
}

char tempPrintBuffer[64];

void drawScore() {
	//lcd.loadFonts(SANS9);
	lcd.setTextSize(2);
	// clear old score
	lcd.setTextColor(BLACK_COLOR);
	sprintf(tempPrintBuffer, "%d", prevPlayerScore);
	lcd.drawString(tempPrintBuffer, 70, 1);
	lcd.setTextColor(WHITE_COLOR);
	sprintf(tempPrintBuffer, "%d", playerScore);
	lcd.drawString(tempPrintBuffer, 70, 1);
	prevPlayerScore = playerScore;
}

void drawLives() {
	lcd.setTextSize(2);
	// clear old lives
	lcd.setTextColor(BLACK_COLOR);
	sprintf(tempPrintBuffer, "%d", prevPlayerLives);
	lcd.drawString(tempPrintBuffer, (tftWidth / 2) + 50, 1);
	lcd.setTextColor(WHITE_COLOR);
	sprintf(tempPrintBuffer, "%d", playerLives);
	lcd.drawString(tempPrintBuffer, (tftWidth / 2) + 50, 1);
	prevPlayerLives = playerLives;
}

void drawLevel() {
	lcd.setTextSize(2);
	// clear old level
	lcd.setTextColor(BLACK_COLOR);
	sprintf(tempPrintBuffer, "%d", level);
	lcd.drawString(tempPrintBuffer, tftWidth - 20, 1);
	lcd.setTextColor(WHITE_COLOR);
	sprintf(tempPrintBuffer, "%d", level + 1);
	lcd.drawString(tempPrintBuffer, tftWidth - 20, 1);
}

void newBall() {
	xPos = 0;
	yPos = 90;
	xVel = yVel = 2;
	moveBall();
	delay(1000);
}

boolean checkBallLost() {
	if (yPos > tftHeight + ballSize) {
		return true;
	}
	else {
		return false;
	}
}
#define WHEEL_LOW_LIMIT		1200
#define WHEEL_HIGH_LIMIT	2800
void moveBat() {
	int16_t newBatX,wheelVal;
	wheelVal = analogRead(WHEEL_PIN);
	if (wheelVal < WHEEL_LOW_LIMIT)
	{
		wheelVal = WHEEL_LOW_LIMIT;
	}
	if (wheelVal > WHEEL_HIGH_LIMIT)
	{
		wheelVal = WHEEL_HIGH_LIMIT;
	}
	batX = map(wheelVal, WHEEL_LOW_LIMIT, WHEEL_HIGH_LIMIT, 0, lcd.width() - batWidth - 1);
	batSprite.fillRect(0,0,320, batHeight,BLACK_COLOR);
	batSprite.fillRect(batX, 0, batWidth, batHeight, BLUE_COLOR);
	batSprite.pushSprite(0, batY);
}

// bounding box collision detection
boolean checkCollision(int x1, int y1, int width1, int height1, int x2, int y2, int width2, int height2) {
	boolean hit = false;
	if (
		(((x2 + width2) >= x1) && (x2 <= (x1 + width1)))
		&& (((y2 + height2) >= y1) && (y2 <= (y1 + height1)))
		) {
		hit = true;
	}

	return hit;
}

void checkHitBat() {
	// check bat and bottom half of ball
	float xInc;
	boolean hit = checkCollision(batX, batY, batWidth, batHeight, (int)round(xPos) - ballSize, (int)round(yPos), ballSize * 2, ballSize);
	if (hit) {
		// reverse ball y direction but increase speed
		yVel += 0.05;
		if (yVel > 5) {
			yVel = 5;
		}
		yVel = -yVel;
		// rounded bounce
		xInc = (xPos - (float)(batX + (batWidth / 2))) / (float)batWidth;
		xVel += 6 * xInc;
		if (abs(xVel) > 6) {
			if (xVel < 0) {
				xVel = -6;
			}
			else {
				xVel = 6;
			}
		}
		// make sure ball not hitting bat
		yPos = (float)(batY - ballSize - 1);
	}
}

void checkHitBlock() {
	int row, col;
	for (row = 0; row < 5; row++) {
		for (col = 0; col < 16; col++) {
			if (blocks[row][col].isActive && blocks[row][col].isHit(xPos, yPos, ballSize * 2, ballSize * 2)) {
				blocks[row][col].removeBlock();
				playerScore += blocks[row][col].score;
				drawScore();
				yVel = -yVel;
				return;
			}
		}
	}
}

boolean checkAllBlocksHit() {
	int row, col, actives;
	actives = 0;
	for (row = 0; row < 5; row++) {
		for (col = 0; col < 16; col++) {
			if (blocks[row][col].isActive) {
				return false;
			}
		}
	}

	return true;
}

unsigned long lastFrame;

#define GS_START			1
#define GS_CLICK_TO_PLAY	2
#define GS_GAME_OVER		3
#define GS_PLAYING			4
#define GS_DRAW_BLOCKS		5

void loop(void) {

	gameState = GS_START;
	lastFrame = millis();
	bool logoLoaded = false;
	if (SDmountOK) {
		logoLoaded = loadPngFromSDcard("/breakout_logo.png", &breakOutLogo, true, false);
	}		
	// Game loop
	while (true)
	{
		// limit frame rate
		while ((millis() - lastFrame) < 20);
		lastFrame = millis();

		switch (gameState) {
		case GS_START: // start
			gameState = GS_CLICK_TO_PLAY;
			break;

		case GS_CLICK_TO_PLAY: // click to play
			if (!logoLoaded)
			{
				lcd.fillScreen(BLACK_COLOR);
				lcd.setTextColor(ORANGE_COLOR);
				lcd.setTextSize(4);
				lcd.drawString("Press A to start", 20, 104);
			}
			else
			{
				printf("Logo loaded\n");
				//displayPngObj(0, 0, &breakOutLogo);
				lcd.pushImage(0, 0, 320, 240, (unsigned short*)breakOutLogo.data);
			}
			while (!A_PRESSED);
			initGame();
			gameState = GS_PLAYING;
			break;

		case GS_PLAYING: // play
			moveBall();
			moveBat();
			checkHitBat();
			checkHitBlock();
			if (checkBallLost()) {
				playerLives--;
				drawLives();
				if (playerLives > 0) {
					newBall();
				}
				else {
					gameState = GS_GAME_OVER; // end game
				}
			}
			if (checkAllBlocksHit()) {
				gameState = GS_DRAW_BLOCKS;
			}
			break;

		case GS_DRAW_BLOCKS: // new blocks
			delay(1000);
			level++;
			drawLevel();
			initGameBoard();
			gameState = GS_PLAYING;
			break;

		case GS_GAME_OVER: // end
			lcd.fillScreen(BLACK_COLOR);
			lcd.setTextColor(WHITE_COLOR);
			lcd.setTextSize(4);
			lcd.drawString("You Scored:", 20, 50);
			lcd.setTextColor(CYAN_COLOR);
			sprintf(tempPrintBuffer, "%d", playerScore);
			lcd.drawString(tempPrintBuffer, 20, 100);
			while (!A_PRESSED);
			while (A_PRESSED);
			gameState = GS_START; // click to play
			break;
		}
	}
}