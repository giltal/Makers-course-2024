/*
 Name:		ESP32_S3_MiniMakerPlat_SpaceshipGame.ino
 Created:	7/1/2024 7:27:15 PM
 Author:	GilTal
*/
#if 1
#include "TFT_eSPI.h"
#include "picture.h"
#include "ship.h"
#include "enemy.h"
#include "rocket.h"
#include "rocketH.h"
#include "start.h"

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite sprite = TFT_eSprite(&tft);
TFT_eSprite player = TFT_eSprite(&tft);
TFT_eSprite enemy = TFT_eSprite(&tft);
TFT_eSprite stats = TFT_eSprite(&tft);

#define gray 0xA514

int imageW = 480;
int imageH = 480;
int screenW = 320;
int screenH = 140;
int m = imageW;

int start = 1;
unsigned short imageS[44800] = { 0 }; // edit this screenW * screen H

int pos = 0;
int x = 0;
int y = 30;
int changeX = 1;
int changeY = 1;

int ex = 0;
int ey = 0;
int ecX = 1;
int ecY = 1;

#define left 43
#define up 44
#define down 18
#define right 17
#define a 21
#define b 16
#define color   0x0000
int px = 30;
int py = 80;
int w = 63;
int h = 28;

bool bullet[10] = { 0 }; // my bulets
int bx[10] = { 0 };    // position x of each bullet
int by[10] = { 0 };    //position y of each bullet
int bullet_counter = 0;
int bdamage = 4;

bool rocket[3] = { 0 };
int rx[3] = { 0 };
int ry[3] = { 0 };

int rdamage = 20;

bool ebullet[8] = { 0 };
int ebx[10] = { 0 };    // position x of each Enemy bullet
int eby[10] = { 0 };    //position y of each Enemy bullet
int ebullet_counter = 0;
int edamage = 10;

int debounceA = 0;
int debounceB = 0;

bool e = 0;

int level = 1;
int score = 0;
int rocketN = 2;
long tt = 0;
int health = 100;
int shield = 100;
int enemyHealth = 100;
int miss = 0;
int hit = 0;
int headshot = 0;
int Min = 0;
int Sec = 0;
long Time = 0;
bool gameOver = 0;

String Minutes;
String Seconds;

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

void setup() {

    Serial.begin(115200);
    delay(250);
    
    pinMode(PIN_START_SELECT_BUTTON, ANALOG);
    pinMode(PIN_RIGHT_LEFT_JOY, ANALOG);
    pinMode(PIN_DOWN_UP_JOY, ANALOG);
    pinMode(PIN_A_B_BUTTON, ANALOG);
    pinMode(WHEEL_PIN, ANALOG);

    //pinMode(0, INPUT_PULLUP); //...................................................delete

    tft.init();
    printf("ESP32-S3 Game\n");
    tft.setRotation(1);
    tft.setSwapBytes(true);
    
    tft.setTextColor(TFT_SILVER, color);
    tft.fillScreen(0);
    tft.drawString("T-DISPLAY S3", 5, 162);
    tft.drawLine(0, 19, 320, 19, TFT_WHITE);
    tft.drawLine(0, 160, 320, 160, TFT_WHITE);
    sprite.createSprite(320, 140);
    sprite.setSwapBytes(true);
    player.createSprite(63, 40);
    player.setTextColor(TFT_WHITE, TFT_BLACK);
    enemy.createSprite(aniWidth, aniHeigth);
    stats.createSprite(320, 19);
    stats.setSwapBytes(true);

    ey = random(20, 50);
    ex = random(190, 250);
    tft.fillScreen(0);
    tft.pushImage(0, 35, 320, 170, startScreen);
    while (!A_PRESSED)
    {
    }
    tft.fillScreen(color);
}

void drawBackground()
{
    pos = x + imageW * y;
    start = pos;
    m = screenW + pos;
    for (int i = 0; i < screenW * screenH; i++)
    {
        if (start % m == 0)
        {
            start = start + (imageW - screenW);
            m = m + imageW;
        }
        imageS[i] = picture[start];
        start++;

    }
    x = x + changeX;
    if (x == imageW - screenW - 1 || x < 0)
        changeX = changeX * -1;

    y = y + changeY;
    if (y == imageH - screenH - 1 || y < 0)
        changeY = changeY * -1;

    ex = ex + ecX;
    if (ex == 250 || ex < 180)
        ecX = ecX * -1;

    ey = ey + ecY;
    if (ey == 90 || ey < -20)
        ecY = ecY * -1;
}

