#include <WiFi.h>
#include <WebServer.h>
#include <AutoConnect.h>
#include <Wire.h>
#include <TimeLib.h>
#include <SDS011.h>
#include <SSD1306Wire.h>
#include <OLEDDisplayUi.h>
#include "FrameImages.h"

WebServer Server;
AutoConnect Portal(Server);
AutoConnectConfig Config;

#define RXD1 13
#define TXD1 12

#define RXD2 15
#define TXD2 14

SSD1306Wire display(0x3c, 5, 4);
OLEDDisplayUi ui(&display);

HardwareSerial SerialSDS(1);
SDS011 sdsAQM;

HardwareSerial SerialBME680(2);

float p10 = 0.0, p25 = 0.0;
int error;
unsigned long mBME680Interval = 4000;

uint16_t temp1 = 0;
int16_t temp2 = 0;
unsigned char Re_buf[30], counter = 0;
unsigned char sign=0;

float Temperature = 0.0, Humidity = 0.0;
unsigned char i = 0;
uint32_t Gas = 0;
uint32_t Pressure = 0;
uint16_t IAQ = 0;
int16_t Altitude = 0;
uint8_t IAQ_accuracy = 0;

int screenW = 128;
int screenH = 64;
int clockCenterX = screenW / 2;
int clockCenterY = ((screenH - 16) / 2) + 16;

String twoDigits(int digits) {
	if (digits < 10) {
		String i = '0' + String(digits);
		return i;
	}
	else {
		return String(digits);
	}
}

void msOverlay(OLEDDisplay *display, OLEDDisplayUiState *state) {
	display->setTextAlignment(TEXT_ALIGN_RIGHT);
	display->setFont(ArialMT_Plain_10);
	display->drawString(128, 0, String(millis() / 1000));
}

void drawFrame1(OLEDDisplay *display, OLEDDisplayUiState *state, int16_t x, int16_t y) {
	display->drawXbm(x + 34, y, ClimateOrb_Logo_width, ClimateOrb_Logo_height, ClimateOrb_Logo_bits);
}

void drawFrame2(OLEDDisplay *display, OLEDDisplayUiState *state, int16_t x, int16_t y) {
	display->setTextAlignment(TEXT_ALIGN_LEFT);
	display->setFont(ArialMT_Plain_16);
	display->drawString(0 + x, 10 + y, "PM 2.5");

	display->setFont(ArialMT_Plain_24);
	display->drawString(0 + x, 34 + y, " " + String((int)p25) + " PPM");
}

void drawFrame3(OLEDDisplay *display, OLEDDisplayUiState *state, int16_t x, int16_t y) {
	display->setTextAlignment(TEXT_ALIGN_LEFT);
	display->setFont(ArialMT_Plain_16);
	display->drawString(0 + x, 10 + y, "PM 10");

	display->setFont(ArialMT_Plain_24);
	display->drawString(0 + x, 34 + y, " " + String((int)p10) + " PPM");
}

void drawFrame4(OLEDDisplay *display, OLEDDisplayUiState *state, int16_t x, int16_t y) {
	display->setTextAlignment(TEXT_ALIGN_LEFT);
	display->setFont(ArialMT_Plain_16);
	display->drawString(0 + x, 10 + y, "Temperature");

	display->setFont(ArialMT_Plain_24);
	display->drawString(0 + x, 34 + y, " " + String((int)Temperature) + " C");
}

void drawFrame5(OLEDDisplay *display, OLEDDisplayUiState *state, int16_t x, int16_t y) {
	display->setTextAlignment(TEXT_ALIGN_LEFT);
	display->setFont(ArialMT_Plain_16);
	display->drawString(0 + x, 10 + y, "Humidity");

	display->setFont(ArialMT_Plain_24);
	display->drawString(0 + x, 34 + y, " " + String((int)Humidity) + " %");
}

void drawFrame6(OLEDDisplay *display, OLEDDisplayUiState *state, int16_t x, int16_t y) {
	display->setTextAlignment(TEXT_ALIGN_LEFT);
	display->setFont(ArialMT_Plain_16);
	display->drawString(0 + x, 10 + y, "Pressure");

	display->setFont(ArialMT_Plain_24);
	display->drawString(0 + x, 34 + y, " " + String((int)(Pressure/100.0)) + " hPa");
}

void drawFrame7(OLEDDisplay *display, OLEDDisplayUiState *state, int16_t x, int16_t y) {
	display->setTextAlignment(TEXT_ALIGN_LEFT);
	display->setFont(ArialMT_Plain_16);
	display->drawString(0 + x, 10 + y, "TVOC");

	display->setFont(ArialMT_Plain_24);
	display->drawString(0 + x, 34 + y, " " + String((int)Gas) + " Ohms");
}

void drawFrame8(OLEDDisplay *display, OLEDDisplayUiState *state, int16_t x, int16_t y) {
	display->setTextAlignment(TEXT_ALIGN_LEFT);
	display->setFont(ArialMT_Plain_16);
	display->drawString(0 + x, 10 + y, "IAQ");

	display->setFont(ArialMT_Plain_24);
	display->drawString(0 + x, 34 + y, " " + String((int)IAQ));
}

