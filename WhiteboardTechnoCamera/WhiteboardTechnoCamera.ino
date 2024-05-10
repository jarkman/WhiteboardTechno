// build for M5TimerCam


// derived from the sta.ino sample for TimerCameraX

// build for M5UnitCam
// partition scheme default (4Mb with spiffs)
// PSRAM enabled

//#include "esp_camera.h"

#include <Wire.h>
#include "M5TimerCAM.h"
#include <WiFi.h>

#include "I2CConstants.h"

#include "Track.h"
#include "Tune.h"

const char* ssid     = "beetle";
const char* password = "nomplasm";

WiFiServer server(80);
static void jpegStream(WiFiClient* client);


Tune tune;

bool doWifi = false;

void setup()
{
  Wire.begin(4, 13); // join i2c bus - nonstandard pins for this board
  Serial.begin(9600);
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


    TimerCAM.begin();

    if (!TimerCAM.Camera.begin()) {
        Serial.println("Camera Init Fail");
        return;
    }
    Serial.println("Camera Init Success");

    int res = -3;

    res = TimerCAM.Camera.sensor->set_pixformat(TimerCAM.Camera.sensor,
                                          //PIXFORMAT_RGB888); bad
                                          //PIXFORMAT_GRAYSCALE); bad
                                          PIXFORMAT_RGB565);
                                          // PIXFORMAT_JPEG); // good
    Serial.println(res);
    res = TimerCAM.Camera.sensor->set_framesize(TimerCAM.Camera.sensor,
                                          //FRAMESIZE_QVGA);   // 320x240 - works
                                          FRAMESIZE_SVGA );     // 800x600  is the defalt in this example - very stoppy but doesn't crash
                                          // FRAMESIZE_QXGA);     // 2048x1536 is the max the camera can do - makes it crash
    Serial.println(res);
    //res = TimerCAM.Camera.sensor->set_vflip(TimerCAM.Camera.sensor, 1);
    //Serial.println(res);
    //res = TimerCAM.Camera.sensor->set_hmirror(TimerCAM.Camera.sensor, 0);
    //Serial.println(res);
    

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


    tune.loop();

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
    else
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
        camera_fb_t *fb = NULL;

        int err = TimerCAM.Camera.get();

        if (!err ) {
          Serial.print("get err");
          Serial.println(err);
        }
        
        fb = TimerCAM.Camera.fb;


        //fb = esp_camera_fb_get();
        esp_err_t res = ESP_OK;
        if (!fb) {
          Serial.println("Camera capture failed");
          res = ESP_FAIL;
        } else {


          process_frame(fb, "loop");
          //esp_camera_fb_return(fb);
          fb = NULL;

          logEnd();
        }
      }

    }
}


void process_frame(camera_fb_t *fb, char *at) {
  //Serial.println(at);
  // uint8_t * buf;
  //size_t len;                 /*!< Length of the buffer in bytes */
  //  size_t width;               /*!< Width of the buffer in pixels */
  //  size_t height;              /*!< Height of the buffer in pixels */
  //  pixformat_t format;         /*!< F

  
  Serial.print("processFrame ");
  Serial.println(at);

  Serial.print("len ");
  Serial.println(fb->len);
  Serial.print("height ");
  Serial.println(fb->height);
  Serial.print("width ");
  Serial.println(fb->width);
  Serial.print("format ");
  Serial.println(fb->format);

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
        int err = TimerCAM.Camera.get();

        if (!err ) {
          Serial.print("get err");
          Serial.println(err);
        }
        else
        {
          Serial.println("after get");
            TimerCAM.Power.setLed(255);
            Serial.printf("pic size: %d\n", TimerCAM.Camera.fb->len);

            client->print(_STREAM_BOUNDARY);

            

            uint8_t * buf = NULL;
            size_t buf_len = 0;
            bool converted = frame2bmp(TimerCAM.Camera.fb, &buf, &buf_len);
            //esp_camera_fb_return(fb);
            if(!converted){
                log_e("BMP Conversion failed");
                //httpd_resp_send_500(req);
                return;// ESP_FAIL;
            }
            //res = httpd_resp_send(req, (const char *)buf, buf_len);
            

            client->printf(_STREAM_PART,buf_len);
            
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
            Serial.printf("MJPG: %luKB %lums (%.1ffps)\r\n",
                          (long unsigned int)(buf_len / 1024),
                          (long unsigned int)frame_time,
                          1000.0 / (long unsigned int)frame_time);

            TimerCAM.Camera.free();
            TimerCAM.Power.setLed(0);
            free(buf);
        }

    }

client_exit:
    TimerCAM.Camera.free();
    TimerCAM.Power.setLed(0);
    client->stop();
    Serial.printf("Image stream end\r\n");
}

