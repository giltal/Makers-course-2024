/*
 Name:		ESP32_S3_MP_AsyncWebServer.ino
 Created:	12/17/2024 8:50:19 PM
 Author:	97254
*/

#include "TFT_eSPI.h"
#include "pngAux.h"
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include "SPIFFS.h"

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

// Web Server
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h> 
#include <ESPmDNS.h>

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

void webServerSetup();
void action(AsyncWebServerRequest* request);
void mainPage(AsyncWebServerRequest* request);
void setupWiFiPage(AsyncWebServerRequest* request);
void handleWebMenuPress(AsyncWebServerRequest* request);
void handleWebPlusPress(AsyncWebServerRequest* request);
void handleWebMinusPress(AsyncWebServerRequest* request);
void handleTempHumiPage(AsyncWebServerRequest* request);
void handleIOsPage(AsyncWebServerRequest* request);

float readDHTTemperature();
float readDHTHumidity();
String processor(const String& var);

bool spiffsOK = false;
////////////////////// WiFi ////////////////////////////

uint32_t IO[3] = { 0,0,0 };

String HotSpotName = "TPLINK";
String HotSpotPass = "";
String APpassword = "11223344";
String APname = "MiMakeAccessPoint";

TaskHandle_t wifiTask;

bool wifiEnabled = false, useHotSpot = false;
unsigned int wifiStatusFlag = 0;

bool webMenuPressed = false, webPlusPressed = false, webMinusPressed = false;
bool wifiSetupPageVisited = false;
unsigned char* webMainPageDataPointer = NULL;
unsigned int webMainPageDataSize = 0;

#define WIFI_STATUS_NOT_IN_USE				0
#define WIFI_STATUS_AP_IN_USE				1
#define WIFI_STATUS_CONNECTED_TO_AP			2
#define WIFI_STATUS_TRYING_TO_CONNECT_TO_AP	3

