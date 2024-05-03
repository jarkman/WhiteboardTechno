// build for M5TimerCam

// Wire Master Writer
// by Nicholas Zambetti <http://www.zambetti.com>

// Demonstrates use of the Wire library
// derived from the sta.ino sample for TimerCameraX
//build for M5UnitCam
// partition scheme default (4Mb with spiffs)
// PSRAM enabled

#include "esp_camera.h"

#include <Wire.h>
#include "M5TimerCAM.h"
#include <WiFi.h>

const char* ssid     = "beetle";
const char* password = "nomplasm";

WiFiServer server(80);
static void jpegStream(WiFiClient* client);

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

    TimerCAM.Camera.sensor->set_pixformat(TimerCAM.Camera.sensor,
                                          PIXFORMAT_JPEG);
    TimerCAM.Camera.sensor->set_framesize(TimerCAM.Camera.sensor,
                                          FRAMESIZE_QVGA);   // 320x240 - works
                                          //FRAMESIZE_SVGA );     // 800x600  is the defalt in this example - very stoppy but doesn't crash
                                          // FRAMESIZE_QXGA);     // 2048x1536 is the max the camera can do - makes it crash
    TimerCAM.Camera.sensor->set_vflip(TimerCAM.Camera.sensor, 1);
    TimerCAM.Camera.sensor->set_hmirror(TimerCAM.Camera.sensor, 0);

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

long last_diags = 0;
bool doneDelayedSetup = false;

void loop() {

   if( ! doneDelayedSetup )
    {
      doneDelayedSetup = true;
      delayedSetup();
    }

    WiFiClient client = server.available();  // listen for incoming clients
    if (client) {                            // if you get a client,
        while (client.connected()) {   // loop while the client's connected
            if (client.available()) {  // if there's bytes to read from the
                jpegStream(&client);
            }
        }
        // close the connection:
        client.stop();
        Serial.println("Client Disconnected.");
    }
}

// used to image stream
#define PART_BOUNDARY "123456789000000000000987654321"
static const char* _STREAM_CONTENT_TYPE =
    "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART =
    "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

static void jpegStream(WiFiClient* client) {
    Serial.println("Image stream satrt");
    client->println("HTTP/1.1 200 OK");
    client->printf("Content-Type: %s\r\n", _STREAM_CONTENT_TYPE);
    client->println("Content-Disposition: inline; filename=capture.jpg");
    client->println("Access-Control-Allow-Origin: *");
    client->println();
    static int64_t last_frame = 0;
    if (!last_frame) {
        last_frame = esp_timer_get_time();
    }

    for (;;) {
        if (TimerCAM.Camera.get()) {
            TimerCAM.Power.setLed(255);
            Serial.printf("pic size: %d\n", TimerCAM.Camera.fb->len);

            client->print(_STREAM_BOUNDARY);
            client->printf(_STREAM_PART, TimerCAM.Camera.fb);
            int32_t to_sends    = TimerCAM.Camera.fb->len;
            int32_t now_sends   = 0;
            uint8_t* out_buf    = TimerCAM.Camera.fb->buf;
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
                          (long unsigned int)(TimerCAM.Camera.fb->len / 1024),
                          (long unsigned int)frame_time,
                          1000.0 / (long unsigned int)frame_time);

            TimerCAM.Camera.free();
            TimerCAM.Power.setLed(0);
        }
    }

client_exit:
    TimerCAM.Camera.free();
    TimerCAM.Power.setLed(0);
    client->stop();
    Serial.printf("Image stream end\r\n");
}

byte x = 0;

void sendI2C()
{
  Serial.println(x);
  Wire.beginTransmission(4); // transmit to device #4
  Wire.write("x is ");        // sends five bytes
  Wire.write(x);              // sends one byte  
  Wire.endTransmission();    // stop transmitting
  x++;
  delay(500);
}