void showStats()
{
    stats.fillSprite(color);
    stats.setTextColor(gray, color);
    stats.drawString("SHIELD:", 4, 2, 2);

    stats.drawRect(56, 4, 54, 11, gray);
    stats.fillRect(58, 6, map(health, 0, 100, 0, 50), 7, TFT_GREEN);

    for (int i = 0; i < rocketN + 1; i++)
        stats.pushImage(120 + (i * 8) + (i * 3), 3, 7, 12, rrocket);

    stats.drawString("LVL:      SCORE:", 164, 2, 2);

    stats.setTextColor(0x7BF, color);
    stats.drawString(String(level), 194, 2, 2);
    stats.drawString(String(score), 276, 2, 2);
}

void checkColision()
{
    for (int i = 0; i < 8; i++)
    {
        if (ebullet[i] == true)
            if (ebx[i] > px && ebx[i]<px + 58 && eby[i]>py && eby[i] < py + 27)
            {
                health = health - edamage;
                ebullet[i] = false;
            }
    }

    for (int i = 0; i < 10; i++)
    {
        if (bullet[i] == true)
            if (bx[i] > ex + 10 && bx[i]<ex + 62 && by[i]>ey && by[i] < ey + 66)
            {
                if (by[i] > ey + 25 && by[i] < ey + 25 + 18)
                {
                    score = score + 12; headshot++;
                    enemyHealth = enemyHealth - 3;
                }
                else
                {
                    score = score + 10;
                    enemyHealth = enemyHealth - 2;
                }
                hit++;

                bullet[i] = false;
            }
    }

    for (int i = 0; i < 3; i++)
    {
        if (rocket[i] == true)
            if (rx[i] > ex + 10 && rx[i]<ex + 62 && ry[i]>ey && ry[i] < ey + 66)
            {

                score = score + 16;

                hit++;
                enemyHealth = enemyHealth - 12;
                rocket[i] = false;
            }
    }
}

int frameC = 0;

long timeLast = 0;
long timeNext = 1000;

void reset()
{
    level = 1;
    score = 0;
    rocketN = 2;
    tt = 0;
    health = 100;
    shield = 100;
    enemyHealth = 100;
    edamage = 10;
    miss = 0;
    hit = 0;
    headshot = 0;
    Min = 0;
    Sec = 0;
    Time = 0;
    ey = random(20, 50);
    ex = random(190, 250);
    gameOver = 0;
}