void wifiTaskCode(void* pvParameters)
{
	webServerSetup();
	WiFi.begin();
	AsyncElegantOTA.begin(&server);
	server.begin();
	int numOfClientsConnected;
	unsigned int reconnectCounter = 50;

	while (1)
	{
		numOfClientsConnected = WiFi.softAPgetStationNum();
		if (numOfClientsConnected > 0)
		{
			if (wifiEnabled)
			{
				WiFi.disconnect();
				wifiEnabled = false;
			}
			wifiStatusFlag = WIFI_STATUS_AP_IN_USE;
		}
		else
		{
			if (WiFi.isConnected() && useHotSpot)
			{
				wifiStatusFlag = WIFI_STATUS_CONNECTED_TO_AP;
				reconnectCounter = 0;
				wifiEnabled = true;
			}
			else if (useHotSpot)
			{
				wifiStatusFlag = WIFI_STATUS_TRYING_TO_CONNECT_TO_AP;
				reconnectCounter++;
				if (reconnectCounter >= 50)
				{
					reconnectCounter = 0;
					WiFi.disconnect();
					WiFi.begin(HotSpotName.c_str(), HotSpotPass.c_str());
					wifiEnabled = true;
				}
			}
			else
			{
				if (wifiEnabled)
				{
					WiFi.disconnect();
					wifiEnabled = false;
					reconnectCounter = 0;
				}
				wifiStatusFlag = WIFI_STATUS_NOT_IN_USE;
			}
		}
		delay(100);
	}
}

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
		
	spiffsOK = false;
	if (SPIFFS.begin())
	{
		printf("SPIFFS mounted\n");
		spiffsOK = true;
	}
	else
	{
		if (SPIFFS.format())
		{
			printf("SPIFFS formated\n");
			if (SPIFFS.begin())
			{
				printf("SPIFFS mounted\n");
				spiffsOK = true;
			}
		}
	}

	if (spiffsOK == false)
	{
		printf("SPIFFS error!\n");
	}
	if (spiffsOK)
	{
		File file;
		// WiFi Part
		printf("Load WiFi info:\n");
		file = SPIFFS.open("/wifi.txt", "r");
		if (file)
		{
			String tempS = file.readStringUntil('\n');
			useHotSpot = tempS.toInt();
			printf("useHotSpot = %d\n", useHotSpot);
			APpassword = file.readStringUntil('\n');
			printf("APpassword - %s\n", APpassword.c_str());
			HotSpotName = file.readStringUntil('\n');
			printf("accessPoint - %s\n", HotSpotName.c_str());
			HotSpotPass = file.readStringUntil('\n');
			printf("password - %s\n", HotSpotPass.c_str());
			file.close();
			if (HotSpotName.indexOf('\r') == -1)
				HotSpotName = HotSpotName.substring(0, HotSpotName.indexOf('\n'));
			else
				HotSpotName = HotSpotName.substring(0, HotSpotName.indexOf('\r'));
			HotSpotName += '\0';

			if (HotSpotPass.indexOf('\r') == -1)
				HotSpotPass = HotSpotPass.substring(0, HotSpotPass.indexOf('\n'));
			else
				HotSpotPass = HotSpotPass.substring(0, HotSpotPass.indexOf('\r'));
			HotSpotPass += '\0';
		}
		else
		{
			printf("Error: WiFi.txt cannot be opened\n");
		}
	}

	WiFi.mode(WIFI_AP_STA);
	
	if (APpassword.indexOf('\r') == -1)
		APpassword = APpassword.substring(0, APpassword.indexOf('\n'));
	else
		APpassword = APpassword.substring(0, APpassword.indexOf('\r'));
	APpassword += '\0';
	printf("Pass len: %d\n", strlen(APpassword.c_str()));
	
	if (!WiFi.softAP(APname, APpassword))
	{
		printf("Failed to init WiFi AP\n");
	}
	else
	{
		printf("AP IP address: ");
		printf("%s\n",WiFi.softAPIP().toString().c_str());
	}
	
	if (!MDNS.begin("MiMake"))
	{
		printf("Error setting up MDNS responder!\n");
	}
	else
		printf("mDNS responder started\n");

	xTaskCreatePinnedToCore(
		wifiTaskCode, /* Task function. */
		"wifiTask",   /* name of task. */
		10000,     /* Stack size of task */
		NULL,      /* parameter of the task */
		5,         /* priority of the task */
		&wifiTask,    /* Task handle to keep track of created task */
		0);        /* pin task to core 0 */

	File file;
	// WiFi Part
	printf("Load mainPage.html\n");
	file = SPIFFS.open("/mainPage.html", "r");
	if (file)
	{
		webMainPageDataSize = file.size();
		printf("mainPage.html size = %d\n", webMainPageDataSize);
		webMainPageDataPointer = (unsigned char*)malloc(webMainPageDataSize + 2);
		if (webMainPageDataPointer)
		{
			printf("webMainPageDataPointer allocated!\n");
			file.read(webMainPageDataPointer, webMainPageDataSize);
			webMainPageDataPointer[webMainPageDataSize + 1] = '\0';
		}
		file.close();
	}
}

// the loop function runs over and over again until power down or reset
void loop()
{
	lcd.fillScreen(RGB_TO_565(0, 0, 0));
	lcd.setTextColor(0xffff);
	if (SPIFFS.exists("/StarJedi_28.vlw") == false)
	{
		printf("Error: fonts StarJedi_28 are missing from SPIFFS\n");
		lcd.setTextSize(4);
	}
	else
	{
		lcd.loadFont("StarJedi_28");
	}
	lcd.setTextColor(RGB_TO_565(255, 100, 100));
	lcd.setCursor(0, 10);
	lcd.printf("WiFi status:\n");
	lcd.setCursor(0, 50);
	lcd.printf("WiFi AP status:\n");
	lcd.setCursor(0, 90);
	lcd.printf("AP iP: %s\n", WiFi.softAPIP().toString().c_str());
	lcd.drawCircle(305, 20, 13, RGB_TO_565(255, 255, 255));
	lcd.drawCircle(305, 60, 13, RGB_TO_565(255, 255, 255));
	// IOs
	lcd.fillCircle(80, 200, 27, RGB_TO_565(255, 255, 255));
	lcd.fillCircle(160, 200, 27, RGB_TO_565(255, 255, 255));
	lcd.fillCircle(240, 200, 27, RGB_TO_565(255, 255, 255));

	bool drawIP = true;
	while (1)
	{
		if (wifiStatusFlag == WIFI_STATUS_AP_IN_USE)
		{
			lcd.fillCircle(305, 60, 12, RGB_TO_565(0, 255, 0));
			drawIP = true;
		}
		else
		{
			lcd.fillCircle(305, 60, 12, RGB_TO_565(0, 0, 0));
		}
		if (wifiStatusFlag == WIFI_STATUS_TRYING_TO_CONNECT_TO_AP)
		{
			lcd.fillCircle(305, 20, 12, RGB_TO_565(255, 0, 0));
		}
		else if(wifiStatusFlag == WIFI_STATUS_CONNECTED_TO_AP)
		{
			lcd.fillCircle(305, 20, 12, RGB_TO_565(0, 255, 0));
			if (drawIP)
			{
				drawIP = false;
				lcd.setCursor(0, 130);
				lcd.printf("STA iP: %s", WiFi.localIP().toString().c_str());
			}
		}
		else
		{
			lcd.fillCircle(305, 20, 12, RGB_TO_565(0, 0, 0));
		}
		// Handle Virtual IOs
		lcd.fillCircle(80, 200, 25, RGB_TO_565(255, 0, 0) * IO[0]);
		lcd.fillCircle(160, 200, 25, RGB_TO_565(0, 255, 0) * IO[1]);
		lcd.fillCircle(240, 200, 25, RGB_TO_565(0, 0, 255) * IO[2]);
		delay(1000);
	}
}

