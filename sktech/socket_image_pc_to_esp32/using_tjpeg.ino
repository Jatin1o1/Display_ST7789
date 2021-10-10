
#include <TFT_eSPI.h>

#include <WiFi.h>
#include <TJpg_Decoder.h>


TFT_eSPI tft = TFT_eSPI();


/* pin configuration 
#define TFT_MOSI 23  // SDA Pin on ESP32   (DIN pin of LCD )
#define TFT_SCLK 18  // SCL Pin on ESP32   ( CLK pin of LCD)
#define TFT_CS   15  // Chip select control pin
#define TFT_DC    2  // Data Command control pin
#define TFT_RST   4  // Reset pin (could connect to RST pin)
*/

TaskHandle_t Task1;
//const uint8_t* arrayName;           // Name of FLASH array containing Jpeg
bool doDecoding = false;            // Mutex flag to start decoding
bool mcuReady = false;              // Mutex flag to indicate an MCU block is ready for rendering
uint16_t mcuBuffer[16*16];          // Buffer to grab a snapshot of decoded MCU block
int32_t mcu_x, mcu_y, mcu_w, mcu_h; // Snapshot of the place to render the MCU


#define MAX_IMAGE_SIZE 65535   // 64KB maximum image size to be received
uint16_t last_image_size = 0;

#define minimum(a,b)     (((a) < (b)) ? (a) : (b))


uint8_t arrayName[MAX_IMAGE_SIZE]  = { NULL };

#define WIFI_SSID "LeCasa 2nd Floor 2"
#define WIFI_PASSWORD "LeCasa@1575"

const uint16_t port = 8090;
const char * host = "192.168.31.240";

#define tft_width_offset 40
#define tft_height_offset 53

#define tft_width 240
#define tft_height 135

bool mcu_decoded(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap)
{
   // Stop further decoding as image is running off bottom of screen
  if ( y >= tft.height() ) return 0;

  while(mcuReady) yield(); // Wait here if rendering of last MCU block to TFT is still in progress

  memcpy(mcuBuffer, bitmap, 16*16*2); // Grab a copy of the MCU block image
  mcu_x = x;                          // Grab postion and size of MCU block
  mcu_y = y;
  mcu_w = w;
  mcu_h = h;
  mcuReady = true; // Flag to tell processor 1 that rendering of MCU can start

  // Return 1 to decode next Jpeg MCU block
  return 1;
}

// This is the task that runs on processor 0 (Arduino sketch runs on processor 1)
// It decodes the Jpeg image
void decodeJpg(void* p) {
  // This is an infinite loop, effectively the same as the normal sketch loop()
  // but this function and loop is running on processor 0
  for(;;) {
    // Decode the Jpeg image
    if (doDecoding) { // Only start decoding if main sketch sets this flag
      TJpgDec.drawJpg(0, 0, arrayName, sizeof(arrayName)); // Runs until complete image decoded
      doDecoding = false; // Set mutex false to indicate decoding has ended
    }
    // Must yield in this loop
    yield();
  }
}


void setup() {
    Serial.begin(115200);

    // We start by connecting to a WiFi network
    Serial.println("WiFi Connecting.");

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println(WiFi.localIP());
    

    //server.begin();

    tft.begin();
    tft.setRotation(1);  // landscape
    tft.fillScreen(TFT_BLACK);
    delay(500);
    tft.fillScreen(TFT_RED);
    delay(500);
    tft.fillScreen(TFT_BLACK);


    xTaskCreatePinnedToCore(decodeJpg, "decodeJpg", 10000, NULL, 0, NULL, 0);

    // The jpeg image can be scaled by a factor of 1, 2, 4, or 8
    TJpgDec.setJpgScale(1);

    // The byte order can be swapped (set true for TFT_eSPI)
    TJpgDec.setSwapBytes(true);

    // The decoder must be given the exact name of the mcu buffer function above
    TJpgDec.setCallback(mcu_decoded);

}

void loop() {


    WiFiClient client;

    if (!client.connect(host, port)) {
        Serial.println("Connection to host failed");
        tft.fillScreen(TFT_BLACK);
        delay(500);
        tft.drawRect(tft_width_offset,tft_height_offset,tft_width,tft_height,TFT_GREEN); 
        delay(500);
        return;
    }

  // Get the width and height in pixels of the jpeg if you wish
    uint16_t w = 0, h = 0;
    
    // Time recorded for test purposes
    uint32_t t = millis();

    Serial.println("Connected to server successful!");
    client.print("S");
    getImage(client); // receive image from server 
    Serial.println("got image 1 ");

    TJpgDec.getJpgSize(&w, &h, arrayName, sizeof(arrayName));
    Serial.print("Width = "); Serial.print(w); Serial.print(", height = "); Serial.println(h);

      // The order here is important, doDecoding must be set "true" last after other parameters have been defined
    //arrayName  = imagebuf; // Name of FLASH array to be decoded
    mcuReady   = false; // Flag which is set true when a MCU block is ready for display
    doDecoding = true;  // Flag to tell task to decode the image

    while(doDecoding || mcuReady) {
        if (mcuReady) {
        tft.pushImage(mcu_x, mcu_y, mcu_w, mcu_h, mcuBuffer);
        mcuReady = false;
        }
        // Must yield in this loop
        yield();
    }
    // How much time did rendering take
    t = millis() - t;
    Serial.print(t); Serial.println(" ms");

    //drawArrayJpeg(imagebuf, sizeof(imagebuf), tft_width_offset, tft_height_offset); // Draw a jpeg image received from server
    delay(200);
    tft.fillScreen(TFT_BLACK);

     
    memset(arrayName,NULL,65535);
     w = 0, h = 0;
    

    client.print("S3");
    getImage(client); // receive image from server 
    Serial.println("got image 2 ");
    
    TJpgDec.getJpgSize(&w, &h, arrayName, sizeof(arrayName));
    Serial.print("Width = "); Serial.print(w); Serial.print(", height = "); Serial.println(h);

    t = millis();

  // The order here is important, doDecoding must be set "true" last after other parameters have been defined
  //arrayName  = imagebuf; // Name of FLASH array to be decoded
  mcuReady   = false; // Flag which is set true when a MCU block is ready for display
  doDecoding = true;  // Flag to tell task to decode the image

  // Only render MCU blocks if decoding is in progress OR an MCU is ready to render
  // Note: the OR mcuReady is required so the last block is rendered after decoding has ended
  while(doDecoding || mcuReady) {
    if (mcuReady) {
      tft.pushImage(mcu_x, mcu_y, mcu_w, mcu_h, mcuBuffer);
      mcuReady = false;
    }
    // Must yield in this loop
    yield();
  }

  // How much time did rendering take
  t = millis() - t;
  Serial.print(t); Serial.println(" ms");
    /*
    for (int i =0; i < 65535; i++){
      if (imagebuf[i] ==0) { imagebuf[i]=1; }
    }*/
     memset(arrayName,NULL,65535);   
    //drawArrayJpeg(imagebuf, sizeof(imagebuf), tft_width_offset, tft_height_offset); // Draw a jpeg image received from server
    
    delay(300);
}


void getImage(WiFiClient client) {
    
    while (client.available() > 0 || arrayName[0] == NULL) {
      //imagebuf={ NULL} ;
      client.read(arrayName, 65535);
    }
}
