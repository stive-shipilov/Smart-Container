#include "HX711.h"
#include <ESP8266WiFi.h>
#include <Ticker.h> // Используем библиотеку Ticker

String apiKey = "********"; 
const char *ssid = "********"; 
const char *pass = "********";
const char* server = "api.thingspeak.com";

WiFiClient client;
HX711 scale(D5, D6);

const int buttonPin = D3;  // Подключаем KY-004 к D3 (GPIO0)
volatile bool resetScale = false;  // Флаг для обработки нажатия кнопки

float weight;
float calibration_factor = -425975; 

// Таймер для отправки данных
Ticker timer;

// Функция обработки прерывания
void IRAM_ATTR buttonPress() {
  resetScale = true;  // Устанавливаем флаг
}

// Функция отправки данных на ThingSpeak
void sendData() {
  // Чтение веса
  scale.set_scale(calibration_factor);
  weight = scale.get_units(); 
  
  // Выводим значение веса в консоль
  Serial.print("Weight: ");
  Serial.print(weight, 8);  // 8 знаков после запятой
  Serial.println(" KG");

  // Подключение к серверу ThingSpeak
  if (client.connect(server, 80)) {
    String postStr = apiKey;
    postStr += "&field1=";
    postStr += String(weight, 5);  // Отправляем вес с точностью до 5 знаков
    postStr += "\r\n\r\n";

    // HTTP-запрос для отправки данных
    client.print("POST /update HTTP/1.1\n");
    client.print("Host: api.thingspeak.com\n");
    client.print("Connection: close\n");
    client.print("X-THINGSPEAKAPIKEY: " + apiKey + "\n");
    client.print("Content-Type: application/x-www-form-urlencoded\n");
    client.print("Content-Length: ");
    client.print(postStr.length());
    client.print("\n\n");
    client.print(postStr);

    Serial.println("📤 Данные отправлены!");

    // Ждем ответа от сервера
    while (client.available()) {
      String line = client.readStringUntil('\n');
      Serial.println(line);
      if (line.startsWith("HTTP/1.1 200 OK")) {
        Serial.println("✅ Данные успешно загружены в ThingSpeak!");
        break;
      }
    }
  } else {
    Serial.println("❌ Ошибка подключения к ThingSpeak!");
  }
  client.stop();  // Закрытие соединения с сервером
}

void setup() {
  Serial.begin(115200);
  
  // Подключаем кнопку с подтягивающим резистором к 3.3V
  pinMode(buttonPin, INPUT_PULLUP);  // Включаем внутренний подтягивающий резистор
  attachInterrupt(digitalPinToInterrupt(buttonPin), buttonPress, FALLING); // Прерывание при нажатии кнопки
  
  scale.set_scale();
  scale.tare();  // Сбрасываем весы

  // Подключение к Wi-Fi
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");

  // Запускаем таймер для отправки данных каждые 15 секунд
  timer.attach(15, sendData);  // Время в секундах для отправки данных
}

void loop() {
  // Если кнопка была нажата, сбрасываем весы
  if (resetScale) {  
    scale.set_scale();
    scale.tare();  
    Serial.println("Весы сброшены!");
    resetScale = false;  // Сбрасываем флаг
  }

  // Цикл не блокирует работу, так как таймер управляет отправкой данных
  Serial.println("Waiting...");
  delay(1000);  // Задержка 1 секунда для стабильности цикла
}