#include <Arduino.h>
#include <headers.h>
#include <credentials.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>

#define BME_SCL 22
#define BME_SDA 21

#define DATAPOINTS 168 // 168 for 7 days

Adafruit_BME280 bme;
AsyncWebServer server(80);

float temperatureHistory[DATAPOINTS];
float humidityHistory[DATAPOINTS];
float pressureHistory[DATAPOINTS];
int historyIndex = 0;
unsigned long startTime = 0;
unsigned long lastTime;
unsigned long currentTime;
unsigned long delayTime = 1000; // 3600000 for 1 hour
unsigned long timestamps[DATAPOINTS];

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println(F("Waterboei"));
    Wire.begin(BME_SDA, BME_SCL);

    if (!bme.begin(0x76, &Wire)) { // could be 0x77
        Serial.println("Could not find a valid BME280 sensor, check wiring!");
        while (true) {
            delay(1000);
        }
    }

    if (!LittleFS.begin()) {
        Serial.println("Failed to initialize LittleFS");
        return;
    }

    setupWifi();
    setupServer();
    startTime = millis();
    lastTime = millis();
}

void loop() {
    currentTime = millis();
    if (currentTime - lastTime >= delayTime) {
        lastTime = currentTime;
        BMEValues();
    }
}

void setupWifi() {
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Connecting");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nIP: " + WiFi.localIP().toString());
}

void setupServer() {
    server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");
    server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request) {
        String json = "{";
        json += "\"temperatuur\":" + String(bme.readTemperature(), 1) + ",";
        json += "\"luchtvochtigheid\":" + String(bme.readHumidity(), 1) + ",";
        json += "\"luchtdruk\":" + String(bme.readPressure() / 100.0F, 1) + ",";
        json += "\"temperatureHistory\":[";
        int count = min(historyIndex, DATAPOINTS);
        int start = historyIndex >= DATAPOINTS ? historyIndex % DATAPOINTS : 0;
        for (int i = 0; i < count; i++) {
            if (i > 0) json += ",";
            json += String(temperatureHistory[(start + i) % DATAPOINTS], 1);
        }
        json += "],\"humidityHistory\":[";
        for (int i = 0; i < count; i++) {
            if (i > 0) json += ",";
            json += String(humidityHistory[(start + i) % DATAPOINTS], 1);
        }
        json += "],\"pressureHistory\":[";
        for (int i = 0; i < count; i++) {
            if (i > 0) json += ",";
            json += String(pressureHistory[(start + i) % DATAPOINTS], 1);
        }
        json += "],\"timestamps\":[";
        for (int i = 0; i < count; i++) {
            if (i > 0) json += ",";
            json += String(timestamps[i]);
        }
        json += "]}";
        request->send(200, "application/json", json);
    });

    server.begin();
    Serial.println("HTTP server started");
}

void BMEValues() {
    float temperature = bme.readTemperature();
    float humidity = bme.readHumidity();
    float pressure = bme.readPressure();

    temperatureHistory[historyIndex % DATAPOINTS] = temperature;
    humidityHistory[historyIndex % DATAPOINTS] = humidity;
    pressureHistory[historyIndex % DATAPOINTS] = pressure;

    timestamps[historyIndex % DATAPOINTS] = (millis() - startTime) / 1000; // 3600000 for 1 hour
    historyIndex++;

    // print sensors to serial
    // Serial.print("Temperatuur: ");
    // Serial.print(temperature);
    // Serial.println("°C");
    // Serial.print("Luchtvochtigheid: ");
    // Serial.print(humidity);
    // Serial.println("%");
    // Serial.print("Luchtdruk: ");
    // Serial.print(pressure / 100.0F);
    // Serial.println("hPa");
}