void loop() {

    if (SELECT_PRESSED)
        reset();
    showStats();
    checkColision();

    //check time
    if (Time + 1000 < millis())
    {
        Time = millis();
        Sec++;
        if (Sec > 59)
        {
            Sec = 0; Min++;
        }
        if (Sec < 10) Seconds = "0" + String(Sec); else Seconds = String(Sec);
        if (Min < 10) Minutes = "0" + String(Min); else Minutes = String(Min);
    }

    if (timeLast + timeNext < millis())
    {
        timeLast = millis();
        timeNext = (random(6, 15) * 100) - level * 30;
        ebullet[ebullet_counter] = true;
        ebx[ebullet_counter] = ex + aniWidth / 2;
        eby[ebullet_counter] = ey + aniHeigth / 2;
        ebullet_counter++;
        if (ebullet_counter == 8)
            ebullet_counter = 0;
    }

    if (A_PRESSED)
    {
        if (debounceA == 0)
        {
            debounceA = 1;
            bullet[bullet_counter] = true;
            bx[bullet_counter] = px + w;
            by[bullet_counter] = py + h / 2;
            bullet_counter++;
            if (bullet_counter == 10)
                bullet_counter = 0;
        }
    }
    else debounceA = 0;

    if (B_PRESSED)
    {
        if (debounceB == 0 && rocketN >= 0)
        {
            debounceB = 1;
            rocket[rocketN] = true;
            rx[rocketN] = px + w;
            ry[rocketN] = py + h / 2;
            rocketN--;
        }
    }
    else debounceB = 0;


    if (TINYJOYPAD_UP && py > 0)
        py = py - 3;
    if (TINYJOYPAD_DOWN && py < 140 - 28)
        py = py + 3;

    if (TINYJOYPAD_LEFT && px > 0)
        px = px - 3;
    if (TINYJOYPAD_RIGHT && px < 290)
        px = px + 3;

    if (e == 0) {
        drawBackground();
        frameC++;
    }

    e = !e;
    sprite.pushImage(0, 0, screenW, screenH, imageS);

    player.pushImage(0, 0, w, h, ship);
    for (int i = 0; i < 8; i++)
        player.fillCircle(random(0, 10), random(9, 18), 1, TFT_RED);
    player.drawString(String(health) + "%", 16, 32);
    enemy.pushImage(0, 0, aniWidth, aniHeigth, logo2[frameC]);

    player.pushToSprite(&sprite, px, py, TFT_BLACK);
    enemy.pushToSprite(&sprite, ex, ey, TFT_BLACK);

    for (int i = 0; i < 10; i++)
        if (bullet[i] == true) {
            bx[i] = bx[i] + 4;
            if (bx[i] < 322)
                sprite.fillCircle(bx[i], by[i], 2, 0xEA5B); /// player bullets........................................
            else {
                miss++;
                bullet[i] = false;
            }
        }

    for (int i = 0; i < 8; i++)
        if (ebullet[i] == true) {
            ebx[i] = ebx[i] - 4;
            if (ebx[i] > -4)
                sprite.fillCircle(ebx[i], eby[i], 3, 0x7BF);
            else
                ebullet[i] = false;
        }

    for (int i = 0; i < 3; i++)
        if (rocket[i] == true) {
            rx[i] = rx[i] + 2;
            if (rx[i] < 320)
                sprite.pushImage(rx[i], ry[i] - 6, 12, 7, rocketH);
            else
                rocket[i] = false;
        }

    sprite.drawRoundRect(268, 8, 44, 14, 3, gray);

    sprite.setTextColor(gray, TFT_BLACK);
    sprite.drawString(Minutes + ":" + Seconds, 276, 11);
    sprite.drawString("HEADS:" + String(headshot), 254, 128);
    sprite.drawString("HIT :" + String(hit), 254, 104);
    sprite.drawString("MISS :" + String(miss), 254, 116);
    sprite.drawString("ENEMY HEALTH:", 8, 128);
    sprite.fillRect(92, 129, map(enemyHealth, 0, 100, 0, 50), 5, 0xF4DE); //..............enemy healthBAr
    sprite.drawRect(90, 127, 54, 9, gray); //..............enemy healthBAr
    sprite.drawLine(0, 0, 320, 0, 0x4C7A);
    sprite.drawLine(0, 139, 320, 139, 0x4C7A);

    sprite.pushSprite(0, 60);
    stats.pushSprite(0, 40);

    if (health <= 0)
    {
        tft.fillScreen(color);
        tft.drawString("GAME OVER", 40, 40, 4);
        tft.drawString("SCORE: " + String(score), 40, 65, 2);
        tft.drawString("LEVEL: " + String(level), 40, 85, 2);
        tft.drawString("TIME: " + Minutes + ":" + Seconds, 40, 105, 2);
        tft.drawString("HEADSHOT: " + String(headshot), 234, 128);
        tft.drawString("HIT :" + String(hit), 234, 104);
        tft.drawString("MISS :" + String(miss), 234, 116);

        delay(2000);
        while (!START_PRESSED)
        {
        }
        reset();
    }
    if (enemyHealth <= 0)
    {
        level++;
        enemyHealth = 100 + (15 * level);
        rocketN = 2;
        edamage++;
        tft.fillScreen(color);
        tft.drawString("LEVEL " + String(level), 80, 80, 4);
        delay(3000);
    }
    if (frameC == framesNumber - 1)
        frameC = 0;
}
#else
#include <TFT_eSPI.h> // Hardware-specific library
#include <SPI.h>

TFT_eSPI tft = TFT_eSPI();       // Invoke custom library

#define DEG2RAD 0.0174532925

#define LOOP_DELAY 10 // Loop delay to slow things down

byte inc = 0;
unsigned int col = 0;

