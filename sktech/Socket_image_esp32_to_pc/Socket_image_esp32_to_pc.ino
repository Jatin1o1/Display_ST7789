
#include <TFT_eSPI.h>
#include <JPEGDecoder.h>
#include <WiFi.h>
#include "jpeg1.h"

TFT_eSPI tft = TFT_eSPI();

/*
#define TFT_MOSI 23  // SDA Pin on ESP32   (DIN pin of LCD )
#define TFT_SCLK 18  // SCL Pin on ESP32   ( CLK pin of LCD)
#define TFT_CS   15  // Chip select control pin
#define TFT_DC    2  // Data Command control pin
#define TFT_RST   4  // Reset pin (could connect to RST pin)
*/

#define WIFI_SSID "LeCasa 2nd Floor 2"
#define WIFI_PASSWORD "LeCasa@1575"

const uint16_t port = 8090;
const char * host = "192.168.31.240";

//WiFiServer server(80);

#define minimum(a,b)     (((a) < (b)) ? (a) : (b))


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

}

//####################################################################################################
// Main loop
//####################################################################################################
void loop() {


WiFiClient client;

    if (!client.connect(host, port)) {
        Serial.println("Connection to host failed");
        delay(1000);
        //return;
    }

    Serial.println("Connected to server successful!");

  
    // Image metadata.  Yes it should be cleaned up to use printf if the function is available
    //Serial.print("Size of image:");
    //Serial.println(fb->len);
    //Serial.print("Shape->width:");
    //Serial.print(fb->width);
    //Serial.print("height:");
    //Serial.println(fb->height);
    client.print("Shape->width:");
    client.print(300);
    client.print("height:");
    client.println(300);


    // Give the server a chance to receive the information before sending an acknowledgement.
    delay(1000);
    getResponse(client);
    //Serial.print(EagleEye);
    client.write(EagleEye, sizeof(EagleEye));

    Serial.println("Disconnecting...");
    client.stop();

    delay(3000);
    Serial.println("displaying image on tft");

    drawArrayJpeg(EagleEye, sizeof(EagleEye), 40, 53); // Draw a jpeg image stored in memory
    delay(2000);


}

void drawArrayJpeg(const uint8_t arrayname[], uint32_t array_size, int xpos, int ypos) {

  int x = xpos;
  int y = ypos;

  JpegDec.decodeArray(arrayname, array_size);
  
  jpegInfo(); // Print information from the JPEG file (could comment this line out)
  
  renderJPEG(x, y);
  
  Serial.println("#########################");
}

void renderJPEG(int xpos, int ypos) {

  // retrieve infomration about the image
  uint16_t *pImg;
  uint16_t mcu_w = JpegDec.MCUWidth;
  uint16_t mcu_h = JpegDec.MCUHeight;
  uint32_t max_x = JpegDec.width;
  uint32_t max_y = JpegDec.height;

  // Jpeg images are draw as a set of image block (tiles) called Minimum Coding Units (MCUs)
  // Typically these MCUs are 16x16 pixel blocks
  // Determine the width and height of the right and bottom edge image blocks
  uint32_t min_w = minimum(mcu_w, max_x % mcu_w);
  uint32_t min_h = minimum(mcu_h, max_y % mcu_h);

  // save the current image block size
  uint32_t win_w = mcu_w;
  uint32_t win_h = mcu_h;

  // record the current time so we can measure how long it takes to draw an image
  uint32_t drawTime = millis();

  // save the coordinate of the right and bottom edges to assist image cropping
  // to the screen size
  max_x += xpos;
  max_y += ypos;

  // read each MCU block until there are no more
  while (JpegDec.read()) {
    
    // save a pointer to the image block
    pImg = JpegDec.pImage ;

    // calculate where the image block should be drawn on the screen
    int mcu_x = JpegDec.MCUx * mcu_w + xpos;  // Calculate coordinates of top left corner of current MCU
    int mcu_y = JpegDec.MCUy * mcu_h + ypos;

    // check if the image block size needs to be changed for the right edge
    if (mcu_x + mcu_w <= max_x) win_w = mcu_w;
    else win_w = min_w;

    // check if the image block size needs to be changed for the bottom edge
    if (mcu_y + mcu_h <= max_y) win_h = mcu_h;
    else win_h = min_h;

    // copy pixels into a contiguous block
    if (win_w != mcu_w)
    {
      uint16_t *cImg;
      int p = 0;
      cImg = pImg + win_w;
      for (int h = 1; h < win_h; h++)
      {
        p += mcu_w;
        for (int w = 0; w < win_w; w++)
        {
          *cImg = *(pImg + w + p);
          cImg++;
        }
      }
    }

    // calculate how many pixels must be drawn
    uint32_t mcu_pixels = win_w * win_h;

    tft.startWrite();

    // draw image MCU block only if it will fit on the screen
    if (( mcu_x + win_w ) <= tft.width() && ( mcu_y + win_h ) <= tft.height())
    {

      // Now set a MCU bounding window on the TFT to push pixels into (x, y, x + width - 1, y + height - 1)
      tft.setAddrWindow(mcu_x, mcu_y, win_w, win_h);

      // Write all MCU pixels to the TFT window
      while (mcu_pixels--) {
        // Push each pixel to the TFT MCU area
        tft.pushColor(*pImg++);
      }

    }
    else if ( (mcu_y + win_h) >= tft.height()) JpegDec.abort(); // Image has run off bottom of screen so abort decoding

    tft.endWrite();
  }

  // calculate how long it took to draw the image
  drawTime = millis() - drawTime;

  // print the results to the serial port
  Serial.print(F(  "Total render time was    : ")); Serial.print(drawTime); Serial.println(F(" ms"));
  Serial.println(F(""));
}

void jpegInfo() {
  Serial.println(F("==============="));
  Serial.println(F("JPEG image info"));
  Serial.println(F("==============="));
  Serial.print(F(  "Width      :")); Serial.println(JpegDec.width);
  Serial.print(F(  "Height     :")); Serial.println(JpegDec.height);
  Serial.print(F(  "Components :")); Serial.println(JpegDec.comps);
  Serial.print(F(  "MCU / row  :")); Serial.println(JpegDec.MCUSPerRow);
  Serial.print(F(  "MCU / col  :")); Serial.println(JpegDec.MCUSPerCol);
  Serial.print(F(  "Scan type  :")); Serial.println(JpegDec.scanType);
  Serial.print(F(  "MCU width  :")); Serial.println(JpegDec.MCUWidth);
  Serial.print(F(  "MCU height :")); Serial.println(JpegDec.MCUHeight);
  Serial.println(F("==============="));
}



void getResponse(WiFiClient client) {
  byte buffer1[8] = { NULL };
  int length1=0;
  while (client.available() > 0 || buffer1[0] == NULL) {
    int len = client.available();
    length1=len;
    if (len > 8) {len = 8;length1=len;}
    client.read(buffer1, len);
    
  }
  if (true) {
      Serial.println("data received form server is " );
       Serial.write(buffer1,length1); // show in the serial monitor (slows some boards)
       Serial.println("");
    }
}

