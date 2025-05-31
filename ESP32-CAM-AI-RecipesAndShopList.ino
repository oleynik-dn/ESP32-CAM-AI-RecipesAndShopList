// Подключение необходимых библиотек
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "Base64.h"
#include "esp_camera.h"
#include <WebServer.h>

extern "C" {
  #include "mbedtls/base64.h"
}

WebServer server(80);


//======================================== 
// Telegram
const char* BOT_TOKEN = "токен_бота";
const char* CHAT_ID = "id_чата";


// Данные для подключения к Wi-Fi
const char* ssid = "your_SSID"; // Замените на свои данные
const char* password = "your_PASSWORD"; // Замените на свои данные


// Данные для загрузки фото в Google Drive: "Deployment ID" GAS проекта и имя папки
String myDeploymentID = "Deployment_ID_of_your_application"; // Замените на свои данные
String myMainFolderName = "PhotosOfProducts";


// Переменная для настройки захвата фото со светодиодной вспышкой.
// Установите значение "false", тогда светодиод вспышки не будет загораться при захвате фото.
// Установите значение "true", тогда светодиод вспышки загорается при захвате фото.
bool LED_Flash_ON = true;
//========================================


// Определение GPIO-выводов для модуля камеры AI THINKER
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
 

// Определение GPIO для светодиодной вспышки
#define FLASH_LED_PIN 4


// Создание защищенного Wi-Fi клиента
WiFiClientSecure client;

// Функция проверки соединения с Google Apps Script
void Test_Con() {
  const char* host = "script.google.com";
  while(1) {
    Serial.println("-----------");
    Serial.println("Connection Test...");
    Serial.println("Connect to " + String(host));
  
    client.setInsecure();
  
    if (client.connect(host, 443)) {
      Serial.println("Connection successful.");
      Serial.println("-----------");
      client.stop();
      break;
    } else {
      Serial.println("Connected to " + String(host) + " failed.");
      Serial.println("Wait a moment for reconnecting.");
      Serial.println("-----------");
      client.stop();
    }
  
    delay(1000);
  }
}