void drawFrame9(OLEDDisplay *display, OLEDDisplayUiState *state, int16_t x, int16_t y) {
	display->setTextAlignment(TEXT_ALIGN_LEFT);
	display->setFont(ArialMT_Plain_16);
	display->drawString(0 + x, 10 + y, "Altitude");

	display->setFont(ArialMT_Plain_24);
	display->drawString(0 + x, 34 + y, " " + String((int)Altitude) + " Meters");
}

void drawFrame10(OLEDDisplay *display, OLEDDisplayUiState *state, int16_t x, int16_t y) {
	display->setTextAlignment(TEXT_ALIGN_CENTER);
	display->setFont(ArialMT_Plain_16);
	display->drawString(0 + x, 10 + y, "Sensor Status: " + String(IAQ_accuracy));

	String timenow = String(hour()) + ":" + twoDigits(minute()) + ":" + twoDigits(second());
	display->setFont(ArialMT_Plain_24);
	display->drawString(clockCenterX + x, clockCenterY + y, timenow);
}

FrameCallback frames[] = {drawFrame1, drawFrame2, drawFrame3, drawFrame4, drawFrame5, drawFrame6, drawFrame7, drawFrame8, drawFrame9, drawFrame10};

int frameCount = 10;

OverlayCallback overlays[] = {msOverlay};
int overlaysCount = 1;

//bool startCP(IPAddress ip) {
//	Serial.println("Captive Portal started! IP:" + WiFi.localIP().toString());
//	return true;
//}

void rootPage() {
	char content[] = "Connection Successful!";
	Server.send(200, "text/plain", content);
}

//const char *host = "api.climateorb.com";

void setup() {
	Serial.begin(115200);
	Config.title = "ClimateOrb";
	Config.apid = "Orb-" + String((uint32_t)(ESP.getEfuseMac() >> 32), HEX);
	Config.psk = "climateorb";
	Portal.config(Config);
	//Portal.onDetect(startCP);
	Server.on("/", rootPage);
	if (Portal.begin()) {
		Serial.println("WiFi connected: " + WiFi.localIP().toString());
	}
	delay(1000);
	Serial.println("SerialSDS TX is on pin: " + String(TXD1));
	Serial.println("SerialSDS RX is on pin: " + String(RXD1));
	sdsAQM.begin(&SerialSDS, RXD1, TXD1);

	Serial.println("SerialBME680 TX is on pin: " + String(TXD2));
	Serial.println("SerialBME680 RX is on pin: " + String(RXD2));
	SerialBME680.begin(9600, SERIAL_8N1, RXD2, TXD2);
	delay(1000);

	SerialBME680.write(0XA5);
	SerialBME680.write(0X55);
	SerialBME680.write(0X3F);
	SerialBME680.write(0X39);
	delay(200);

	SerialBME680.write(0XA5);
	SerialBME680.write(0X56);
	SerialBME680.write(0X02);
	SerialBME680.write(0XFD);
	delay(300);

	ui.setTargetFPS(60);
	ui.setActiveSymbol(activeSymbol);
	ui.setInactiveSymbol(inactiveSymbol);
	ui.setIndicatorPosition(BOTTOM);
	ui.setIndicatorDirection(LEFT_RIGHT);
	ui.setFrameAnimation(SLIDE_LEFT);
	ui.setFrames(frames, frameCount);
	ui.setOverlays(overlays, overlaysCount);
	ui.init();

	display.flipScreenVertically();
}

void loop()
{
	//Update the Frames
	int remainingTimeBudget = ui.update();

	if (remainingTimeBudget > 0) {
		// Get Sensor Data - BME680 over UART
		while (SerialBME680.available()) {
			Re_buf[counter] = (unsigned char) SerialBME680.read();
			if(counter==0&&Re_buf[0]!=0x5A) return;
			if(counter==1&&Re_buf[1]!=0x5A) {
				counter=0;
				return;
			};           
			counter++;       
			if(counter==20) {
				counter=0;
				sign=1;
			}
		}
		
		if(sign) {
			sign=0;
			if(Re_buf[0]==0x5A&&Re_buf[1]==0x5A) {
				temp2=(Re_buf[4]<<8|Re_buf[5]);
				Temperature=(float)temp2/100;
				temp1=(Re_buf[6]<<8|Re_buf[7]);
				Humidity=(float)temp1/100;
				Pressure=((uint32_t)Re_buf[8]<<16)|((uint16_t)Re_buf[9]<<8)|Re_buf[10];
				IAQ_accuracy= (Re_buf[11]&0xf0)>>4;
				IAQ=((Re_buf[11]&0x0F)<<8)|Re_buf[12];
				Gas=((uint32_t)Re_buf[13]<<24)|((uint32_t)Re_buf[14]<<16)|((uint16_t)Re_buf[15]<<8)|Re_buf[16];
				Altitude=(Re_buf[17]<<8)|Re_buf[18];
				Serial.print("Temperature:" + String(Temperature) + ", Humidity:" + String(Humidity) + ", Pressure:" + String(Pressure));
				Serial.println(", IAQ:"+ String(IAQ) + ", Gas:" + String(Gas) + ", Altitude:" + String(Altitude) + ", IAQ_accuracy:" + String(IAQ_accuracy));
			}
		}

		// Get Sensor Data - SDS011 over UART
		error = sdsAQM.read(&p25, &p10);
		if (!error)
		{
			Serial.print("P2.5: " + String(p25) + ", ");
			Serial.println("P10:  " + String(p10));
		}
		delay(remainingTimeBudget);
	}

	Portal.handleClient();
}
