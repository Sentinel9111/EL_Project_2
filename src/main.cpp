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
#define ONE_WIRE_BUS 33
#define TURBIDITY_PIN 32

#define DATAPOINTS 168 // 168 for 7 days

// initialise sensors and webserver
Adafruit_BME280 bme;
AsyncWebServer server(80);
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensor(&oneWire);
bool bmeFound = false;

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
unsigned long delayTime = 5000; // 3600000 for 1 hour

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
int algaeGrowRisk = 0;
byte riskPrediction = 0;

// bootup sequence
void setup() {
    Serial.begin(115200);
    delay(1000); // wait a second for serial monitor
    Serial.println(F("starting waterboei"));

    setupLittleFS();
    setupWifi();
    setupTime();
    setupServer();

    // initialise sensors
    sensor.begin();
    pinMode(TURBIDITY_PIN, INPUT);
    setupBME();

    // reconnect Wi-Fi on disconnect
    WiFi.onEvent([](WiFiEvent_t event) {
        if (event == ARDUINO_EVENT_WIFI_STA_DISCONNECTED) {
            Serial.println("WiFi disconnected, reconnecting...");
            WiFi.reconnect();
        }
    });

    Serial.println("--------------------------");

    // initialise non-blocking delays
    startTime = millis();
    lastTime = millis();
}

void loop() {
    // non-blocking delay
    currentTime = millis();
    if (currentTime - lastTime >= delayTime) {
        lastTime = currentTime;

        // call all sensor functions
        BMEValues();
        delay(50);
        waterTempValues();
        delay(50);
        turbidityValues();
        delay(50);
        riskCalculations();

        Serial.println("--------------------------");

        // save time history
        timestamps[historyIndex % DATAPOINTS] = time(nullptr);
        historyIndex++;
    }
}

void setupBME() {
    Wire.begin(BME_SDA, BME_SCL);
    Wire.setTimeout(100);

    // look for the BME280
    if (bme.begin(0x76, &Wire)) {
        Serial.println("Found BME280 sensor on 0x76");
        bmeFound = true;
    } else if (bme.begin(0x77, &Wire)) {
        Serial.println("Found BME280 sensor on 0x77");
        bmeFound = true;
    } else {
        Serial.println("BME280 sensor not found");
        bmeFound = false;
    }
}

// setup file system
void setupLittleFS() {
    // initialise LittleFS
    if (!LittleFS.begin()) {
        Serial.println("Failed to initialize LittleFS");
        while (true) {
            delay(500);
        }
    }
    Serial.println("LittleFS initialized");
}

void setupWifi() {
    // connect to Wi-Fi network
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Connecting");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nIP: http://" + WiFi.localIP().toString() + "/");
}

