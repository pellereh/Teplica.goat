/**
 * Умная теплица на ESP32
 * Управление: полив, вентиляция, форточка, освещение
 * Мониторинг: температура, влажность, давление, освещенность
 */
#include <WiFi.h>           // WiFi
#include <AsyncTCP.h>       // Асинхронный TCP
#include <ESPAsyncWebServer.h> // Веб-сервер
#include <ESP32Servo.h>     // Сервопривод
#include <FastLED.h>        // RGB-светодиоды
#include <Adafruit_BME280.h> // Датчик темп/влаж/давл
#include <Wire.h>           // I2C
#include <BH1750.h>         // Датчик освещенности
#include "page.h"           // HTML-страница

// Прототип функции FillSolidColor
void FillSolidColor(uint32_t c);

// Настройки WiFi
const char* ap_ssid = "ESP32_AP";
const char* ap_password = "12345678";

// Состояния устройств
bool pumpState = false;    // Насос
bool windState = false;    // Вентилятор
bool windowState = false;  // Форточка
bool lightState = false;   // Освещение

// GPIO выводы
const int pumpPin = 17;    // Насос
const int windPin = 16;    // Вентилятор
const int windowPin = 19;  // Сервопривод форточки
const int lightPin = 18;   // Освещение

// Объект для управления сервоприводом форточки
Servo servoWindow;

// Настройки RGB-ленты
#define LED_PIN     18     // Пин данных для подключения RGB-ленты
#define LED_COUNT   64     // Количество светодиодов в ленте
#define LED_TYPE    SK6812 // Тип используемой RGB-ленты
CRGB leds[LED_COUNT];      // Массив для хранения состояния каждого светодиода

// Инициализация датчиков
BH1750 lightSensor;        // Датчик освещенности BH1750
Adafruit_BME280 bme;       // Датчик температуры, влажности и давления BME280

// Создание объекта веб-сервера на порту 80
AsyncWebServer server(80);

/**
 * Функция преобразования шестнадцатеричного представления цвета в RGB
 * @param hexColor Строка с шестнадцатеричным представлением цвета (например, "#FF0000" для красного)
 * @return Целочисленное представление цвета в формате GRB (для FastLED)
 */
uint32_t HexToRGB(String hexColor) {
    if (hexColor.startsWith("#")) {
        hexColor = hexColor.substring(1); // Убираем символ #
    }
    uint32_t red = strtol(&hexColor[0], NULL, 16);   // Красный компонент
    uint32_t green = strtol(&hexColor[2], NULL, 16); // Зеленый компонент
    uint32_t blue = strtol(&hexColor[4], NULL, 16);  // Синий компонент

    // Для FastLED нужно поменять порядок на GRB
    return (green << 16) | (red << 8) | blue;
}

/**
 * Функция для заполнения всей RGB-ленты одним цветом
 * @param c Цвет в формате GRB для FastLED
 */
void FillSolidColor(uint32_t c) {
    for (int i = 0; i < LED_COUNT; i++) {
        leds[i] = c;
    }
    FastLED.show(); // Применение изменений и отображение цвета на ленте
}

/**
 * Функция начальной настройки
 * Вызывается один раз при старте системы
 */
