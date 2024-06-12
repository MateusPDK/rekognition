#include "esp_camera.h"
#include <WiFi.h>
#include "base64.h" // Biblioteca para conversão base64
#include <HTTPClient.h>

// Defina as configurações da câmera
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

// Defina o nome e a senha da rede Wi-Fi
// const char* ssid = "Podgorski";
// const char* password = "p1122334455";

const char* ssid = "Moto G (5) Plus 2894";
const char* password = "m1122334455";

// Defina a URL da função Lambda
const char* lambda_url = "https://zqfip4fa2ijubrew2ylxpri57m0iqbjm.lambda-url.us-east-1.on.aws";

void setup() {
  Serial.begin(115200);
  
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
  
  // Inicializa a câmera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Falha na inicialização da câmera com erro 0x%x", err);
    return;
  }

  Serial.println("Inicializando a conexão Wi-Fi...");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Tentando conectar à rede Wi-Fi...");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Conectado com sucesso à rede Wi-Fi!");
    Serial.print("Endereço IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("Falha ao conectar à rede Wi-Fi.");
    return;
  }

  // Tirar uma foto
  camera_fb_t * fb = NULL;
  fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Falha ao capturar imagem");
    return;
  }

  // Converter a imagem para base64
  String base64Image = base64::encode((uint8_t *)fb->buf, fb->len);

  // Printar a imagem em base64 no console
  Serial.println("Fazendo o upload da imagem...");

  // Configurar o cliente HTTP
  HTTPClient http;
  http.begin(lambda_url); // No need to disable SSL/TLS verification if the URL is correct
  http.addHeader("Content-Type", "text/plain");

  // Enviar a imagem em base64
  int httpResponseCode = http.POST(base64Image);

  // Verificar o resultado da solicitação HTTP
  if (httpResponseCode > 0) {
    Serial.printf("Imagem enviada com sucesso. Código HTTP: %d\n", httpResponseCode);
    String response = http.getString();
    Serial.println("Resposta do servidor:");
    Serial.println(response);
  } else {
    Serial.printf("Erro ao enviar a imagem. Código HTTP: %d\n", httpResponseCode);
    Serial.println(http.errorToString(httpResponseCode));
  }

  // Finalizar a solicitação HTTP
  http.end();

  // Liberar a memória da imagem
  esp_camera_fb_return(fb);
}

void loop() {
  // Nada para fazer aqui
}
