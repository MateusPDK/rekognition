#include "esp_camera.h"
#include <WiFi.h>
#include "base64.h"
#include <HTTPClient.h>

// Camera configuration settings
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

// WiFi credentials
const char* ssid = "Podgorski";
const char* password = "p1122334455";

// Lambda function URL
const char* lambda_url = "https://zqfip4fa2ijubrew2ylxpri57m0iqbjm.lambda-url.us-east-1.on.aws";

void setup() {
  Serial.begin(115200);
  
  // Camera configuration
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

  if(psramFound()){
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  // Initialize the camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  // Warm up time for the camera
  delay(2000);

  // WiFi connection
  Serial.println("Connecting to Wi-Fi...");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to Wi-Fi...");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Connected to Wi-Fi!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("Failed to connect to Wi-Fi.");
    return;
  }

  // Capture image
  camera_fb_t * fb = NULL;
  fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    return;
  }

  // Check image quality and size
  Serial.printf("Image taken! Size: %u bytes\n", fb->len);
  
  // Convert image to base64
  String base64Image = base64::encode((uint8_t *)fb->buf, fb->len);

  // Print base64 image (optional for debugging)
  // Serial.println("Base64 image:");
  // Serial.println(base64Image);

  // HTTP POST request
  Serial.println("Uploading image...");

  HTTPClient http;
  http.begin(lambda_url);
  http.addHeader("Content-Type", "text/plain");

  int httpResponseCode = http.POST(base64Image);

  if (httpResponseCode > 0) {
    Serial.printf("Image uploaded successfully. HTTP Response code: %d\n", httpResponseCode);
    String response = http.getString();
    Serial.println("Server response:");
    Serial.println(response);
  } else {
    Serial.printf("Failed to upload image. HTTP Response code: %d\n", httpResponseCode);
    Serial.println(http.errorToString(httpResponseCode));
  }

  http.end();
  
  // Return the frame buffer back to the driver for reuse
  esp_camera_fb_return(fb);
}

void loop() {
  // No action in loop
}
