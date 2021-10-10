 #include <TFT_eSPI.h>
#include <JPEGDecoder.h>
#include <WiFi.h>

TFT_eSPI tft = TFT_eSPI();

/* pin configuration 
#define TFT_MOSI 23  // SDA Pin on ESP32   (DIN pin of LCD )
#define TFT_SCLK 18  // SCL Pin on ESP32   ( CLK pin of LCD)
#define TFT_CS   15  // Chip select control pin
#define TFT_DC    2  // Data Command control pin
#define TFT_RST   4  // Reset pin (could connect to RST pin)
*/

#define MAX_IMAGE_SIZE 65535   // 64KB maximum image size to be received
uint16_t last_image_size = 0;

#define minimum(a,b)     (((a) < (b)) ? (a) : (b))

uint8_t imagebuf[MAX_IMAGE_SIZE]  = { NULL };

#define WIFI_SSID "LeCasa 2nd Floor 2"
#define WIFI_PASSWORD "LeCasa@1575"

const uint16_t port = 8090;
const char * host = "192.168.31.240";

#define tft_width_offset 40
#define tft_height_offset 53

#define tft_width 240
#define tft_height 135

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

    Serial.println("Connected to server successful!");
    client.print("S");
    getImage(client); // receive image from server 
    Serial.println("got image 1 ");
    drawArrayJpeg(imagebuf, sizeof(imagebuf), tft_width_offset, tft_height_offset); // Draw a jpeg image received from server
     
    memset(imagebuf,NULL,65535);

    client.print("S3");
    getImage(client); // receive image from server 
    Serial.println("got image 2 ");
    /*
    for (int i =0; i < 65535; i++){
      if (imagebuf[i] ==0) { imagebuf[i]=1; }
    }*/
    
    drawArrayJpeg(imagebuf, sizeof(imagebuf), tft_width_offset, tft_height_offset); // Draw a jpeg image received from server
       
    delay(3000);
}
void drawArrayJpeg(const uint8_t *arrayname, uint32_t array_size, int xpos, int ypos) {

  int x = xpos;
  int y = ypos;

  try {
    Serial.println("trying to decode image " );
   JpegDec.decodeArray(&arrayname[0], array_size);
   Serial.println("image decoded");
  renderJPEG(x, y);
  } 
  catch (const std::exception& e) {
    for (int i=0; i < array_size; i++){
      Serial.print(arrayname[i]);}
      
      Serial.println(e.what() ); // information from error printed   
  }
  Serial.println("#########################");
}



void renderJPEG(int xpos, int ypos) {

  // retrieve infomration about the image
  Serial.println(" 125");
  uint16_t *pImg;
  uint16_t mcu_w = JpegDec.MCUWidth;
  uint16_t mcu_h = JpegDec.MCUHeight;
  uint32_t max_x = JpegDec.width;
  uint32_t max_y = JpegDec.height;

  Serial.println(" 131");
  Serial.print(" MCU_W is ");
  Serial.println( mcu_w);
  Serial.print(" MCU_H is ");
  Serial.println(mcu_h);

  // Jpeg images are draw as a set of image block (tiles) called Minimum Coding Units (MCUs)
  // Typically these MCUs are 16x16 pixel blocks
  // Determine the width and height of the right and bottom edge image blocks
  uint32_t min_w = minimum(mcu_w, max_x % mcu_w);
  uint32_t min_h = minimum(mcu_h, max_y % mcu_h);

//  #define minimum(a,b)     (((a) < (b)) ? (a) : (b))

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

void getImage(WiFiClient client) {
    
    while (client.available() > 0 || imagebuf[0] == NULL) {
      //imagebuf={ NULL} ;
      client.read(imagebuf, 65535);
    }
}
