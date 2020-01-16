#include <Wire.h>
#include <rgb_lcd.h>
#include <ESP8266WiFi.h>
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
   #include<avr/power.h>
#endif
#define LED D4 //Output-Port to which the NeoPixels are connected
#define NUMPIXELS 30 //number of LEDs in the used NeoPixel-Stripe
#define DELAYVAL 100 //milliseconds betweeen the updates of the single pixels when the RSSI changes
#define DAMP 2 //Damping-factor to decrease brightness of the (usually kind of to bright) NeoPixels

Adafruit_NeoPixel pixels(NUMPIXELS, LED, NEO_RGB + NEO_KHZ800); //constructor for the NeoPixels
rgb_lcd lcd; // constructor for the LCD-Display
const int colorR = 50;
const int colorG = 100;
const int colorB = 255;
/*
prints out a list of available networks, can be used for i.e. debugging, but isn't used in the follwing code:
*/
int printScanResults(int networksFound){
  for (int i = 0; i<networksFound; ++i){
    Serial.printf("%d: %s, Ch:%d (%ddBm) %s\n", i+1, WiFi.SSID(i).c_str(), WiFi.channel(i), WiFi.RSSI(i), WiFi.encryptionType(i) == ENC_TYPE_NONE ? "open" : "");
  }
  return WiFi.RSSI(0);
}
/*takes in values from 0 to 1, returns an unsigned int with the RGB-Colour interpolated between red (val=0)
and green (val=1). The RGB-Values are stored inside the result-variable by bitshifting the char-values by 8, 16 and 24 bits.
 */
unsigned int heatmap(double val){
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
  pinMode(LED, OUTPUT); //set output port for the NeoPixels
  pixels.begin();

  lcd.begin(16, 2); //Setup the LCD-Display (for more detailed description go to https://github.com/Seeed-Studio/Grove_LCD_RGB_Backlight)
  lcd.setRGB(colorR,colorG,colorB);
  lcd.print("strongest WiFi: ");

  //Setup serial port for monitoring (can be omitted if used as "standalone device"):
  Serial.begin(115200);
  Serial.println();
  //Set the WiFi-Mode for the ESP8266WiFi-Class
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
}

void loop() {
  //store count of available networks in numNetworks and store the RSSI of the first in signalStrength:
  int numNetworks = WiFi.scanNetworks();
  int32_t signalStrength = WiFi.RSSI(0);

  //create an array the length of NUMPIXELS, assign a heatmap-colour ranging from red (bottom) to green (top) to indicate signal strength by color:
  unsigned int heat[NUMPIXELS] = {0};
  for (int i = 0 ; i<NUMPIXELS; ++i){
    heat[i] = heatmap(1.0/NUMPIXELS*(double)i);
  }

  //locate the array position of the WiFi with the highest RSSI:
  int NumberSSIDToBePrinted = 0;
  for (int i = 0; i<numNetworks-1; ++i){
    if (WiFi.RSSI(i+1)>signalStrength){
       signalStrength = WiFi.RSSI(i+1);
       NumberSSIDToBePrinted = i+1;
    }
  }
  //Print the SSID of the network with the highest RSSI to the LCD:
  lcd.setCursor(0,1);
  lcd.print(WiFi.SSID(NumberSSIDToBePrinted).c_str());

  //calculate how many pixels the indicator should show (ranging from -80dBm --> 3 pixels from bottom to 35dBm --> all pixels, linearly interpolated):
  int pixelstoshow = 0;
  if (signalStrength<-80) pixelstoshow = 3;
  else if (signalStrength>-30) pixelstoshow = NUMPIXELS;
  else{
    pixelstoshow = ((NUMPIXELS-3)/45)*signalStrength+(3+(80*(NUMPIXELS-3))/45); //Linear interpolation between 3 and NUMPIXELS, -80dBm and -35 dBm
  }

  //Set each pixel with the respective color calculated above and stored in the heat-array
  pixels.clear();
  for (int i = 0; i<pixelstoshow; ++i){
    unsigned char r = heat[i]>>8;
    unsigned char g = heat[i]>>16;
    unsigned char b = heat[i]<<24;
    pixels.setPixelColor(i, pixels.Color(g/DAMP,r/DAMP,b/DAMP));
    pixels.show();
    delay(DELAYVAL);

  }

  //the following block uses the same principles as the above, but only updates the LEDs and the display if there are any changes to what they display
  //without this "update-check" the pixels.clear() method causes a very irritating on/off-stutter when the main loop is repeated.
  int pixelstoshow2 = 0;
  for(int t = 0; t<1000; ++t){
     int numNetworks = WiFi.scanNetworks();
     int32_t signalStrength2 = WiFi.RSSI(0);
     for (int i = 0; i<numNetworks-1; ++i){
       if (WiFi.RSSI(i+1)>signalStrength2)
       signalStrength2 = WiFi.RSSI(i+1);
       NumberSSIDToBePrinted2 = i+1;
     }
     if (signalStrength2<-80) pixelstoshow2 = 3;
     else if (signalStrength>-30) pixelstoshow2 = NUMPIXELS;
     else pixelstoshow2 = ((NUMPIXELS-3)/45)*signalStrength+(3+(80*(NUMPIXELS-3))/45);
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
  if(NumberSSIDToBePrinted!=NumberSSIDToBePrinted2){
    lcd.setCursor(0,1);
    lcd.print(WiFi.SSID(NumberSSIDToBePrinted2).c_str());
  }
  NumberSSIDToBePrinted = NumberSSIDToBePrinted2;
  }



}
