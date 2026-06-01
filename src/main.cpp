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
#include <OneWire.h>
#include <DallasTemperature.h>

#define BME_SCL 22
#define BME_SDA 21
#define ONE_WIRE_BUS 16
#define turbidity_pin 34

#define DATAPOINTS 168 // 168 for 7 days

// initialise sensors and webserver
Adafruit_BME280 bme;
AsyncWebServer server(80);
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensor(&oneWire);

// sensor datapoint arrays
float temperatureHistory[DATAPOINTS];
float humidityHistory[DATAPOINTS];
float pressureHistory[DATAPOINTS];
float waterTemperatureHistory[DATAPOINTS];
int turbidityHistory[DATAPOINTS];

unsigned long timestamps[DATAPOINTS];
int historyIndex = 0;

// millis
unsigned long startTime = 0;
unsigned long lastTime = 0;
unsigned long currentTime;
unsigned long delayTime = 60000; // 3600000 for 1 hour

// sensors
float temperature;
float humidity;
float pressure;
float waterTemperature;
int turbidity;

// risk
float tempDiff;
int swimRisk = 0;
int fishRisk = 0;
byte riskPrediction = 0;

void setup() {
    Serial.begin(115200);
    delay(1000); // wait a second for serial monitor
    Serial.println(F("starting waterboei"));

    pinMode(turbidity_pin, INPUT);

	WiFi.onEvent([](WiFiEvent_t event) {
    	if (event == ARDUINO_EVENT_WIFI_STA_DISCONNECTED) {
        	Serial.println("WiFi disconnected, reconnecting...");
        	WiFi.reconnect();
    	}
	});

    setupBME();
    setupLittleFS();
    setupWifi();
    setupTime();
    setupServer();

    startTime = millis();
    lastTime = millis();
}

void loop() {
    currentTime = millis();
    if (currentTime - lastTime >= delayTime) {
        lastTime = currentTime;
        BMEValues();
        waterTempValues();
        turbidityValues();
        riskCalculations();

        timestamps[historyIndex % DATAPOINTS] = time(nullptr);
        historyIndex++;
    }
}

void setupBME() {
    Wire.begin(BME_SDA, BME_SCL);

    if (!bme.begin(0x76, &Wire)) { // could be 0x77
        Serial.println("Could not find a valid BME280 sensor, check wiring!");
    }
    Serial.println("BME280 sensor found");
}

void setupLittleFS() {
    if (!LittleFS.begin()) {
        Serial.println("Failed to initialize LittleFS");
        while (true) {
            delay(500);
        }
    }
    Serial.println("LittleFS initialized");
}

void setupWifi() {
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Connecting");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nIP: http://" + WiFi.localIP().toString() + "/");
}

void setupServer() {
    server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");
    server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request) {
        String json = "{";
        json += "\"temperatuur\":" + String(temperature, 1) + ",";
        json += "\"luchtvochtigheid\":" + String(humidity, 0) + ",";
        json += "\"luchtdruk\":" + String(pressure, 0) + ",";
        json += "\"watertemperatuur\":" + String(waterTemperature, 1) + ",";
        json += "\"troebelheid\":" + String(turbidity) + ",";
        json += "\"zwemrisico\":" + String(swimRisk) + ",";
        json += "\"visrisico\":" + String(fishRisk) + ",";
        json += "\"risicostijging\":" + String(riskPrediction) + ",";
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
        json += "],\"waterTemperatureHistory\":[";
        for (int i = 0; i < count; i++) {
            if (i > 0) json += ",";
            json += String(waterTemperatureHistory[(start + i) % DATAPOINTS]);
        }
        json += "],\"turbidityHistory\":[";
        for (int i = 0; i < count; i++) {
            if (i > 0) json += ",";
            json += String(turbidityHistory[(start + i) % DATAPOINTS]);
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
    temperature = bme.readTemperature();
    humidity = bme.readHumidity();
    pressure = bme.readPressure() / 100.0F;

    temperatureHistory[historyIndex % DATAPOINTS] = temperature;
    humidityHistory[historyIndex % DATAPOINTS] = humidity;
    pressureHistory[historyIndex % DATAPOINTS] = pressure;
}

void waterTempValues() {
    sensor.requestTemperatures();
    waterTemperature = sensor.getTempCByIndex(0);

    waterTemperatureHistory[historyIndex % DATAPOINTS] = waterTemperature;
}

void turbidityValues() {
    int sensValue = analogRead(turbidity_pin);

    turbidity = ((2380.0 - sensValue) / (2380.0 - 500.0)) * 300.0;
    if( turbidity < 0 ) turbidity = 0;
    if( turbidity > 300) turbidity = 300;

    turbidityHistory[historyIndex % DATAPOINTS] = turbidity;
    // lucht 2047 - turbidity 50
    // kraanwater 2380 - turbidity 0
    // troebel melk water 500 - turbidity 300
}

void riskCalculations() {
    swimRisk = 0;
    fishRisk = 0;
    int tempRisk = 0;
    int turbidityRisk = 0;

    // turbidity
    if (turbidity <= 5) {
        turbidityRisk += 0;
    }
    else if (turbidity <= 25) {
        turbidityRisk += 20;
    }
    else if (turbidity <= 50) {
        turbidityRisk += 50;
    }
    else if (turbidity <= 100) {
        turbidityRisk += 80;
    }
    else {
        turbidityRisk += 100;
    }

    // water temp / outside temp
    if (waterTemperature <= 4) {
        tempRisk += 0;
    }
    else if (waterTemperature <= 10) {
        tempRisk += 20;
    }
    else if (waterTemperature <= 16) {
        tempRisk += 40;
    }
    else if (waterTemperature <= 20) {
        tempRisk += 80;
    }
    else {
        tempRisk += 100;
    }

    tempDiff = temperature - waterTemperature;
    if (tempDiff > 1 ) tempRisk += 10;
    else if (tempDiff < -1) { tempRisk -= 10;}

    swimRisk = tempRisk * 0.4 + turbidityRisk * 0.6; // turbidity belangrijker voor zwemmers
    fishRisk = tempRisk * 0.7 + turbidityRisk * 0.3; // temp belangrijker voor vissen

    if (tempDiff > 1) {
        riskPrediction = 1; // risico stijgt
    }
    else if (tempDiff < -1) {
        riskPrediction = 2; // risico daalt
    }
    else {
        riskPrediction = 3; // risico blijft gelijk
    } 
}