#define CHECK_AND_UPDATE_FIELD(WEB_VAL,USER_VAL)\
if (request->hasParam(WEB_VAL, true))\
{\
	if (!USER_VAL)\
	{\
		USER_VAL = true;\
	}\
}\
else\
{\
	if (USER_VAL != false)\
	{\
		USER_VAL = false;\
	}\
}

#define SEND_MAIN_PAGE_HTML() if(webMainPageDataPointer) request->send_P(200, "text/html", webMainPageDataPointer, webMainPageDataSize);\
							  else request->send(SPIFFS, "/mainPage.html") 

// Handle inputs from web pages
void action(AsyncWebServerRequest* request)
{
	printf("ACTION!\n");
	int params = request->params();
	for (int i = 0; i < params; i++)
	{
		AsyncWebParameter* p = request->getParam(i);
		Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
	}
	if (wifiSetupPageVisited)
	{
		wifiSetupPageVisited = false;
		if (spiffsOK && request->hasParam("APpassword", true))
		{
			File file;
			//Disbale Interrupts
			file = SPIFFS.open("/wifi.txt", "w");
			if (file)
			{
				printf("Saving WiFi.txt\n");
				CHECK_AND_UPDATE_FIELD("useWiFi", useHotSpot);
				if (useHotSpot)
				{
					uint8_t buff[3] = { '1','\n' };
					file.write(buff, 2);
				}
				else
				{
					uint8_t buff[3] = { '0','\n' };
					file.write(buff, 2);
				}
				file.write((const uint8_t*)request->getParam("APpassword", true)->value().c_str(),
					request->getParam("APpassword", true)->value().length());
				file.write('\n');
				APpassword = request->getParam("APpassword", true)->value();
				file.write((const uint8_t*)request->getParam("AccessPoint", true)->value().c_str(),
					request->getParam("AccessPoint", true)->value().length());
				file.write('\n');
				HotSpotName = request->getParam("AccessPoint", true)->value();
				file.write((const uint8_t*)request->getParam("Password", true)->value().c_str(),
					request->getParam("Password", true)->value().length());
				file.write('\n');
				HotSpotPass = request->getParam("Password", true)->value();
				file.close();
			}
		}
	}
	SEND_MAIN_PAGE_HTML();
}
void mainPage(AsyncWebServerRequest* request)
{
	webMenuPressed = false;
	webPlusPressed = false;
	webMinusPressed = false;
	SEND_MAIN_PAGE_HTML();
}
#if 1
const static char HTMLcommonHeader[]PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head><meta charset="utf-8" name="viewport" content="width=device-width, initial-scale=1.0, user-scalable=no">
<style>
html {font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center; font-size: 35px}
.button {
  background-color: #4CAF50; /* Green */
  border: none;
  color: white;
  padding: 15px 32px;
  text-align: center;
  text-decoration: none;
  display: inline-block;
  font-size: 25px;
  margin: 0px 5px;
  cursor: pointer;
}
.OKbutton {background-color: #f4CAF50; border-radius: 50px;box-shadow: 0px 0px 5px 5px rgb(20,20,20);font-size: 40px;} 
{display: inline-block;  font-size: 25px;  margin: 4px 2px;  cursor: pointer; -webkit-border-radius: 18px;}
.boxStyle { padding: 12px 1px;  margin: 8px 0;  box-sizing: border-box;  border: 2px solid red;  border-radius: 10px; font-size: 32px;text-align: center;}
.largeCheckBox {width:50px; height:50px;}
</style>
<body>
<form action='action' method="POST">
)rawliteral";

