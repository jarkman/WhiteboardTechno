// build for M5TimerCam


// derived from the sta.ino sample for TimerCameraX

// build for M5UnitCam
// partition scheme default (4Mb with spiffs)
// PSRAM enabled

//#include "esp_camera.h"

#include <Wire.h>
#include "esp_camera.h"
#include <WiFi.h>

#include "I2CConstants.h"

#include "Track.h"
#include "Tune.h"


// ===================
// Select camera model
// ===================
//#define CAMERA_MODEL_WROVER_KIT // Has PSRAM
//#define CAMERA_MODEL_ESP_EYE // Has PSRAM
//#define CAMERA_MODEL_ESP32S3_EYE // Has PSRAM
//define CAMERA_MODEL_M5STACK_PSRAM // Has PSRAM   
//#define CAMERA_MODEL_M5STACK_V2_PSRAM  // M5Camera version B  - this is what we build with for Sam Underwood
//#define CAMERA_MODEL_M5STACK_WIDE // Has PSRAM
//#define CAMERA_MODEL_M5STACK_ESP32CAM // No PSRAM
#define CAMERA_MODEL_M5STACK_UNITCAM // No PSRAM
//#define CAMERA_MODEL_AI_THINKER // Has PSRAM
//#define CAMERA_MODEL_TTGO_T_JOURNAL // No PSRAM
//#define CAMERA_MODEL_XIAO_ESP32S3 // Has PSRAM
// ** Espressif Internal Boards **
//#define CAMERA_MODEL_ESP32_CAM_BOARD
//#define CAMERA_MODEL_ESP32S2_CAM_BOARD
//#define CAMERA_MODEL_ESP32S3_CAM_LCD
//#define CAMERA_MODEL_DFRobot_FireBeetle2_ESP32S3 // Has PSRAM
//#define CAMERA_MODEL_DFRobot_Romeo_ESP32S3 // Has PSRAM
#include "camera_pins.h"

const char* ssid     = "beetle";
const char* password = "nomplasm";

WiFiServer server(80);
static void jpegStream(WiFiClient* client);


Tune tune;

bool doWifi = true;
bool drawMarkers = true;

bool streamRunning = false;

camera_fb_t *fb = NULL;

void setup()
{
  Wire.begin(4, 13); // join i2c bus - nonstandard pins for this board
  Serial.begin(115200);
  Serial.println("WhiteboardTechnoCamera");
  Serial.println("... waiting");
  delay(5000);
  Serial.println("..waited");
}

void delayedSetup()
{
  Serial.println("Starting");
Serial.println("Starting");
Serial.println("Starting");


    setupCamera();

    Serial.println("Camera Init Success");

    //TimerCAM.Camera.config->fb_location = CAMERA_FB_IN_PSRAM;  
    int res = -3;

    
    

  if( doWifi )
  {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    WiFi.setSleep(false);
    Serial.println("");
    Serial.print("Connecting to ");
    Serial.println(ssid);
    // Wait for connection
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    server.begin();
  }
}

void setupCamera()
{

// https://www.reddit.com/r/esp32/comments/vndbh7/how_to_increase_fps_on_esp32cam/#:~:text=The%20internet%20advises%20setting%20it,the%20FPS%20the%20camera%20captures.

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.frame_size = FRAMESIZE_QQVGA;  // 160x120 - works for streaming
  // config.frame_size = FRAMESIZE_QVGA; //causes all kinds of slowness when streaming
  
  config.pixel_format = PIXFORMAT_RGB565;  // for face detection/recognition
  config.grab_mode = CAMERA_GRAB_LATEST; //CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12;
  config.fb_count = 2;

#if CONFIG_IDF_TARGET_ESP32S3
    config.fb_count = 2;
#endif
  

#if defined(CAMERA_MODEL_ESP_EYE)
  pinMode(13, INPUT_PULLUP);
  pinMode(14, INPUT_PULLUP);
#endif

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  sensor_t *s = esp_camera_sensor_get();
  // initial sensors are flipped vertically and colors are a bit saturated
  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1);        // flip it back
    s->set_brightness(s, 1);   // up the brightness just a bit
    s->set_saturation(s, -2);  // lower the saturation
  }
  