void setupServer() {
    // api for sending data as json to webserver
    // starts up webserver converts sensor data to json and sends it to the webserver
    server.on("/api/data", HTTP_GET, [](AsyncWebServerRequest *request) {
        String json = "{";
        json += "\"temperatuur\":" + String(temperature, 1) + ",";
        json += "\"luchtvochtigheid\":" + String(humidity, 0) + ",";
        json += "\"luchtdruk\":" + String(pressure, 0) + ",";
        json += "\"watertemperatuur\":" + String(waterTemperature, 1) + ",";
        json += "\"troebelheid\":" + String(turbidity) + ",";
        json += "\"zwemrisico\":" + String(swimRisk) + ",";
        json += "\"visrisico\":" + String(fishRisk) + ",";
        json += "\"algengroei\":" + String(algaeGrowRisk) + ",";
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

    // shut stupid errors up
    server.serveStatic("/", LittleFS, "/")
        .setDefaultFile("/index.html")
        .setCacheControl("max-age=86400") // Helpt tegen constante her-aanvragen
        .setFilter([](AsyncWebServerRequest *request) {
            return !request->url().startsWith("/api");
        });

    server.begin();
    Serial.println("HTTP server started");
}

void setupTime() {
    // get time for time server
	configTime(3600, 3600, "pool.ntp.org");
    Serial.print("Syncing time");
    while (time(nullptr) < 1000000000) {
        delay(500);
        Serial.print(".");
    }
    Serial.println(" OK");
}

void BMEValues() {
    // read BME280 values
    if (bmeFound) {
        temperature = bme.readTemperature();
        humidity = bme.readHumidity();
        pressure = bme.readPressure() / 100.0F;
    } else {
        temperature = 0.0;
        humidity = 0.0;
        pressure = 0.0;
    }

    // save BME280 values in history array
    temperatureHistory[historyIndex % DATAPOINTS] = temperature;
    humidityHistory[historyIndex % DATAPOINTS] = humidity;
    pressureHistory[historyIndex % DATAPOINTS] = pressure;

    Serial.print("Air temperature:   ");
    Serial.println(temperature);
    Serial.print("Humidity:          ");
    Serial.println(humidity);
    Serial.print("Pressure:          ");
    Serial.println(pressure);
}

void waterTempValues() {
    // read water temperature sensor values
    sensor.requestTemperatures();
    waterTemperature = sensor.getTempCByIndex(0);

    // save values in history array
    waterTemperatureHistory[historyIndex % DATAPOINTS] = waterTemperature;

    Serial.print("Water temperature: ");
    Serial.println(waterTemperature);
}

void turbidityValues() {
    // read turbidity sensor values
    int sensValue = analogRead(TURBIDITY_PIN);
    // calculate actual NTU value
    turbidity = ((2380.0 - sensValue) / (2380.0 - 500.0)) * 300.0;
    if( turbidity < 0 ) turbidity = 0;
    if( turbidity > 300) turbidity = 300;

    // save values in history array
    turbidityHistory[historyIndex % DATAPOINTS] = turbidity;

    Serial.print("Turbidity:         ");
    Serial.println(turbidity);
}

void riskCalculations() {
    swimRisk = 0;
    fishRisk = 0;
    algaeGrowRisk = 0;
    int tempRisk = 0;
    int turbidityRisk = 0;

    // calculate turbidity risk
    if (turbidity <= 1) {
        turbidityRisk += 0;
    } else if (turbidity <= 5) {
        turbidityRisk += 10;
    } else if (turbidity <= 10) {
        turbidityRisk += 30;
    } else if (turbidity <= 20) {
        turbidityRisk += 60;
    } else  if (turbidity <= 45) {
        turbidityRisk += 80;
    } else {
        turbidityRisk += 100;
    }

    // calculate water temperature risk
    if (waterTemperature <= 4) {
        tempRisk += 0;
    } else if (waterTemperature <= 10) {
        tempRisk += 15;
    } else if (waterTemperature <= 16) {
        tempRisk += 35;
    } else if (waterTemperature <= 20) {
        tempRisk += 55;
    } else if (waterTemperature <= 25) {
        tempRisk += 70;
    } else {
        tempRisk += 100;
    }

    // calculate temperature difference risk
    tempDiff = temperature - waterTemperature;
    if (tempDiff > 2 ) tempRisk += 5;
    else if (tempDiff < -5) { tempRisk -= 5;}
    tempRisk = constrain(tempRisk, 0, 100);

    // calculate swimRisk, fishRisk and algaeGrowRisk
    swimRisk = tempRisk * 0.1 + turbidityRisk * 0.9; // turbidity belangrijker voor zwemmers
    fishRisk = tempRisk * 0.7 + turbidityRisk * 0.3; // temp belangrijker voor vissen
    algaeGrowRisk = tempRisk * 0.6 + turbidityRisk * 0.4;

    // calculate risk prediction
    if (tempDiff > 1) {
        riskPrediction = 1; // risico stijgt
    } else if (tempDiff < -1) {
        riskPrediction = 2; // risico daalt
    } else {
        riskPrediction = 3; // risico blijft gelijk
    }
}