const static char getWebPageHTMLpart1[]PROGMEM = R"rawliteral(
<br>Access Point Name:<br>
<input type="text" class="boxStyle" name="AccessPoint" value="
)rawliteral";

const static char getWebPageHTMLpart2[]PROGMEM = R"rawliteral(
">
<br>
Password:
<br>
<input type="password" class="boxStyle" name="Password" value="
)rawliteral";

const static char getWebPageHTMLpart3[]PROGMEM = R"rawliteral(
">
<br>
AP Password:
<br>
<input type="text" class="boxStyle" name="APpassword" required minlength="6" value="
)rawliteral";

const static char getWebPageHTMLpart4[]PROGMEM = R"rawliteral(
">
<br>
<input type="checkbox" class="largeCheckBox" name="useWiFi" value="useWiFi" 
)rawliteral";

const static char getWebPageHTMLpart5[]PROGMEM = R"rawliteral(
> Use WiFi <br>
<p><button type = "submit" class = "button OKbutton" href = "/">OK</button></p>
</form>
<href="/">
</html>
)rawliteral";
#endif
void setupWiFiPage(AsyncWebServerRequest* request)
{
	String webPage;

	webPage = HTMLcommonHeader;
	webPage += getWebPageHTMLpart1;
	webPage += HotSpotName.c_str();
	webPage += getWebPageHTMLpart2;
	webPage += HotSpotPass.c_str();
	webPage += getWebPageHTMLpart3;
	webPage += APpassword.c_str();
	webPage += getWebPageHTMLpart4;

	if (useHotSpot)
		webPage += "checked";
	else
		webPage += "";

	webPage += getWebPageHTMLpart5;

	request->send(200, "text/html", webPage);
	wifiSetupPageVisited = true;
}
#if 1
const static char HTMLclockSetup[]PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head><meta charset="utf-8" name="viewport" content="width=device-width, initial-scale=1.0, user-scalable=no">
<title>Clock Update</title>
<style>
html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center; font-size: 30px}
.button {
  background-color: #4CAF50; /* Green */
  border: none;
  color: white;
  padding: 15px 32px;
  text-align: center;
  text-decoration: none;
  display: inline-block;
  font-size: 25px;
  margin: 0px 5px;
  cursor: pointer;
}
.OKbutton {background-color: #f4CAF50; border-radius: 40px;box-shadow: 0px 0px 10px 10px rgb(20,20,20);font-size: 35px;} .boxStyle {  padding: 6px 6px;  margin: 8px 0;  box-sizing: border-box;  border: 2px solid red;  border-radius: 10px; font-size: 20px;text-align: center;}
.boxStyle2 {  padding: 5px 5px; width: 60px; margin: 8px 0;  box-sizing: border-box;  border: 2px solid red;  border-radius: 10px; font-size: 25px;text-align: center;}
p {font-size: 30px;color: #000;margin-bottom: 0px}
.largeCheckBox {width:25px; height:30px;border-radius: 10px;}
</style>
<head>
<body>
<h3>כוון שעה</h3>
</head>
<form action='action' method="POST">
<p>תאריך</p>
<input class="boxStyle" type="date" name="date" id="dateInput">
<p>שעה</p>
<input class="boxStyle2" type="number" step="1" min="0" max="23" name="hour" id="timeInputH">
:
<input class="boxStyle2" type="number" step="1" min="0" max="59" name="min" id="timeInputM">
:
<input class="boxStyle2" type="number" step="1" min="0" max="59" name="sec" id="timeInputS">
<br>
<br>
<script type="text/javascript">
var today = new Date();
var todaysDate;
var dd = today.getDate();
var mm = today.getMonth() + 1;
var yyyy = today.getFullYear();
var hour = today.getHours();
if(hour<10) { hour = '0'+ hour }; 
var min = today.getMinutes();
if(min<10) { min = '0'+ min }; 
var sec = today.getSeconds();
if(sec<10) { sec = '0'+ sec }; 
if(dd<10) { dd = '0'+ dd }; 
if(mm<10) { mm = '0'+ mm }; 
todaysDate = yyyy + '-' + mm + '-' + dd;
document.getElementById("timeInputH").value = hour;
document.getElementById("timeInputM").value = min;
document.getElementById("timeInputS").value = sec;
document.getElementById("dateInput").value = todaysDate;
</script>
<p><button type="submit" class="button OKbutton" href="/">עדכן</button></p>
<br>
<a class="button OKbutton" href="/updateTimeFromWeb">עדכן מהרשת</a>
<br>
)rawliteral";
#endif

void handleWebMenuPress(AsyncWebServerRequest* request)
{
	webMenuPressed = true;
	webPlusPressed = false;
	webMinusPressed = false;
	SEND_MAIN_PAGE_HTML();
}
void handleWebPlusPress(AsyncWebServerRequest* request)
{
	webMenuPressed = false;
	webPlusPressed = true;
	webMinusPressed = false;
	SEND_MAIN_PAGE_HTML();
}
void handleWebMinusPress(AsyncWebServerRequest* request)
{
	webMenuPressed = false;
	webPlusPressed = false;
	webMinusPressed = true;
	SEND_MAIN_PAGE_HTML();
}

void webServerSetup()
{
	server.on("/action", HTTP_POST, action);
	server.on("/", HTTP_GET, mainPage);
	server.on("/wifiSetupSelected", HTTP_GET, setupWiFiPage);
	server.on("/menuPressed", HTTP_GET, handleWebMenuPress);
	server.on("/plusPressed", HTTP_GET, handleWebPlusPress);
	server.on("/minusPressed", HTTP_GET, handleWebMinusPress);
	server.on("/wifiTempHumiSelected", HTTP_GET, handleTempHumiPage);
	server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest* request) {
		request->send_P(200, "text/plain", String(readDHTTemperature()).c_str());
		});
	server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest* request) {
		request->send_P(200, "text/plain", String(readDHTHumidity()).c_str());
		});
	server.on("/smartTimingImg", HTTP_GET, [](AsyncWebServerRequest* request)
		{
			request->send(SPIFFS, "/smartTimingImg.png", "image/png");
		});
	server.on("/wifiImg", HTTP_GET, [](AsyncWebServerRequest* request)
		{
			request->send(SPIFFS, "/wifiImg.png", "image/png");
		});
	// Handle IOs - handleIOsPage
	server.on("/ioPageSelected", HTTP_GET, handleIOsPage);

	server.on("/update", HTTP_GET, [](AsyncWebServerRequest* request) {
		String inputMessage1;
		String inputMessage2;
		// GET input1 value on <ESP_IP>/update?output=<inputMessage1>&state=<inputMessage2>
		if (request->hasParam("output") && request->hasParam("state")) 
		{
			inputMessage1 = request->getParam("output")->value();
			inputMessage2 = request->getParam("state")->value();
			if (inputMessage1.toInt() < 3)
			{
				IO[inputMessage1.toInt()] = inputMessage2.toInt();
			}
		}
		else 
		{
			inputMessage1 = "No message sent";
			inputMessage2 = "No message sent";
		}
		//printf("inputMessage1 = %s\n", inputMessage1.c_str());
		//printf("inputMessage2 = %s\n", inputMessage2.c_str());
		request->send(200, "text/plain", "OK");
		});
}