byte red = 31; // Red is the top 5 bits of a 16-bit colour value
byte green = 0;// Green is the middle 6 bits
byte blue = 0; // Blue is the bottom 5 bits
byte state = 0;

void setup(void) {
    tft.begin();

    tft.setRotation(1);

    tft.fillScreen(TFT_BLACK);

}


void loop() {

    // Continuous elliptical arc drawing
    fillArc(160, 120, inc * 6, 1, 140, 100, 10, rainbow(col));

    // Continuous segmented (inc*2) elliptical arc drawing
    fillArc(160, 120, ((inc * 2) % 60) * 6, 1, 120, 80, 30, rainbow(col));

    // Circle drawing using arc with arc width = radius
    fillArc(160, 120, inc * 6, 1, 42, 42, 42, rainbow(col));

    inc++;
    col += 1;
    if (col > 191) col = 0;
    if (inc > 59) inc = 0;

    delay(LOOP_DELAY);
}


// #########################################################################
// Draw a circular or elliptical arc with a defined thickness
// #########################################################################

// x,y == coords of centre of arc
// start_angle = 0 - 359
// seg_count = number of 6 degree segments to draw (60 => 360 degree arc)
// rx = x axis outer radius
// ry = y axis outer radius
// w  = width (thickness) of arc in pixels
// colour = 16-bit colour value
// Note if rx and ry are the same then an arc of a circle is drawn

void fillArc(int x, int y, int start_angle, int seg_count, int rx, int ry, int w, unsigned int colour)
{

    byte seg = 6; // Segments are 3 degrees wide = 120 segments for 360 degrees
    byte inc = 6; // Draw segments every 3 degrees, increase to 6 for segmented ring

    // Calculate first pair of coordinates for segment start
    float sx = cos((start_angle - 90) * DEG2RAD);
    float sy = sin((start_angle - 90) * DEG2RAD);
    uint16_t x0 = sx * (rx - w) + x;
    uint16_t y0 = sy * (ry - w) + y;
    uint16_t x1 = sx * rx + x;
    uint16_t y1 = sy * ry + y;

    // Draw colour blocks every inc degrees
    for (int i = start_angle; i < start_angle + seg * seg_count; i += inc) {

        // Calculate pair of coordinates for segment end
        float sx2 = cos((i + seg - 90) * DEG2RAD);
        float sy2 = sin((i + seg - 90) * DEG2RAD);
        int x2 = sx2 * (rx - w) + x;
        int y2 = sy2 * (ry - w) + y;
        int x3 = sx2 * rx + x;
        int y3 = sy2 * ry + y;

        tft.fillTriangle(x0, y0, x1, y1, x2, y2, colour);
        tft.fillTriangle(x1, y1, x2, y2, x3, y3, colour);

        // Copy segment end to segment start for next segment
        x0 = x2;
        y0 = y2;
        x1 = x3;
        y1 = y3;
    }
}

// #########################################################################
// Return the 16-bit colour with brightness 0-100%
// #########################################################################
unsigned int brightness(unsigned int colour, int brightness)
{
    byte red = colour >> 11;
    byte green = (colour & 0x7E0) >> 5;
    byte blue = colour & 0x1F;

    blue = (blue * brightness) / 100;
    green = (green * brightness) / 100;
    red = (red * brightness) / 100;

    return (red << 11) + (green << 5) + blue;
}

// #########################################################################
// Return a 16-bit rainbow colour
// #########################################################################
unsigned int rainbow(byte value)
{
    // Value is expected to be in range 0-127
    // The value is converted to a spectrum colour from 0 = blue through to 127 = red

    switch (state) {
    case 0:
        green++;
        if (green == 64) {
            green = 63;
            state = 1;
        }
        break;
    case 1:
        red--;
        if (red == 255) {
            red = 0;
            state = 2;
        }
        break;
    case 2:
        blue++;
        if (blue == 32) {
            blue = 31;
            state = 3;
        }
        break;
    case 3:
        green--;
        if (green == 255) {
            green = 0;
            state = 4;
        }
        break;
    case 4:
        red++;
        if (red == 32) {
            red = 31;
            state = 5;
        }
        break;
    case 5:
        blue--;
        if (blue == 255) {
            blue = 0;
            state = 0;
        }
        break;
    }
    return red << 11 | green << 5 | blue;
}
#endif
