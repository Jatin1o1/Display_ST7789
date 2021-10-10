#include <WiFi.h>
#include <JPEGDecoder.h>
#include <SPI.h>
#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI();

/*
#define TFT_MOSI 23  // SDA Pin on ESP32   (DIN pin of LCD )
#define TFT_SCLK 18  // SCL Pin on ESP32   ( CLK pin of LCD)
#define TFT_CS   15  // Chip select control pin
#define TFT_DC    2  // Data Command control pin
#define TFT_RST   4  // Reset pin (could connect to RST pin)
*/


const char AP_DEMO_HTTP_200_IMAGE[] = "HTTP/1.1 200 OK\r\nPragma: public\r\nCache-Control: max-age=1\r\nExpires: Thu, 26 Dec 2016 23:59:59 GMT\r\nContent-Type: image/";

typedef enum
{
    UPL_AP_STAT_MAIN = 1,           // GET /
    UPL_AP_STAT_LED_HIGH,           // GET /H
    UPL_AP_STAT_LED_LOW,            // GET /L
    UPL_AP_STAT_GET_IMAGE,          // GET /logo.bmp
    UPL_AP_STAT_GET_FAVICON,        // GET /favicon.ico
    UPL_AP_STAT_POST_UPLOAD,        // POST /upload
    UPL_AP_STAT_POST_START_BOUNDRY, // POST /upload boundry
    UPL_AP_STAT_POST_GET_BOUNDRY,   // POST /upload boundry
    UPL_AP_STAT_POST_START_IMAGE,   // POST /upload image
    UPL_AP_STAT_POST_GET_IMAGE,     // POST /upload image
} UPL_AP_STAT_t;

#define WIFI_SSID "LeCasa 2nd Floor 2"
#define WIFI_PASSWORD "LeCasa@1575"

#define LED_PIN 4

WiFiServer server(80);

#define MAX_IMAGE_SIZE 65535
#define MAX_BUF_SIZE 1024
//#define IMAGE_DEBUG

int value = 0;
char boundbuf[MAX_BUF_SIZE];
int boundpos = 0;
char imagetypebuf[MAX_BUF_SIZE];
int imagetypepos = 0;
//char imagebuf[MAX_IMAGE_SIZE];
uint8_t imagebuf[MAX_IMAGE_SIZE] PROGMEM = {};
int imagepos = 0;

void setup()
{
    bool ret;

    Serial.begin(115200);

    // We start by connecting to a WiFi network
    Serial.println();
    Serial.println();
    Serial.println("WiFi Connecting.");

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println(WiFi.localIP());

    server.begin();

    tft.begin();
    tft.setRotation(1);  // landscape
  tft.fillScreen(TFT_BLACK);
  delay(500);
  tft.fillScreen(TFT_RED);
  delay(500);
  tft.fillScreen(TFT_BLACK);
  tft.drawRect(40,53,240,135,TFT_GREEN); 
}

void printUploadForm(WiFiClient client)
{
    Serial.println("printUploadForm");
    // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
    // and a content-type so the client knows what's coming, then a blank line:
    client.println("HTTP/1.1 200 OK");
    client.println("Content-type:text/html");
    client.println();
    client.println("<html>");
    client.println("<body>");
    client.println();
    client.println("<form action=\"upload\" method=\"post\" enctype=\"multipart/form-data\">");
    client.println("Select image to upload:");
    client.println("<input type=\"file\" name=\"fileToUpload\" id=\"fileToUpload\">");
    client.println("<input type=\"submit\" value=\"Upload Image\" name=\"submit\">");
    client.println("</form>");
    client.println();
    client.println("</body>");
    client.println("</html>");

    client.println();
}

void printImage(WiFiClient client)
{
    Serial.println("printImage");
    // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
    // and a content-type so the client knows what's coming, then a blank line:
    client.print(AP_DEMO_HTTP_200_IMAGE);
        client.print("hi dude ");

    client.print(imagetypebuf);
    client.print("hi again ");
    client.print("\r\n\r\n");
#ifdef IMAGE_DEBUG
    Serial.print(AP_DEMO_HTTP_200_PNG);
#endif
    for (int i = 0; i < imagepos; i++)
    {
        client.write(imagebuf[i]);
#ifdef IMAGE_DEBUG
        Serial.write(imagebuf[i]);
#endif
    }
    //drawArrayJpeg((uint8_t *)imagebuf, imagepos, 30, 52);
    drawArrayJpeg((uint8_t *)imagebuf, imagepos,40,53); // Draw a jpeg image stored in memory, test cropping

}

