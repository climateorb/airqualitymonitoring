#include <Wire.h>
#include <TimeLib.h>
#include <SDS011.h>
#include "SSD1306Wire.h"
#include "OLEDDisplayUi.h"
#include "images.h"

#define RXD1 13
#define TXD1 12

#define RXD2 15
#define TXD2 14

SSD1306Wire  display(0x3c, 5, 4);
OLEDDisplayUi ui     ( &display );

HardwareSerial SerialSDS(1);
SDS011 sdsAQM;

HardwareSerial SerialBME680(2);

float p10, p25;
int error;
String quality;
unsigned long mBME680Interval = 4000;
//unsigned long mSDS011Runtime = 40000;
//unsigned long mSDS011Interval = 120000;


uint16_t temp1=0;
int16_t temp2=0;
unsigned char Re_buf[30],counter=0;
unsigned char sign=0;

int screenW = 128;
int screenH = 64;
int clockCenterX = screenW/2;
int clockCenterY = ((screenH-16)/2)+16;

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
  display->drawXbm(x + 34, y, WiFi_Logo_width, WiFi_Logo_height, WiFi_Logo_bits);
}

void drawFrame2(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(ArialMT_Plain_16);
  display->drawString(0 + x, 10 + y, "PM 2.5");

  display->setFont(ArialMT_Plain_24);
  display->drawString(0 + x, 34 + y, " "+String((int) p25)+" PPM");
}

void drawFrame3(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(ArialMT_Plain_16);
  display->drawString(0 + x, 10 + y, "PM 10");

  display->setFont(ArialMT_Plain_24);
  display->drawString(0 + x, 34 + y, " "+String((int) p10)+" PPM");
}

void drawFrame4(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
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

FrameCallback frames[] = { drawFrame1, drawFrame2, drawFrame3, drawFrame4, drawFrame5 };

int frameCount = 5;

OverlayCallback overlays[] = { msOverlay };
int overlaysCount = 1;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("SerialSDS TX is on pin: "+String(TXD1));
  Serial.println("SerialSDS RX is on pin: "+String(RXD1));
  sdsAQM.begin(&SerialSDS, RXD1, TXD1);

//  sdsAQM.sleep();
//  delay(1000);
  
  Serial.println("SerialBME680 TX is on pin: "+String(TXD2));
  Serial.println("SerialBME680 RX is on pin: "+String(RXD2));
  SerialBME680.begin(9600, SERIAL_8N1, RXD2, TXD2);
  delay(1000);
  
  SerialBME680.peek();  
  delay(2000); 
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


void loop() {
  float Temperature ,Humidity;
  unsigned char i=0,sum=0;
  uint32_t Gas;
  uint32_t Pressure;
  uint16_t IAQ;
  int16_t  Altitude;
  uint8_t IAQ_accuracy;

  int remainingTimeBudget = ui.update();

  // Used to remember the time of the last BME680 measurement.
  static unsigned long previousBME680Reading = 0;
  unsigned long currentMillis = millis();
  
  if (currentMillis - previousBME680Reading >= mBME680Interval) {
    // Get Sensor Data
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
      if(Re_buf[0]==0x5A&&Re_buf[1]==0x5A ) {
        for(i=0;i<19;i++) {
          sum+=Re_buf[i];
        }
        if(sum==Re_buf[i]) {
          temp2=(Re_buf[4]<<8|Re_buf[5]);
          Temperature=(float)temp2/100;
          temp1=(Re_buf[6]<<8|Re_buf[7]);
          Humidity=(float)temp1/100;
          Pressure=((uint32_t)Re_buf[8]<<16)|((uint16_t)Re_buf[9]<<8)|Re_buf[10];
          IAQ_accuracy= (Re_buf[11]&0xf0)>>4;
          IAQ=((Re_buf[11]&0x0F)<<8)|Re_buf[12];
          Gas=((uint32_t)Re_buf[13]<<24)|((uint32_t)Re_buf[14]<<16)|((uint16_t)Re_buf[15]<<8)|Re_buf[16];
          Altitude=(Re_buf[17]<<8)|Re_buf[18];
          Serial.print("Temperature:");
          Serial.print(Temperature);
          Serial.print(" ,Humidity:");
          Serial.print(Humidity);
          Serial.print(" ,Pressure:");
          Serial.print(Pressure);
          Serial.print("  ,IAQ:");
          Serial.print(IAQ);
          Serial.print(" ,Gas:");
          Serial.print(Gas );
          Serial.print("  ,Altitude:");
          Serial.print(Altitude);
          Serial.print("  ,IAQ_accuracy:");
          Serial.println(IAQ_accuracy);
        }
      }
    }
    previousBME680Reading = currentMillis;
  }

  error = sdsAQM.read(&p25, &p10);
  if (!error) {
    Serial.print("P2.5: " + String(p25) + ", ");
    Serial.println("P10:  " + String(p10));
  }

  if ((int) p25<=25 || (int) p10<=50) {
    quality = "Optimal";
  } else if (((int) p25>25 && (int) p25<50) || ((int) p10>50 && (int) p10<100)) {
    quality = "Suffocating";
  } else if (((int) p25>=50 || (int) p10>=100)) {
    quality = "Dangerous";
  }
  
  if (remainingTimeBudget > 0) {
    delay(remainingTimeBudget);
  }
}