#if defined(CAMERA_MODEL_M5STACK_WIDE) || defined(CAMERA_MODEL_M5STACK_ESP32CAM)
  s->set_vflip(s, 1);
  s->set_hmirror(s, 1);
#endif

#if defined(CAMERA_MODEL_ESP32S3_EYE)
  s->set_vflip(s, 1);
#endif

// Setup LED FLash if LED pin is defined in camera_pins.h
#if defined(LED_GPIO_NUM)
  setupLedFlash(LED_GPIO_NUM);
#endif


}


long last_diags = 0;
bool doneDelayedSetup = false;
long lastFrameMillis = 0;

long scanStart = 0;

bool doGraph = false;


void logStart() {
  scanStart = micros();
}

void logEnd() {
  long t = micros() - scanStart;
  /*Serial.print("Scan took ");
  Serial.print(t);
  Serial.println(" us");
  */
}


void loop() {

  

   if( ! doneDelayedSetup )
    {
      doneDelayedSetup = true;
      delayedSetup();
    }


    

    if( doWifi )
    {
      WiFiClient client = server.available();  // listen for incoming clients
      if (client) {                            // if you get a client,
          while (client.connected()) {   // loop while the client's connected
              if (client.available()) {  // if there's bytes to read from the
                  bmpStream(&client);
              }
          }
          // close the connection:
          client.stop();
          Serial.println("Client Disconnected.");
      }
    }

    if( ! streamRunning )
    {
      long interval;

      if( doGraph )
        interval = 2;
      else
        interval = 400; 
        // loop too fast and we get bum frames somehow. 
        // this was improved a bit by config.grab_mode = CAMERA_GRAB_LATEST; 
        // and config.fb_count = 2;

      delay( interval );
      long now = millis();
      if( now - lastFrameMillis > interval )
      {
        lastFrameMillis = now;
      
        logStart();
        
        if( fb != NULL )
        {
          esp_camera_fb_return(fb);
          fb = NULL;
        }

        fb = esp_camera_fb_get();
        esp_err_t res = ESP_OK;
        if (!fb) {
          Serial.println("Camera capture failed");
          res = ESP_FAIL;
        } else {


          processFrame("loop");
         
          logEnd();
        }
      }

    }

    tune.loop();


   
}

uint32_t pixelBrightness( int x, int y )
{
  int byte = 2 * (fb->width * y + x);

  uint8_t *pix_address = fb->buf + byte;

  uint16_t m = *((uint16_t *)pix_address);
  uint16_t pix = (m & 0xff) << 8 | ((m >> 8) & 0xff);  // swap the bytes
  
  int red = (pix >> 11) & 0b0011111;
  int green = ((pix >> 5) & 0b0111111) / 2;  // divide by 2 to normalise
  int blue = (pix >> 0) & 0b0011111;

  
  return (red+green+blue)/3;
}


void setPixel(int x, int y, uint16_t rgb) {
  if( x < 0 || y < 0 || x > fb->width || y > fb->height)
  {
    Serial.printf("Bad xy (%d, %d vs %d, %d) in setPixel\n", x, y, fb->width, fb->height);
    return;
  }
  int byte = 2 * (fb->width * y + x);

  uint8_t a = (rgb & 0xff00) >> 8;
  uint8_t b = rgb & 0xff;

  fb->buf[byte] = a;
  fb->buf[byte + 1] = b;
}

void drawRect(int x0, int y0, int w, int h, uint16_t rgb) {

  for (int y = y0; y < y0+h; y++)
  {
    setPixel(x0, y, rgb);
    setPixel(x0+w, y, rgb);
  }
  for (int x = x0; x < x0+w; x++)
  {
    setPixel(x, y0, rgb);
    setPixel( x, y0+h, rgb);
  }
    
}


void drawMarker(int x0, int y0, uint16_t rgb) {
  for (int y = y0-1; y < y0+1; y++)
    for (int x = x0 - 1; x <= x0 + 1; x++) {
      setPixel(x, y, rgb);
    }
}

