#include <Arduino.h>
#include <headers.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#define BME_SCL 22
#define BME_SDA 21

Adafruit_BME280 bme;

unsigned long lastTime;
unsigned long currentTime;
unsigned long delayTime = 1000;

void setup() {
    Serial.begin(115200);
    Serial.println(F("Waterboei"));
    Wire.begin(BME_SDA, BME_SCL);

    if (!bme.begin(0x77, &Wire)) { // could be 0x76
        Serial.println("Could not find a valid BME280 sensor, check wiring!");
        while (true) {
            delay(1000);
        }
    }
    lastTime = millis();
}

void loop() {
    currentTime = millis();
    if (currentTime - lastTime >= delayTime) {
        lastTime = currentTime;
        BMEValues();
    }
}

void BMEValues() {
    Serial.print("Temperatuur: ");
    Serial.print(bme.readTemperature());
    Serial.println("°C");
    Serial.print("Luchtvochtigheid: ");
    Serial.print(bme.readHumidity());
    Serial.println("%");
    Serial.print("Luchtdruk: ");
    Serial.print(bme.readPressure() / 100.0F);
    Serial.println("hPa");
}