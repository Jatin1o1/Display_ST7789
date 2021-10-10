//#include <SPI.h>
#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI();

#define tft_width_offset 40
#define tft_height_offset 53

#define tft_width 240
#define tft_height 135
/*
#define TFT_MOSI 23  // SDA Pin on ESP32   (DIN pin of LCD )
#define TFT_SCLK 18  // SCL Pin on ESP32   ( CLK pin of LCD)
#define TFT_CS   15  // Chip select control pin
#define TFT_DC    2  // Data Command control pin
#define TFT_RST   4  // Reset pin (could connect to RST pin)
*/


void setup()
{

    Serial.begin(115200);

    // We start by connecting to a WiFi network
    Serial.println();
    Serial.println();
    Serial.println("WiFi Connecting.");

    tft.begin();
    tft.setRotation(1);  // landscape
   tft.fillScreen(TFT_BLACK);
    delay(500);
    tft.fillScreen(TFT_RED);
    delay(500);
    tft.fillScreen(TFT_BLACK);
    //tft.drawRect(40,53,240,135,TFT_GREEN); 
    tft.drawRect(tft_width_offset,tft_height_offset,tft_width,tft_height,TFT_GREEN); 
}

void loop()
{
   
}