void processFrame( char *at) {
  //Serial.println(at);
  // uint8_t * buf;
  //size_t len;                 /*!< Length of the buffer in bytes */
  //  size_t width;               /*!< Width of the buffer in pixels */
  //  size_t height;              /*!< Height of the buffer in pixels */
  //  pixformat_t format;         /*!< F

  
  Serial.print("processFrame ");
  Serial.println(at);

  if( fb == NULL )
  {
    Serial.println("No frame!");
    return;
  }
  Serial.print("len ");
  Serial.println(fb->len);
  Serial.print("height ");
  Serial.println(fb->height);
  Serial.print("width ");
  Serial.println(fb->width);
  Serial.print("format ");
  Serial.println(fb->format);

  drawMarker(4,4, 0xf000);
  drawMarker(fb->width-4, fb->height-4, 0x000f);

  
}

void plotLines(int width, int pos)
{
  // this is the secret to getting more points to display in the serial plotter:
  // https://github.com/arduino/arduino-ide/issues/803#issuecomment-1338149431
  
  
  for (int x = 0; x < width; x++) 
  {
    //Serial.printf("%d\n", sum[x]);
    int l = -1;

    if( pos > 0 && abs( x-pos) < 5 )
      l = 300;

    if( pos < 0 )
      l = -50;
    //Serial.printf("B:%d, S:%d, D:%d, L:%d\n", baseline[x], sum[x],darkness[x], l);
  }

  for (int x = 0; x < 8; x++) 
  {
    Serial.printf("%d,%d,%d\n", -1,-1,-1);
  }
}



// used to image stream
#define PART_BOUNDARY "123456789000000000000987654321"
static const char* _STREAM_CONTENT_TYPE =
    "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART =
    "Content-Type: image/x-windows-bmp\r\nContent-Length: %u\r\n\r\n";

static void bmpStream(WiFiClient* client) {


    streamRunning = true;

    Serial.println("Image stream start");
    client->println("HTTP/1.1 200 OK");
    client->printf("Content-Type: %s\r\n", _STREAM_CONTENT_TYPE);
    client->println("Content-Disposition: inline; filename=capture.bmp");
    client->println("Access-Control-Allow-Origin: *");
    client->println();
    static int64_t last_frame = 0;
    if (!last_frame) {
        last_frame = esp_timer_get_time();
    }

    for (;;) {
        Serial.println("before get");
       
        if( fb != NULL )
        {
          esp_camera_fb_return(fb);
          fb = NULL;
        }

        fb = esp_camera_fb_get();
        esp_err_t res = ESP_OK;
        if (!fb) {
          Serial.println("Camera capture failed");
          res = ESP_FAIL;
        } else {
            Serial.printf("pic size: %d\n", fb->len);


            processFrame("stream");
            tune.process(); // to make markers to show

            client->print(_STREAM_BOUNDARY);

            

            uint8_t * buf = NULL;
            size_t buf_len = 0;
            bool converted = frame2bmp(fb, &buf, &buf_len);

            Serial.println("made bmp");
            //esp_camera_fb_return(fb);
            if(!converted){
                log_e("BMP Conversion failed");
                //httpd_resp_send_500(req);
                return;// ESP_FAIL;
            }
            //res = httpd_resp_send(req, (const char *)buf, buf_len);
            

            client->printf(_STREAM_PART,buf_len);
            
            Serial.println("sending");
            int32_t to_sends    = buf_len;
            int32_t now_sends   = 0;
            uint8_t* out_buf    = buf;
            uint32_t packet_len = 8 * 1024;
            while (to_sends > 0) {
                now_sends = to_sends > packet_len ? packet_len : to_sends;
                if (client->write(out_buf, now_sends) == 0) {
                    goto client_exit;
                }
                out_buf += now_sends;
                to_sends -= packet_len;
            }

            int64_t fr_end     = esp_timer_get_time();
            int64_t frame_time = fr_end - last_frame;
            last_frame         = fr_end;
            frame_time /= 1000;

            Serial.println("sent");

            Serial.printf("MJPG: %luKB %lums (%.1ffps)\r\n",
                          (long unsigned int)(buf_len / 1024),
                          (long unsigned int)frame_time,
                          1000.0 / (long unsigned int)frame_time);

            
           
            free(buf);
        }

    }

client_exit:
   
    client->stop();
    Serial.printf("Image stream end\r\n");

    streamRunning = false;
}

