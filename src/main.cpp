#include <Arduino.h>
#include <headers.h>
#include <credentials.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <time.h>

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
unsigned long lastTime = 0;
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

	WiFi.onEvent([](WiFiEvent_t event) {
    	if (event == ARDUINO_EVENT_WIFI_STA_DISCONNECTED) {
        	Serial.println("WiFi disconnected, reconnecting...");
        	WiFi.reconnect();
    	}
	});

    setupWifi();
    setupServer();
	setupTime();
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
    Serial.println("\nIP: http://" + WiFi.localIP().toString());
}

void setupServer() {
    server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");
    server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request) {
        String json = "{";
        json += "\"temperatuur\":" + String(bme.readTemperature(), 1) + ",";
        json += "\"luchtvochtigheid\":" + String(bme.readHumidity(), 0) + ",";
        json += "\"luchtdruk\":" + String(bme.readPressure() / 100.0F, 0) + ",";
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
            json += String(humidityHistory[(start + i) % DATAPOINTS], 0);
        }
        json += "],\"pressureHistory\":[";
        for (int i = 0; i < count; i++) {
            if (i > 0) json += ",";
            json += String(pressureHistory[(start + i) % DATAPOINTS], 0);
        }
        json += "],\"timestamps\":[";
        for (int i = 0; i < count; i++) {
            if (i > 0) json += ",";
            json += String(timestamps[(start + i) % DATAPOINTS]);
        }
        json += "]}";
        request->send(200, "application/json", json);
    });

    server.begin();
    Serial.println("HTTP server started");
}

void setupTime() {
	configTime(3600, 3600, "pool.ntp.org");
    Serial.print("Syncing time");
    while (time(nullptr) < 1000000000) {
        delay(500);
        Serial.print(".");
    }
    Serial.println(" OK");
}

void BMEValues() {
    float temperature = bme.readTemperature();
    float humidity = bme.readHumidity();
    float pressure = bme.readPressure() / 100.0F;

    temperatureHistory[historyIndex % DATAPOINTS] = temperature;
    humidityHistory[historyIndex % DATAPOINTS] = humidity;
    pressureHistory[historyIndex % DATAPOINTS] = pressure;

    timestamps[historyIndex % DATAPOINTS] = time(nullptr);
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