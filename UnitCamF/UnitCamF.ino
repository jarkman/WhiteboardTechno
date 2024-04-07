#include "esp_camera.h"
#include <WiFi.h>

// we're building for a M5Stack Fish-eye camera, with ESP32 and OV2640
// https://shop.m5stack.com/products/fish-eye-camera-module-ov2640?variant=17346938273882

// select board M5UnitCAM
// select tools/partion scheme/8m with spiffs

//
// WARNING!!! PSRAM IC required for UXGA resolution and high JPEG quality
//            Ensure ESP32 Wrover Module or other board with PSRAM is selected
//            Partial images will be transmitted if image exceeds buffer size
//
//            You must select partition scheme from the board menu that has at least 3MB APP space.
//            Face Recognition is DISABLED for ESP32 and ESP32-S2, because it takes up from 15
//            seconds to process single frame. Face Detection is ENABLED if PSRAM is enabled as well

// ===================
// Select camera model
// ===================
//#define CAMERA_MODEL_WROVER_KIT // Has PSRAM
//#define CAMERA_MODEL_ESP_EYE // Has PSRAM
//#define CAMERA_MODEL_ESP32S3_EYE // Has PSRAM
//define CAMERA_MODEL_M5STACK_PSRAM // Has PSRAM                        RS - space problem
#define CAMERA_MODEL_M5STACK_V2_PSRAM  // M5Camera version B Has PSRAM RS - space problem
//#define CAMERA_MODEL_M5STACK_WIDE // Has PSRAM
//#define CAMERA_MODEL_M5STACK_ESP32CAM // No PSRAM
//#define CAMERA_MODEL_M5STACK_UNITCAM // No PSRAM
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

// ===========================
// Enter your WiFi credentials
// ===========================
const char *ssid = "beetle";
const char *password = "nomplasm";


bool doWebServer = true;