float readDHTTemperature() 
{
	float t = analogRead(WHEEL_PIN) / 100.0;//dht.readTemperature();
	return t;
}

float readDHTHumidity() 
{
  float h = analogRead(WHEEL_PIN) / 100.0;//dht.readHumidity();
  return h;
}

const char tempHumi_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css" integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr" crossorigin="anonymous">
  <style>
    html {
     font-family: Arial;
     display: inline-block;
     margin: 0px auto;
     text-align: center;
    }
    h2 { font-size: 3.0rem; }
    p { font-size: 3.0rem; }
    .units { font-size: 1.2rem; }
    .dht-labels{
      font-size: 1.5rem;
      vertical-align:middle;
      padding-bottom: 15px;
    }
  </style>
</head>
<body>
  <h2>ESP32 DHT Server</h2>
  <p>
    <i class="fas fa-thermometer-half" style="color:#059e8a;"></i> 
    <span class="dht-labels">Temperature</span> 
    <span id="temperature">%TEMPERATURE%</span>
    <sup class="units">&deg;C</sup>
  </p>
  <p>
    <i class="fas fa-tint" style="color:#00add6;"></i> 
    <span class="dht-labels">Humidity</span>
    <span id="humidity">%HUMIDITY%</span>
    <sup class="units">&percnt;</sup>
  </p>
