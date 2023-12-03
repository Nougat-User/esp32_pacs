#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>

#define SS_PIN 21
#define RST_PIN 22 
#define LED_PIN 2
#define INIT_BLINK_DELAY 500
#define WRONG_BLINK_DELAY 100

MFRC522 rfid(SS_PIN, RST_PIN); // Инициализация RC522


// ------------------ МАССИВ КЛЮЧЕЙ ----------------------
byte key1[] = {0x7A, 0x5D, 0xC4, 0x1F}; // UID ключа
String key1Name = "Blue Key"; // Название ключа

byte key2[] = {0x01, 0xCE, 0x3D, 0x88}; // UID ключа
String key2Name = "Red Key"; // Название ключа

byte key3[] = {0xAE, 0x36, 0x34, 0xDA}; // UID ключа
String key3Name = "Alex Key"; // Название ключа
// ------------------ МАССИВ КЛЮЧЕЙ ----------------------


// SSID и пароль для Wifi
const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";

// Открытие веб сервера
AsyncWebServer server(80);



// Счетчик открытия двери
unsigned long openTime = 0;

// Таймер открытия двери в мс
unsigned long timer = 5000;

// Функция переключения светодиода
void toggleLED(bool state) {
  digitalWrite(LED_PIN, state ? HIGH : LOW);
}


// Функция светодиода для инициализации СКУД
void blink_init() {
  toggleLED(LOW);
  toggleLED(HIGH);
  delay(INIT_BLINK_DELAY);
  toggleLED(LOW);
  delay(INIT_BLINK_DELAY);
  toggleLED(HIGH);
  delay(INIT_BLINK_DELAY);
  toggleLED(LOW);
}


// Функция светодиода для неправильного ключа
void blink_wrong() {
  toggleLED(LOW);
  toggleLED(HIGH);
  delay(WRONG_BLINK_DELAY);
  toggleLED(LOW);
  delay(WRONG_BLINK_DELAY);
  toggleLED(HIGH);
  delay(WRONG_BLINK_DELAY);
  toggleLED(LOW);
}

// Функция чтения UID ключа
void printCardUID() {
  for (int i = 0; i < rfid.uid.size; i++) {
    Serial.print(rfid.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(rfid.uid.uidByte[i], HEX);
  }
  Serial.println();
}

// Функция корня веб сервера
void handleRoot(AsyncWebServerRequest *request) {
  String message = "ESP32 SKUD";
  message += "\n\nTo open move here: /open";
  request->send(200, "text/plain", message);
}

// Функция открывания двери по Wifi
void handleOpenDoor(AsyncWebServerRequest *request) {
    handleAccessGranted();
    request->send(200, "text/plain", "OPENED...");
}



void setup() {
  Serial.begin(9600);
  SPI.begin();
  rfid.PCD_Init();
  pinMode(LED_PIN, OUTPUT);
  blink_init();
  Serial.println("ESP32 СКУД");
  Serial.println();
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Подключение к Wifi...");
  }
  Serial.print("Подключено... ");
  Serial.print(WiFi.localIP());
  Serial.println();
  // Пути сервера
  server.on("/", HTTP_GET, handleRoot);
  server.on("/open", HTTP_GET, handleOpenDoor);
  server.begin();

}

void loop() {
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    Serial.print("\n---------\n");
    Serial.print("UID: ");
    printCardUID();

    // ------------------ ПРОВЕРКА КЛЮЧЕЙ ----------------------
    if (compareKeys(rfid.uid.uidByte, key1)) {
      Serial.println();
      Serial.print("Доступ разрешен - ");
      Serial.print(key1Name);
      Serial.println();
      handleAccessGranted();
    }
    // ------------------ ПРОВЕРКА КЛЮЧЕЙ ----------------------

    
    else {
      Serial.println("Доступ запрещен.\n---------");
      blink_wrong();
    }

    delay(1000); // Задержка для избежания считывания карты несколько раз подряд
  }

  // Закрываем дверь после истечения времени открытия
  if (openTime > 0 && millis() - openTime > timer) {
    Serial.println();
    Serial.print("Дверь закрыта.\n---------");
    toggleLED(LOW); // Выключаем светодиод после закрытия двери
    openTime = 0;
  }
}

// Функция для сравнения ключей
bool compareKeys(byte cardKey[], byte storedKey[]) {
  for (int i = 0; i < 4; i++) {
    if (cardKey[i] != storedKey[i]) {
      return false; // Ключи не совпадают
    }
  }
  return true; // Ключи совпадают
}

// Функция для обработки успешного открытия двери
void handleAccessGranted() {
  openTime = millis();
  Serial.print("Дверь открыта на ");
  Serial.print(timer / 1000);
  Serial.print(" секунд.");
  toggleLED(HIGH); // Включаем светодиод при открытии двери
}
