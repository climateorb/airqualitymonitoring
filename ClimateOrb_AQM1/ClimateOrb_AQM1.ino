#include <Wire.h>
#include <TimeLib.h>
#include <SDS011.h>
#include "SSD1306Wire.h"
#include "OLEDDisplayUi.h"
#include "images.h"

#define RXD2 13
#define TXD2 12

SSD1306Wire  display(0x3c, 5, 4);
OLEDDisplayUi ui     ( &display );

HardwareSerial SerialSDS(2);
SDS011 sdsAQM;

float p10, p25;
int error;
String quality;

int screenW = 128;
int screenH = 64;
int clockCenterX = screenW/2;
int clockCenterY = ((screenH-16)/2)+16;

// utility function for digital clock display: prints leading 0
String twoDigits(int digits){
  if(digits < 10) {
    String i = '0'+String(digits);
    return i;
  }
  else {
    return String(digits);
  }
}

void msOverlay(OLEDDisplay *display, OLEDDisplayUiState* state) {
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->setFont(ArialMT_Plain_10);
  display->drawString(128, 0, String(millis()/1000));
}

void drawFrame1(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  // draw an xbm image.
  // Please note that everything that should be transitioned
  // needs to be drawn relative to x and y

  display->drawXbm(x + 34, y, WiFi_Logo_width, WiFi_Logo_height, WiFi_Logo_bits);
}

void drawFrame2(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  // Demonstrates the 3 included default sizes. The fonts come from SSD1306Fonts.h file
  // Besides the default fonts there will be a program to convert TrueType fonts into this format
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(ArialMT_Plain_16);
  display->drawString(0 + x, 10 + y, "PM 2.5");

  display->setFont(ArialMT_Plain_24);
  display->drawString(0 + x, 34 + y, " "+String((int) p25)+" PPM");
}

void drawFrame3(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  // Text alignment demo
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(ArialMT_Plain_16);
  display->drawString(0 + x, 10 + y, "PM 10");

  display->setFont(ArialMT_Plain_24);
  display->drawString(0 + x, 34 + y, " "+String((int) p10)+" PPM");
}

void drawFrame4(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  // Demo for drawStringMaxWidth:
  // with the third parameter you can define the width after which words will be wrapped.
  // Currently only spaces and "-" are allowed for wrapping
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(ArialMT_Plain_16);
  display->drawStringMaxWidth(0 + x, y, 128, "Air Quality is "+quality);
}

void drawFrame5(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  String timenow = String(hour())+":"+twoDigits(minute())+":"+twoDigits(second());
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_24);
  display->drawString(clockCenterX + x , clockCenterY + y, timenow );
}

// This array keeps function pointers to all frames
// frames are the single views that slide in
FrameCallback frames[] = { drawFrame1, drawFrame2, drawFrame3, drawFrame4, drawFrame5 };

// how many frames are there?
int frameCount = 5;

// Overlays are statically drawn on top of a frame eg. a clock
OverlayCallback overlays[] = { msOverlay };
int overlaysCount = 1;

void setup() {
  Serial.begin(115200);
  Serial.println("Serial TX is on pin: "+String(TX));
  Serial.println("Serial RX is on pin: "+String(RX));
  sdsAQM.begin(&SerialSDS, RXD2, TXD2);

  // The ESP is capable of rendering 60fps in 80Mhz mode
  // but that won't give you much time for anything else
  // run it in 160Mhz mode or just set it to 30 fps
  ui.setTargetFPS(60);

  // Customize the active and inactive symbol
  ui.setActiveSymbol(activeSymbol);
  ui.setInactiveSymbol(inactiveSymbol);

  // You can change this to
  // TOP, LEFT, BOTTOM, RIGHT
  ui.setIndicatorPosition(BOTTOM);

  // Defines where the first frame is located in the bar.
  ui.setIndicatorDirection(LEFT_RIGHT);

  // You can change the transition that is used
  // SLIDE_LEFT, SLIDE_RIGHT, SLIDE_UP, SLIDE_DOWN
  ui.setFrameAnimation(SLIDE_LEFT);

  // Add frames
  ui.setFrames(frames, frameCount);

  // Add overlays
  ui.setOverlays(overlays, overlaysCount);

  // Initialising the UI will init the display too.
  ui.init();

  display.flipScreenVertically();
}


void loop() {
  int remainingTimeBudget = ui.update();
  if (remainingTimeBudget > 0) {
    error = sdsAQM.read(&p25, &p10);
    if ((int) p25<=25 || (int) p10<=50) {
      quality = "Optimal";
    } else if (((int) p25>25 && (int) p25<50) || ((int) p10>50 && (int) p10<100)) {
      quality = "Suffocating";
    } else if (((int) p25>=50 || (int) p10>=100)) {
      quality = "Dangerous";
    }
    if (!error) {
      Serial.print("P2.5: " + String(p25) + ", ");
      Serial.println("P10:  " + String(p10));
    }
    delay(remainingTimeBudget);
  }
}