void setup() {
    // Инициализация последовательного порта для отладки
    Serial.begin(115200);
    Serial.println("Starting...");

    // Инициализация шины I2C для работы с датчиками
    Wire.begin(21, 22); // GP21 - SDA, GP22 - SCL (линии данных I2C)

    // Инициализация и проверка датчика освещенности
    if (!lightSensor.begin()) {
        Serial.println("Ошибка инициализации BH1750");
    } else {
        Serial.println("BH1750 инициализирован");
    }
    
    // Инициализация и проверка датчика температуры/влажности/давления
    // 0x77 - I2C-адрес датчика BME280
    if (!bme.begin(0x77)) { 
        Serial.println("Ошибка инициализации BME280");
    } else {
        Serial.println("BME280 инициализирован");
    }

    // Настройка выводов GPIO для управления устройствами
    pinMode(pumpPin, OUTPUT);   // Настройка пина насоса как выход
    pinMode(windPin, OUTPUT);   // Настройка пина вентилятора как выход
    pinMode(lightPin, OUTPUT);  // Настройка пина освещения как выход
    // Установка начальных состояний (все выключено)
    digitalWrite(pumpPin, LOW);
    digitalWrite(windPin, LOW);
    digitalWrite(lightPin, LOW);

    // Создание точки доступа WiFi
    if (WiFi.softAP(ap_ssid, ap_password)) {
        Serial.println("Access Point started.");
        Serial.print("Connect to SSID: ");
        Serial.println(ap_ssid);
        Serial.print("Password: ");
        Serial.println(ap_password);
        Serial.println("IP address: 192.168.4.1"); // IP-адрес по умолчанию для точки доступа
    } else {
        Serial.println("Failed to create Access Point.");
        return;
    }

    // Инициализация сервопривода для форточки
    servoWindow.attach(windowPin);  // Привязка сервопривода к соответствующему пину
    servoWindow.write(0);           // Установка начального положения (форточка закрыта)

    // Инициализация RGB-ленты
    FastLED.addLeds<LED_TYPE, LED_PIN, GRB>(leds, LED_COUNT); // Настройка типа ленты и порядка цветов
    FastLED.setBrightness(50);                                // Начальная яркость 50/255
    FillSolidColor(0xFFFFFF);                                 // Установка белого цвета

    // Настройка маршрутов веб-сервера
    
    // Обработка запросов к главной странице
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        String page = PAGE; // Получение шаблона HTML-страницы

        // Получение данных с датчиков
        float temperature = bme.readTemperature();    // Чтение температуры в °C
        float humidity = bme.readHumidity();          // Чтение относительной влажности в %
        float pressure = bme.readPressure() / 100.0F; // Чтение давления в гПа
        float lux = lightSensor.readLightLevel();     // Чтение уровня освещенности в люксах

        // Замена плейсхолдеров в HTML-шаблоне на актуальные значения
        page.replace("%TEMPERATURE%", String(temperature, 1)); // Температура с одним знаком после запятой
        page.replace("%HUMIDITY%", String(humidity, 1));       // Влажность с одним знаком после запятой
        page.replace("%PRESSURE%", String(pressure, 1));       // Давление с одним знаком после запятой
        page.replace("%LUX%", String(lux, 1));                 // Освещенность с одним знаком после запятой
        // Установка состояний переключателей в зависимости от текущего состояния устройств
        page.replace("%WINDOW_STATE%", windowState ? "checked" : "");
        page.replace("%PUMP_STATE%", pumpState ? "checked" : "");
        page.replace("%WIND_STATE%", windState ? "checked" : "");
        page.replace("%LIGHT_STATE%", lightState ? "checked" : "");
        page.replace("%BRIGHTNESS%", String(FastLED.getBrightness())); // Текущая яркость освещения

        // Отправка HTML-страницы клиенту
        request->send(200, "text/html", page);
    });

    // API-маршрут для получения актуальных данных с датчиков в формате JSON
    server.on("/sensor/data", HTTP_GET, [](AsyncWebServerRequest *request) {
        float temperature = bme.readTemperature();
        float humidity = bme.readHumidity();
        float pressure = bme.readPressure() / 100.0F;
        float lux = lightSensor.readLightLevel();

        // Формирование JSON-ответа с данными
        String jsonResponse = "{";
        jsonResponse += "\"temperature\":" + String(temperature, 1) + ",";
        jsonResponse += "\"humidity\":" + String(humidity, 1) + ",";
        jsonResponse += "\"pressure\":" + String(pressure, 1) + ",";
        jsonResponse += "\"lux\":" + String(lux, 1);
        jsonResponse += "}";

        request->send(200, "application/json", jsonResponse);
    });

    // Маршруты для управления насосом
    
    // Включение насоса
    server.on("/pump/on", HTTP_GET, [](AsyncWebServerRequest *request) {
        digitalWrite(pumpPin, HIGH); // Включаем насос
        pumpState = true;           // Обновляем состояние
        Serial.println("Насос ВКЛ");
        request->send(200, "text/plain", "OK");
    });

    // Выключение насоса
    server.on("/pump/off", HTTP_GET, [](AsyncWebServerRequest *request) {
        digitalWrite(pumpPin, LOW);  // Выключаем насос
        pumpState = false;          // Обновляем состояние
        Serial.println("Насос ВЫКЛ");
        request->send(200, "text/plain", "OK");
    });

    // Маршруты для управления вентилятором
    
    // Включение вентилятора
    server.on("/wind/on", HTTP_GET, [](AsyncWebServerRequest *request) {
        digitalWrite(windPin, HIGH); // Включаем вентилятор
        windState = true;           // Обновляем состояние
        Serial.println("Вентилятор ВКЛ");
        request->send(200, "text/plain", "OK");
    });

    // Выключение вентилятора
    server.on("/wind/off", HTTP_GET, [](AsyncWebServerRequest *request) {
        digitalWrite(windPin, LOW);  // Выключаем вентилятор
        windState = false;          // Обновляем состояние
        Serial.println("Вентилятор ВЫКЛ");
        request->send(200, "text/plain", "OK");
    });

    // Маршруты для управления форточкой
    
    // Открытие форточки
    server.on("/window/open", HTTP_GET, [](AsyncWebServerRequest *request) {
        servoWindow.write(90);      // Устанавливаем сервопривод в положение 90° (форточка открыта)
        windowState = true;         // Обновляем состояние
        Serial.println("Форточка ОТКРЫТА");
        request->send(200, "text/plain", "OK");
    });

    // Закрытие форточки
    server.on("/window/close", HTTP_GET, [](AsyncWebServerRequest *request) {
        servoWindow.write(0);       // Устанавливаем сервопривод в положение 0° (форточка закрыта)
        windowState = false;        // Обновляем состояние
        Serial.println("Форточка ЗАКРЫТА");
        request->send(200, "text/plain", "OK");
    });

    // Маршруты для управления освещением
    
    // Включение освещения
    server.on("/light/on", HTTP_GET, [](AsyncWebServerRequest *request) {
        digitalWrite(lightPin, HIGH); // Включаем свет (для обычного освещения)
        lightState = true;           // Обновляем состояние
        FastLED.setBrightness(255);  // Устанавливаем максимальную яркость RGB-ленты
        FastLED.show();              // Обновляем состояние RGB-ленты
        Serial.println("Свет ВКЛ");
        request->send(200, "text/plain", "OK");
    });

    // Выключение освещения
    server.on("/light/off", HTTP_GET, [](AsyncWebServerRequest *request) {
        digitalWrite(lightPin, LOW);  // Выключаем свет (для обычного освещения)
        lightState = false;          // Обновляем состояние
        FastLED.setBrightness(0);    // Устанавливаем минимальную яркость RGB-ленты (выключаем)
        FastLED.show();              // Обновляем состояние RGB-ленты
        Serial.println("Свет ВЫКЛ");
        request->send(200, "text/plain", "OK");
    });

    // Регулировка яркости освещения
    server.on("/light/brightness/", HTTP_GET, [=](AsyncWebServerRequest *request) {
        if (request->hasParam("value")) {
            AsyncWebParameter* p = request->getParam("value");
            int newBrightness = p->value().toInt();              // Получаем значение из параметра запроса
            int constrainedBrightness = constrain(newBrightness, 0, 100); // Ограничиваем значение от 0 до 100
            FastLED.setBrightness(map(constrainedBrightness, 0, 100, 0, 255)); // Преобразуем значение от 0-100 к 0-255
            FastLED.show();                                      // Применяем новую яркость
            Serial.print("Яркость: ");
            Serial.println(constrainedBrightness);
        }
        request->send(200, "text/plain", "OK");
    });

    // Запуск веб-сервера
    server.begin();
    Serial.println("Веб-сервер запущен. Подключитесь к точке доступа и откройте 192.168.4.1 в браузере");
}

/**
 * Основной цикл программы
 * В данной реализации пуст, так как вся обработка происходит 
 * асинхронно через обработчики запросов веб-сервера
 */
void loop() {
    // Ничего не делаем в основном цикле, так как используем асинхронный подход
    // При необходимости здесь можно добавить периодические операции, 
    // например, чтение датчиков и автоматическое управление устройствами
}