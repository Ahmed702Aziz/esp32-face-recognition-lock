#include "esp_camera.h"
#include <WiFi.h>

// === Your Wi-Fi credentials ===
const char *ssid = "TOPNET_ZAHY";
const char *password = "AmeN24980886??";

// === Camera Pin Configuration for AI Thinker (your provided pins) ===
#define PWDN_GPIO_NUM    32
#define RESET_GPIO_NUM   -1
#define XCLK_GPIO_NUM     0
#define SIOD_GPIO_NUM    26
#define SIOC_GPIO_NUM    27

#define Y9_GPIO_NUM      35
#define Y8_GPIO_NUM      34
#define Y7_GPIO_NUM      39
#define Y6_GPIO_NUM      36
#define Y5_GPIO_NUM      21
#define Y4_GPIO_NUM      19
#define Y3_GPIO_NUM      18
#define Y2_GPIO_NUM       5

#define VSYNC_GPIO_NUM   25
#define HREF_GPIO_NUM    23
#define PCLK_GPIO_NUM    22

WiFiServer server(80);

// === Setup Function ===
void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(false);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("Camera Stream Ready! Go to: http://");
  Serial.println(WiFi.localIP());

  // Configure Camera
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
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  // Init with high resolution but fast enough for streaming
  config.frame_size = FRAMESIZE_QVGA; // 320x240
  config.jpeg_quality = 10;           // Lower is better quality (0–63)
  config.fb_count = 1;

  // Init Camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }
sensor_t * s = esp_camera_sensor_get();
s->set_vflip(s, 1); 
  // Start the HTTP server
  server.begin();
}

// === Function to Stream MJPEG ===
void streamJpeg(WiFiClient client) {
  String response = "HTTP/1.1 200 OK\r\n";
  response += "Content-Type: multipart/x-mixed-replace; boundary=frame\r\n\r\n";
  client.print(response);

  while (client.connected()) {
    camera_fb_t * fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed");
      continue;
    }

    client.print("--frame\r\n");
    client.print("Content-Type: image/jpeg\r\n");
    client.print("Content-Length: " + String(fb->len) + "\r\n\r\n");
    client.write(fb->buf, fb->len);
    client.print("\r\n");

    esp_camera_fb_return(fb);

    delay(50); // slight delay to reduce network load
  }
}

// === Main Loop ===
void loop() {
  WiFiClient client = server.available();
  if (client) {
    Serial.println("New client connected");
    // Wait until client sends a request
    while (!client.available()) {
      delay(1);
    }

    String request = client.readStringUntil('\r');
    client.readStringUntil('\n'); // Clear next line

    if (request.indexOf("GET / ") != -1 || request.indexOf("GET /stream") != -1) {
      streamJpeg(client);
    } else {
      // Basic HTML page with embedded stream
      String html = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
      html += "<html><head><title>ESP32-CAM Stream</title></head><body>";
      html += "<h1>ESP32-CAM Video Stream</h1>";
      html += "<img src='/stream' style='width: 100%;'>";
      html += "</body></html>";
      client.print(html);
    }

    delay(1);
    client.stop();
    Serial.println("Client disconnected");
  }
}