void startCameraServer();
void setupLedFlash(int pin);

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

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
  config.frame_size = FRAMESIZE_QVGA;
  //config.pixel_format = PIXFORMAT_JPEG; // for streaming
  config.pixel_format = PIXFORMAT_RGB565;  // for face detection/recognition
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12;
  config.fb_count = 1;

  // if PSRAM IC present, init with UXGA resolution and higher JPEG quality
  //                      for larger pre-allocated frame buffer.
  if (false && config.pixel_format == PIXFORMAT_JPEG) {
    if (psramFound()) {
      config.jpeg_quality = 10;
      config.fb_count = 2;
      config.grab_mode = CAMERA_GRAB_LATEST;
    } else {
      // Limit the frame size when PSRAM is not available
      config.frame_size = FRAMESIZE_SVGA;
      config.fb_location = CAMERA_FB_IN_DRAM;
    }
  } else {
    // Best option for face detection/recognition
    config.frame_size = FRAMESIZE_QVGA;  //FRAMESIZE_240X240;
#if CONFIG_IDF_TARGET_ESP32S3
    config.fb_count = 2;
#endif
  }

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
  // drop down frame size for higher initial frame rate
  if (config.pixel_format == PIXFORMAT_JPEG) {
    s->set_framesize(s, FRAMESIZE_QVGA);
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


  if( doWebServer )
  {
    Serial.print("Connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);
    WiFi.setSleep(false);

    int status = 0;

    while ((status = WiFi.status()) != WL_CONNECTED) {
      delay(500);
      Serial.print( status ); Serial.print(" ");
    }
    Serial.println("");
    Serial.println("WiFi connected");

    startCameraServer();

    Serial.print("Camera Ready! Use 'http://");
    Serial.print(WiFi.localIP());
    Serial.println("' to connect");
  }
}

void logStart();
void logEnd();

void loop() {
  
  delay(1000);

  if( ! doWebServer )
  {
    logStart();
    camera_fb_t *fb = NULL;

    fb = esp_camera_fb_get();
    esp_err_t res = ESP_OK;
    if (!fb) {
      Serial.println("Camera capture failed");
      res = ESP_FAIL;
    } else {


      process_frame(fb, "stream");
      esp_camera_fb_return(fb);
      fb = NULL;

      logEnd();
    }
  }
}


char *dark = " .~:-+#$£";
uint16_t r[512];
uint16_t g[512];
uint16_t b[512];
uint16_t sum[512];
uint16_t baseline[512];
int darkness[512];
bool firstFrame = true;
int32_t frameNumber = 0;

long scanStart = 0;

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

void setPixel(camera_fb_t *fb, int x, int y, uint16_t rgb) {
  int byte = 2 * (fb->width * y + x);

  uint8_t a = (rgb & 0xff00) >> 8;
  uint8_t b = rgb & 0xff;

  fb->buf[byte] = a;
  fb->buf[byte + 1] = b;
}

void drawMarker(camera_fb_t *fb, int pos, int yOffset, uint16_t rgb) {
  for (int y = yOffset * 8 + fb->height / 2 - 4; y < yOffset * 8 + fb->height / 2 + 4; y++)
    for (int x = pos - 4; x <= pos + 4; x++) {
      setPixel(fb, x, y, rgb);
    }
}
void process_frame(camera_fb_t *fb, char *at) {
  //Serial.println(at);
  // uint8_t * buf;
  //size_t len;                 /*!< Length of the buffer in bytes */
  //  size_t width;               /*!< Width of the buffer in pixels */
  //  size_t height;              /*!< Height of the buffer in pixels */
  //  pixformat_t format;         /*!< F

  /*
  Serial.print("len ");
  Serial.println(fb->len);
  Serial.print("height ");
  Serial.println(fb->height);
  Serial.print("width ");
  Serial.println(fb->width);
  Serial.print("format ");
  Serial.println(fb->format);
*/



  int lineBytes = fb->width * 2;
  int startByte = (fb->height / 2) * lineBytes;

  for (int x = 0; x < fb->width; x++) {
    r[x] = 0;
    g[x] = 0;
    b[x] = 0;

    sum[x] = 0;
  }

  if( firstFrame )
    for (int x = 0; x < fb->width; x++) {
      baseline[x] = 0;
    }

  // average over 4 rows
  for (int y = 0; y < 4; y++) {
    for (int x = 0; x < fb->width; x++) {
      int pix_num = startByte + 2 * x;
      uint8_t *pix_address = fb->buf + pix_num;

      //*((uint16_t*) pix_address) = 0b111100000000000;

      //*((uint16_t*) pix_address) = 0b0000000000011111;// b
      //*((uint16_t*) pix_address) = 0b0000011111100000;// g
      //*((uint16_t*) pix_address) = 0b1000000000000000;// r

      uint16_t m = *((uint16_t *)pix_address);
      uint16_t pix = (m & 0xff) << 8 | ((m >> 8) & 0xff);  // swap the bytes
      //*((uint16_t*) pix_address) = n;

      //*((uint16_t*) pix_address) = 0b1111111111111111;
      //*((uint16_t*) pix_address) = 0b1111111111111111;

      int red = (pix >> 11) & 0b0011111;
      int green = ((pix >> 5) & 0b0111111) / 2;  // divide by 2 to normalise
      int blue = (pix >> 0) & 0b0011111;

      // red will be darkest in the sum of green and blue
      r[x] += red; //green + blue;
      g[x] += green; //red + blue;
      b[x] += blue; // red + green;

      sum[x] += (r[x] + g[x] + b[x]) / 3;
    }
    startByte += 2 * fb->width;
  }

  for (int x = 0; x < fb->width; x++) {
    if( frameNumber < 5 ) // first frame seems wierd
      baseline[x] = sum[x];
    else
      baseline[x] = max( baseline[x], sum[x]);

    darkness[x] = (255 * (baseline[x] - sum[x])) / baseline[x]; // normalise for brightness
  }



  plotLines(fb->width);

  int redpos = findLine(r, fb->width, "red");
  int greenpos = findLine(g, fb->width, "green");
  int bluepos = findLine(b, fb->width, "blue");
  //int sumpos = findLine(sum,fb->width,"sum");

  //redpos = fb->width/2;

  if (redpos > -1)
    drawMarker(fb, redpos, 0, 0b1111100000000000);

  //drawMarker(fb, 10,    0b0000011111100000);
  //drawMarker(fb, fb->width-10,    0b0000000000011111);

  if (greenpos > -1)
    drawMarker(fb, greenpos, 1, 0b0000011111100000);
  if (bluepos > -1)
    drawMarker(fb, bluepos, 2, 0b0000000000011111);

  //Serial.print(dark[b/4]);
  //Serial.print(" ");

  firstFrame =false;
  frameNumber++;
}


void plotLines(int width)
{
  // this is the secret to getting more points to display in the serial plotter:
  // https://github.com/arduino/arduino-ide/issues/803#issuecomment-1338149431
  
  for (int x = 0; x < width; x++) 
  {
    //Serial.printf("%d, %d, %d\n", r[x], g[x],b[x]);
    Serial.printf("Baseline:%d, Sum:%d, Darkness:%d\n", baseline[x], sum[x],darkness[x]);
  }

  for (int x = 0; x < 8; x++) 
  {
    Serial.printf("%d, %d, %d\n", -1,-1,-1);
  }
}

int findLine(uint16_t l[], int width, char *label) {



  bool in = false;
  uint16_t threshold;
  int start = 0;
  int end = 0;
  int min = 1000;
  int minAt = -1;
  int max = -1;
  int maxAt = -1;



  int sum = 0;

  for (int i = 0; i < width; i++) {
    sum += l[i];
  }

  threshold = 50;  // from observation   (sum / width) / 2;

  for (int i = 0; i < width; i++) {
    if (l[i] < min) {
      min = l[i];
      minAt = i;
    }

    if (l[i] > max) {
      max = l[i];
      maxAt = i;
    }

    if (l[i] < threshold && !in) {
      in = true;
      start = i;
    }

    if (l[i] >= threshold && in) {
      in = false;
      end = i;
    }
  }

/*
  Serial.println();
  Serial.println(label);
  Serial.print(" ");
  Serial.print(start);
  Serial.print("-");
  Serial.print(end);
  Serial.print("  min of ");
  Serial.print(min);
  Serial.print(" at ");
  Serial.print(minAt);
  Serial.print("  max of ");
  Serial.print(max);
  Serial.print(" at ");
  Serial.println(maxAt);
*/

  if (max > threshold)
    return maxAt;

  //if(start>0 && end>0)
  //  return (start+end)/2;

  return -1;

  ////Serial.println();
}