</body>
<script>
setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("temperature").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/temperature", true);
  xhttp.send();
}, 10000 ) ;

setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("humidity").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/humidity", true);
  xhttp.send();
}, 10000 ) ;
</script>
</html>)rawliteral";

// Replaces placeholder with DHT values

String processor(const String& var)
{
  if(var == "TEMPERATURE")
  {
	  printf("var = temp\n");
	  return String(readDHTTemperature());
  }
  if(var == "HUMIDITY")
  {
		return String(readDHTHumidity());
  }
  if (var == "BUTTON0")
  {
	  if (IO[0])
	  {
		  return String("checked");
	  }
	  else
	  {
		  return String("");
	  }
  }
  if (var == "BUTTON1")
  {
	  if (IO[1])
	  {
		  return String("checked");
	  }
	  else
	  {
		  return String("");
	  }
  }
  if (var == "BUTTON2")
  {
	  if (IO[2])
	  {
		  return String("checked");
	  }
	  else
	  {
		  return String("");
	  }
  }

	return String();
}

void handleTempHumiPage(AsyncWebServerRequest* request)
{
	request->send_P(200, "text/html", tempHumi_html, processor);
}

const char IO_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>ESP Web Server</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="icon" href="data:,">
  <style>
    html {font-family: Arial; display: inline-block; text-align: center;}
    h2 {font-size: 3.0rem;}
    p {font-size: 3.0rem;}
    body {max-width: 600px; margin:0px auto; padding-bottom: 25px;}
    .switch {position: relative; display: inline-block; width: 120px; height: 68px} 
    .switch input {display: none}
    .slider {position: absolute; top: 0; left: 0; right: 0; bottom: 0; background-color: #ccc; border-radius: 6px}
    .slider:before {position: absolute; content: ""; height: 52px; width: 52px; left: 8px; bottom: 8px; background-color: #fff; -webkit-transition: .4s; transition: .4s; border-radius: 3px}
    input:checked+.slider {background-color: #b30000}
    input:checked+.slider:before {-webkit-transform: translateX(52px); -ms-transform: translateX(52px); transform: translateX(52px)}
  </style>
</head>
<body>
  <h2>ESP Web Server</h2>
  <h4>Output - GPIO 0</h4><label class="switch"><input type="checkbox" onchange="toggleCheckbox(this)" id="0" " + %BUTTON0% + "><span class="slider"></span></label>
  <h4>Output - GPIO 1</h4><label class="switch"><input type="checkbox" onchange="toggleCheckbox(this)" id="1" " + %BUTTON1% + "><span class="slider"></span></label>
  <h4>Output - GPIO 2</h4><label class="switch"><input type="checkbox" onchange="toggleCheckbox(this)" id="2" " + %BUTTON2% + "><span class="slider"></span></label>
<script>function toggleCheckbox(element) {
  var xhr = new XMLHttpRequest();
  if(element.checked){ xhr.open("GET", "/update?output="+element.id+"&state=1", true); }
  else { xhr.open("GET", "/update?output="+element.id+"&state=0", true); }
  xhr.send();
}
</script>
</body>
</html>
)rawliteral";

void handleIOsPage(AsyncWebServerRequest* request)
{
	request->send_P(200, "text/html", IO_html, processor);
}

// Replaces placeholder with button section in your web page

String outputState(int output) 
{
	if (IO[output]) 
	{
		String ioStr = "checked";
		return ioStr;
	}
	else 
	{
		String ioStr = "";
		return ioStr;
	}
}

