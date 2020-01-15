#include <Wire.h>
#include <rgb_lcd.h>
#include <ESP8266WiFi.h>
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
   #include<avr/power.h>
#endif
#define LED D4
#define NUMPIXELS 30
#define DELAYVAL 100 //milliseconds
#define DAMP 2 //Dämpfungsfaktor für die Helligkeit

Adafruit_NeoPixel pixels(NUMPIXELS, LED, NEO_RGB + NEO_KHZ800);
rgb_lcd lcd;
const int colorR = 50;
const int colorG = 100;
const int colorB = 255;

int prinScanResults(int networksFound){
  for (int i = 0; i<networksFound; ++i){
    Serial.printf("%d: %s, Ch:%d (%ddBm) %s\n", i+1, WiFi.SSID(i).c_str(), WiFi.channel(i), WiFi.RSSI(i), WiFi.encryptionType(i) == ENC_TYPE_NONE ? "open" : "");
  }
  return WiFi.RSSI(0);
}

unsigned int heatmap(double val){ //takes in values from 0 to 1 --> fleißkomma, zur Sicherheit nur max. 3 Nachkommastellen
  unsigned char r = 0U;
  unsigned char g = 0U;
  unsigned char b = 0U;
  unsigned int result = 0U;
  r = (unsigned char)((1 - val)*255U);
  g = (unsigned char)(val*255U);
  result = (r << 8) + (g << 16) + (b << 24);

  return result;
}


void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  pinMode(LED, OUTPUT);
  pixels.begin();

  lcd.begin(16, 2);
  lcd.setRGB(colorR,colorG,colorB);
  lcd.print("strongest WiFi: ");
  
  Serial.begin(115200);
  Serial.println();

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
}

void loop() {
  int numNetworks = WiFi.scanNetworks();
  int32_t signalStrength = WiFi.RSSI(0);
  unsigned int heat[NUMPIXELS] = {0};
  for (int i = 0 ; i<NUMPIXELS; ++i){
    heat[i] = heatmap(1.0/30*(double)i);
    //Serial.printf("%lf\n",1.0/30*(double)i);
  }
  int NumberSSIDToBePrinted = 0; 
  for (int i = 0; i<numNetworks-1; ++i){
    if (WiFi.RSSI(i+1)>signalStrength){
       signalStrength = WiFi.RSSI(i+1);
       NumberSSIDToBePrinted = i+1;
    }
  }
  lcd.setCursor(0,1);
  lcd.print(WiFi.SSID(NumberSSIDToBePrinted).c_str());
  int pixelstoshow = 0;
  if (signalStrength<-80) pixelstoshow = 3;
  else if (signalStrength>-30) pixelstoshow = NUMPIXELS;
  else{
    pixelstoshow = -(30.0/50)*((-signalStrength)+20)+63;
  }
  Serial.printf("%d\n", pixelstoshow);
  pixels.clear();
  for (int i = 0; i<pixelstoshow; ++i){
    unsigned char r = heat[i]>>8;
    unsigned char g = heat[i]>>16;
    unsigned char b = heat[i]<<24;
    //Serial.printf("%d, %d, %d\n", r, g, b);
    pixels.setPixelColor(i, pixels.Color(g/DAMP,r/DAMP,b/DAMP));
    pixels.show();
    delay(DELAYVAL);
   
  }
  int pixelstoshow2 = 0;
  for(int t = 0; t<1000; ++t){
     int numNetworks = WiFi.scanNetworks();
     int32_t signalStrength2 = WiFi.RSSI(0);
     for (int i = 0; i<numNetworks-1; ++i){
       if (WiFi.RSSI(i+1)>signalStrength2)
       signalStrength2 = WiFi.RSSI(i+1);
     }
     if (signalStrength2<-80) pixelstoshow2 = 3;
     else if (signalStrength>-30) pixelstoshow2 = NUMPIXELS;
     else pixelstoshow2 = -(30.0/50)*((-signalStrength2)+20)+63;
   Serial.printf("%d, %d\n", pixelstoshow, pixelstoshow2);
   if(pixelstoshow2<pixelstoshow){
     for (int i = pixelstoshow-1; i>pixelstoshow2-1; --i){
       pixels.setPixelColor(i, pixels.Color(0,0,0));
       pixels.show();
       delay(DELAYVAL);
     }
   pixelstoshow = pixelstoshow2;
   }
   else if(pixelstoshow2>pixelstoshow){
      for (int i = pixelstoshow-1; i<pixelstoshow2-1; ++i){
        unsigned char r = heat[i]>>8;
        unsigned char g = heat[i]>>16;
        unsigned char b = heat[i]<<24;
        pixels.setPixelColor(i, pixels.Color(g/DAMP,r/DAMP,b/DAMP));
        pixels.show();
        delay(DELAYVAL);
  }
  pixelstoshow = pixelstoshow2;
  }
  }
  
  

}
