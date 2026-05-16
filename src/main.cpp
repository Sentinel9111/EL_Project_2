#include <Arduino.h>
#include <headers.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#define BME_SCL 22
#define BME_SDA 21

Adafruit_BME280 bme;

unsigned long startTime;
unsigned long currentTime;
unsigned long delayTime = 1000;

void setup() {
Serial.begin(9600);
    Serial.println(F("Waterboei"));

    if (!bme.begin(0x77, &Wire)) { // could be 0x76
        Serial.println("Could not find a valid BME280 sensor, check wiring!");
        while (1);
    }
    startTime = millis();
}

void loop() {
    currentTime = millis();
    if (currentTime - startTime >= delayTime) {
        BMEValues();
        startTime = currentTime;
    }
}

void BMEValues() {
    Serial.print("Temperature: ");
    Serial.print(bme.readTemperature());
    Serial.println("°C");
    Serial.print("Humidity: ");
    Serial.print(bme.readHumidity());
    Serial.println("%");
    Serial.print("Pressure: ");
    Serial.print(bme.readPressure() / 100.0F);
    Serial.println("hPa");
}