// Функция захвата и отправки фото в Google Drive
void SendCapturedPhotos() {
  const char* host = "script.google.com";
  Serial.println();
  Serial.println("-----------");
  Serial.println("Connect to " + String(host));
  
  client.setInsecure();

  // Вспышка мигает, сигнализируя начало передачи
  digitalWrite(FLASH_LED_PIN, HIGH);
  delay(100);
  digitalWrite(FLASH_LED_PIN, LOW);
  delay(100);
  //---------------------------------------- 

  // Процесс подключения, захвата и отправки фотографий на Google Диск.
  if (client.connect(host, 443)) {
    Serial.println("Connection successful.");
    
    if (LED_Flash_ON == true) {
      digitalWrite(FLASH_LED_PIN, HIGH);
      delay(100);
    }

    // Захват фото
    Serial.println();
    Serial.println("Taking a photo...");
    
    for (int i = 0; i <= 3; i++) {
      camera_fb_t * fb = NULL;
      fb = esp_camera_fb_get();
       if(!fb) {
          Serial.println("Camera capture failed");
          Serial.println("Restarting the ESP32 CAM.");
          delay(1000);
          ESP.restart();
          return;
        } 
      esp_camera_fb_return(fb);
      delay(200);
    }
  
    camera_fb_t * fb = NULL;
    fb = esp_camera_fb_get();
    if(!fb) {
      Serial.println("Camera capture failed");
      Serial.println("Restarting the ESP32 CAM.");
      delay(1000);
      ESP.restart();
      return;
    } 
  
    if (LED_Flash_ON == true) digitalWrite(FLASH_LED_PIN, LOW);
    
    Serial.println("Taking a photo was successful.");
    //.............................. 

    // Отправка фото в Google Drive
    Serial.println();
    Serial.println("Sending image to Google Drive.");
    Serial.println("Size: " + String(fb->len) + "byte");
    
    String url = "/macros/s/" + myDeploymentID + "/exec?folder=" + myMainFolderName;

    client.println("POST " + url + " HTTP/1.1");
    client.println("Host: " + String(host));
    client.println("Transfer-Encoding: chunked");
    client.println();

    int fbLen = fb->len;
    char *input = (char *)fb->buf;
    int chunkSize = 3 * 1000; // должно быть кратно 3
    int chunkBase64Size = base64_enc_len(chunkSize);
    char output[chunkBase64Size + 1];

    Serial.println();
    int chunk = 0;
    for (int i = 0; i < fbLen; i += chunkSize) {
      int l = base64_encode(output, input, min(fbLen - i, chunkSize));
      client.print(l, HEX);
      client.print("\r\n");
      client.print(output);
      client.print("\r\n");
      delay(100);
      input += chunkSize;
      Serial.print(".");
      chunk++;
      if (chunk % 50 == 0) {
        Serial.println();
      }
    }
    client.print("0\r\n");
    client.print("\r\n");

    esp_camera_fb_return(fb);
    //.............................. 

    // Ожидание ответа
    Serial.println("Waiting for response.");
    long int StartTime = millis();
    while (!client.available()) {
      Serial.print(".");
      delay(100);
      if ((StartTime + 10 * 1000) < millis()) {
        Serial.println();
        Serial.println("No response.");
        break;
      }
    }
    Serial.println();
    while (client.available()) {
      Serial.print(char(client.read()));
    }
    //.............................. 

    // Светодиод вспышки мигает один раз, указывая на успешную отправку фотографий на Google Диск
    digitalWrite(FLASH_LED_PIN, HIGH);
    delay(500);
    digitalWrite(FLASH_LED_PIN, LOW);
    delay(500);
    //.............................. 
  }
  else {
    Serial.println("Connected to " + String(host) + " failed.");
    
    // Светодиод мигает дважды, указывая на сбой подключения
    digitalWrite(FLASH_LED_PIN, HIGH);
    delay(500);
    digitalWrite(FLASH_LED_PIN, LOW);
    delay(500);
    digitalWrite(FLASH_LED_PIN, HIGH);
    delay(500);
    digitalWrite(FLASH_LED_PIN, LOW);
    delay(500);
    //.............................. 
  }
  //---------------------------------------- 

  Serial.println("-----------");

  client.stop();
}
//________________________________________________________________________________ 