void printThanks(WiFiClient client)
{
    Serial.println("printThanks");
    // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
    // and a content-type so the client knows what's coming, then a blank line:
    client.println("HTTP/1.1 200 OK");
    client.println("Content-type:text/html");
    client.println();
    client.println("<html>");
    client.println("<body>");
    client.println();
    client.println("Thank You");
    client.println("<a id=\"logo\" href=\"/\"><img src=\"logo.bmp\" alt=\"logo\" border=\"0\"></a>");
    client.println();
    client.println("</body>");
    client.println("</html>");
    // the content of the HTTP response follows the header:
    //client.print("Click <a href=\"/H\">here</a> turn the LED on pin 5 on<br>");
    //client.print("Click <a href=\"/L\">here</a> turn the LED on pin 5 off<br>");

    // The HTTP response ends with another blank line:
    client.println();
}

void loop()
{
    int cnt;
    bool newconn = false;
    int stat;
    WiFiClient client = server.available(); // listen for incoming clients

    if (client)
    { // if you get a client,
        stat = 0;
        boundpos = 0;
        Serial.println("new client"); // print a message out the serial port
        String currentLine = "";      // make a String to hold incoming data from the client
        while (client.connected())
        { // loop while the client's connected
            cnt = client.available();
            if (cnt)
            { // if there's bytes to read from the client,
#ifdef IMAGE_DEBUG
                if (newconn == false)
                {
                    Serial.println(cnt);
                    newconn = true;
                }
#endif
                char c = client.read(); // read a byte, then
#ifndef IMAGE_DEBUG
                if (stat != UPL_AP_STAT_POST_GET_IMAGE)
                {
#endif
                    Serial.write(c); // print it out the serial monitor
#ifndef IMAGE_DEBUG
                }
#endif

                if (stat == UPL_AP_STAT_POST_GET_IMAGE)
                {
                    if (imagepos < MAX_IMAGE_SIZE)
                    {
                        imagebuf[imagepos] = c;
                        imagepos++;
                    }
                }
                if (c == '\n')
                { // if the byte is a newline character
#ifdef IMAGE_DEBUG
                    Serial.print("stat is equal=");
                    Serial.println(stat);
#endif
                    if (stat == UPL_AP_STAT_POST_START_BOUNDRY)
                    {
                        boundbuf[boundpos] = '\0';
                        boundpos++;
#ifdef IMAGE_DEBUG
                        Serial.println("&&&&&&&&&&&&&&&&&");
                        Serial.println(boundbuf);
                        Serial.println("&&&&&&&&&&&&&&&&&");
#endif
                        stat = UPL_AP_STAT_POST_UPLOAD;
                        Serial.println("stats=UPL_AP_STAT_POST_UPLOAD");
                    }
                    if (stat == UPL_AP_STAT_POST_START_IMAGE && currentLine.length() == 0)
                    {
                        imagetypebuf[imagetypepos] = '\0';
                        imagetypepos++;
#ifdef IMAGE_DEBUG
                        Serial.println("&&&&&&&&&&&&&&&&&");
                        Serial.println(imagetypebuf);
                        Serial.println("&&&&&&&&&&&&&&&&&");
#endif
                        imagepos = 0;
                        stat = UPL_AP_STAT_POST_GET_IMAGE;
                        Serial.println("stats=UPL_AP_STAT_POST_GET_IMAGE");
                    }
                    // if you got a newline, then clear currentLine:
                    currentLine = "";
                    newconn = false;
                }
                else if (c != '\r')
                {                     // if you got anything else but a carriage return character,
                    currentLine += c; // add it to the end of the currentLine
                    if (stat == UPL_AP_STAT_POST_START_BOUNDRY)
                    {
                        if (boundpos < MAX_BUF_SIZE)
                        {
                            boundbuf[boundpos] = c;
                            boundpos++;
                        }
                    }
                    if (stat == UPL_AP_STAT_POST_START_IMAGE)
                    {
                        if (imagetypepos < MAX_BUF_SIZE)
                        {
                            imagetypebuf[imagetypepos] = c;
                            imagetypepos++;
                        }
                    }
                }

                // Check to see if the client request was "GET / "
                if (currentLine.endsWith("GET / "))
                {
                    stat = UPL_AP_STAT_MAIN;
                    Serial.println("stats=UPL_AP_STAT_MAIN");
                }
                if (currentLine.endsWith("GET /logo.bmp "))
                {
                    stat = UPL_AP_STAT_GET_IMAGE;
                    Serial.println("stats=UPL_AP_STAT_GET_IMAGE");
                }
                if (currentLine.endsWith("POST /upload "))
                {
                    stat = UPL_AP_STAT_POST_UPLOAD;
                    Serial.println("stats=UPL_AP_STAT_POST_UPLOAD");
                }
                if (stat == UPL_AP_STAT_POST_UPLOAD && currentLine.endsWith("Content-Type: multipart/form-data; boundary="))
                {
                    stat = UPL_AP_STAT_POST_START_BOUNDRY;
                    Serial.println("stats=UPL_AP_STAT_POST_START_BOUNDRY");
                }
                if (stat == UPL_AP_STAT_POST_UPLOAD && currentLine.endsWith("Content-Type: image/"))
                {
                    stat = UPL_AP_STAT_POST_START_IMAGE;
                    Serial.println("stats=UPL_AP_STAT_POST_START_IMAGE");
                }
                if (stat == UPL_AP_STAT_POST_UPLOAD && boundpos > 0 && currentLine.endsWith(boundbuf))
                {
                    Serial.println("found boundry");
                }
                if (stat == UPL_AP_STAT_POST_GET_IMAGE && boundpos > 0 && currentLine.endsWith(boundbuf))
                {
                    Serial.println("found image boundry");
                    Serial.println(imagepos);
                    stat = UPL_AP_STAT_POST_UPLOAD;
                    imagepos = imagepos - boundpos - 3;
#ifdef IMAGE_DEBUG
                    Serial.println(imagepos);
                    for (int i = 0; i < imagepos; i++)
                    {
                        Serial.write(imagebuf[i]);
                    }
#endif
                    Serial.println("stats=UPL_AP_STAT_POST_UPLOAD");
                }
            }
            else
            {
                if (stat == UPL_AP_STAT_MAIN)
                {
                    printUploadForm(client);
                    break;
                }
                if (stat == UPL_AP_STAT_POST_UPLOAD)
                {
                    printThanks(client);
                    break;
                }
                if (stat == UPL_AP_STAT_GET_IMAGE)
                {
                    printImage(client);
                    break;
                }

                Serial.println("stat unknown");
                delay(1000);
                break;
            }
        }
        // close the connection:
        client.stop();
        Serial.println("client disonnected");
    }

    delay(100);
}

/*====================================================================================
  This sketch contains support functions to render the Jpeg images.
  Created by Bodmer 15th Jan 2017
  ==================================================================================*/

// Return the minimum of two values a and b
#define minimum(a, b) (((a) < (b)) ? (a) : (b))


//####################################################################################################
// Draw a JPEG on the TFT pulled from a program memory array
//####################################################################################################
void drawArrayJpeg(const uint8_t arrayname[], uint32_t array_size, int xpos, int ypos) {

  int x = xpos;
  int y = ypos;

  JpegDec.decodeArray(arrayname, array_size);
  
  jpegInfo(); // Print information from the JPEG file (could comment this line out)
  
  renderJPEG(x, y);
  
  Serial.println("#########################");
}

//####################################################################################################
// Draw a JPEG on the TFT, images will be cropped on the right/bottom sides if they do not fit
//####################################################################################################
// This function assumes xpos,ypos is a valid screen coordinate. For convenience images that do not
// fit totally on the screen are cropped to the nearest MCU size and may leave right/bottom borders.
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

//####################################################################################################
// Print image information to the serial port (optional)
//####################################################################################################
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