//________________________________________________________________________________ VOID SETUP()
void setup() {
  // Настройка конфигурации ESP32-CAM, выполняется один раз при запуске
  
  // Отключаем детектор падения напряжения (brownout detector), чтобы избежать сбоев
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  
  Serial.begin(115200);
  Serial.println();
  delay(1000);

  // Устанавливаем пин светодиода вспышки в режим выхода
  pinMode(FLASH_LED_PIN, OUTPUT);
  
  // Установка ESP32 в режим станции Wi-Fi
  Serial.println();
  Serial.println("Setting the ESP32 WiFi to station mode.");
  WiFi.mode(WIFI_STA);

  // Подключение к Wi-Fi точке доступа
  Serial.println();
  Serial.print("Connecting to : ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  
  // Время ожидания процесса подключения ESP32 CAM к точке доступа WiFi/маршрутизатору WiFi составляет 20 секунд.
  // Если в течение 20 секунд ESP32 CAM не будет успешно подключен к WiFi, ESP32 CAM перезапустится.
  // Я установил это условие, потому что на моем ESP32-CAM бывают моменты, когда кажется, что он не может подключиться к WiFi, поэтому его нужно перезапустить, чтобы подключиться к WiFi.
  int connecting_process_timed_out = 20; // 20 секунд
  connecting_process_timed_out = connecting_process_timed_out * 2;
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    digitalWrite(FLASH_LED_PIN, HIGH);
    delay(250);
    digitalWrite(FLASH_LED_PIN, LOW);
    delay(250);
    if(connecting_process_timed_out > 0) connecting_process_timed_out--;
    if(connecting_process_timed_out == 0) {
      Serial.println();
      Serial.print("Failed to connect to ");
      Serial.println(ssid);
      Serial.println("Restarting the ESP32 CAM.");
      delay(1000);
      ESP.restart();
    }
  }

  // Отключаем светодиод после успешного подключения
  digitalWrite(FLASH_LED_PIN, LOW);
  
  Serial.println();
  Serial.print("Successfully connected to ");
  Serial.println(ssid);
  Serial.print("IP-адрес ESP32-CAM: ");
  Serial.println(WiFi.localIP());
  //---------------------------------------- 

  // Настройка камеры ESP32-CAM
  Serial.println();
  Serial.println("Set the camera ESP32 CAM...");
  
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
  
  // Инициализация камеры с высоким качеством, если доступна PSRAM
  if(psramFound()){
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;  // 0-63, чем ниже число, тем выше качество
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 8;  
    config.fb_count = 1;
  }
  
  // Инициализация камеры
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    Serial.println();
    Serial.println("Restarting the ESP32 CAM.");
    delay(1000);
    ESP.restart();
  }

  sensor_t * s = esp_camera_sensor_get();

  // При необходимости, поменяйте разрешение камеры:
  // -UXGA   = 1600 x 1200 pixels
  // -SXGA   = 1280 x 1024 pixels
  // -XGA    = 1024 x 768  pixels
  // -SVGA   = 800 x 600   pixels
  // -VGA    = 640 x 480   pixels
  // -CIF    = 352 x 288   pixels
  // -QVGA   = 320 x 240   pixels
  // -HQVGA  = 240 x 160   pixels
  // -QQVGA  = 160 x 120   pixels
  s->set_framesize(s, FRAMESIZE_SXGA);  //--> UXGA|SXGA|XGA|SVGA|VGA|CIF|QVGA|HQVGA|QQVGA

  Serial.println("Setting the camera successfully.");
  Serial.println();

  delay(1000);

  Test_Con();

  Serial.println();
  Serial.println("ESP32-CAM captures and sends photos to the server every 20 seconds.");
  Serial.println();
  delay(2000);


  // обработка входящего HTTP-запроса
  server.on("/on", handleOn);
  server.begin();
  Serial.println("HTTP server started");

}


// Функция отправки Telegram-сообщения
void sendTelegramMessage(String message) {
  WiFiClientSecure tgClient;
  tgClient.setInsecure();
  if (!tgClient.connect("api.telegram.org", 443)) {
    Serial.println("Telegram connection failed");
    return;
  }

  String url = "/bot" + String(BOT_TOKEN) + "/sendMessage";
  String data = "chat_id=" + String(CHAT_ID) + "&text=" + urlencode(message);

  tgClient.println("POST " + url + " HTTP/1.1");
  tgClient.println("Host: api.telegram.org");
  tgClient.println("Content-Type: application/x-www-form-urlencoded");
  tgClient.print("Content-Length: ");
  tgClient.println(data.length());
  tgClient.println();
  tgClient.print(data);

  while (tgClient.connected()) {
    String line = tgClient.readStringUntil('\n');
    if (line == "\r") break;
  }

  Serial.println("Telegram response:");
  Serial.println(tgClient.readString());
}

String urlencode(String str) {
  String encoded = "";
  char c;
  char code0, code1;
  for (int i = 0; i < str.length(); i++) {
    c = str.charAt(i);
    if (isalnum(c)) {
      encoded += c;
    } else {
      code0 = (c >> 4) & 0xF;
      code1 = c & 0xF;
      encoded += '%';
      encoded += "0123456789ABCDEF"[code0];
      encoded += "0123456789ABCDEF"[code1];
    }
  }
  return encoded;
}




// Обработчик команды /on
void handleOn() {
  server.send(200, "text/plain", "Снимок выполняется...");
  Serial.println("/on triggered");
  SendCapturedPhotos();  // отправляем снимок на диск
  sendTelegramMessage("📸 Снимок выполнен и отправлен");
}




// тут код для многократного выполнения
void loop() {

  // проверка и обработка новых входящих запросов
  server.handleClient();
}
