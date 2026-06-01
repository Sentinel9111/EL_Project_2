#include <OneWire.h>
#include <DallasTemperature.h>

#include <Arduino.h>
//#include <headers.h>
//#include <credentials.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
//#include <WiFi.h>
//#include <ESPAsyncWebServer.h>
//#include <LittleFS.h>
//#include <time.h>

#define DATAPOINTS 168 // 168 for 7 days

//AsyncWebServer server(80);

#define ONE_WIRE_BUS 33
#define turbidity_pin 32

//water tempsensor
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensor(&oneWire);

//bme
#define BME_SCL 22
#define BME_SDA 21
Adafruit_BME280 bme;

float temperatureHistory[DATAPOINTS];
float humidityHistory[DATAPOINTS];
float pressureHistory[DATAPOINTS];
unsigned long timestamps[DATAPOINTS];
int historyIndex = 0;

//millis timer
unsigned long startTime = 0;
unsigned long lastTime = 0;
unsigned long currentTime;
unsigned long delayTime = 5000; // 3600000 for 1 hour


//sensors
 int ntu;
 float waterTempC;
 float outsideTempC;
 float humidity;
 float pressure;

// berekeningen
 float tempDiff;
 int swimRisk = 0;
 int fishRisk = 0;

// setups
void setupBme(){
  Wire.begin(BME_SDA, BME_SCL);

    if (!bme.begin(0x76, &Wire)) { // could be 0x77
        Serial.println("Could not find a valid BME280 sensor, check wiring!");
        while (true) {
         delay(1000);
        }
    }
}

/*
void setupLittleFS(){

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
*/

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  // sensors
  sensor.begin(); // water temp sensor
  pinMode(turbidity_pin,INPUT);
  //setupBme(); // crashed de code zonder bme aangesloten

  // vage bas code >:(
  /*
  setupLittleFS();
  setupWifi();
  setupServer();
	setupTime();
  */

  startTime = millis();
  lastTime = millis();
}

void loop() {
  //sensor loops
    currentTime = millis();
    if (currentTime - lastTime >= delayTime) {
      lastTime = currentTime;
      waterTempLoop();
      outsideTempLoop();
      turbidityLoop();
      riskCalculationsLoop();
      serialMonitorLoop();
    }
}

void waterTempLoop() {
  sensor.requestTemperatures();
  waterTempC = sensor.getTempCByIndex(0);
}

void outsideTempLoop() {
  outsideTempC = bme.readTemperature();
  humidity = bme.readHumidity();
  pressure = bme.readPressure() / 100.0F;

  temperatureHistory[historyIndex % DATAPOINTS] = outsideTempC;
  humidityHistory[historyIndex % DATAPOINTS] = humidity;
  pressureHistory[historyIndex % DATAPOINTS] = pressure;

  timestamps[historyIndex % DATAPOINTS] = time(nullptr);
  historyIndex++;
}

void turbidityLoop() {

  int sensValue = analogRead(turbidity_pin);

  ntu = ((2380.0 - sensValue) / (2380.0 - 500.0)) * 300.0;
  if( ntu < 0 ) ntu = 0;
  if( ntu > 300) ntu = 300;

  // lucht 2047 - ntu 50
  // kraan water 2380 - ntu 0
  // troebel melk water 500 - ntu 300

}

void riskCalculationsLoop() {

  swimRisk = 0;
  fishRisk = 0;
  int TempRisk = 0;
  int ntuRisk = 0;

 // turbidity
    if (ntu <= 5) {
        ntuRisk += 0;
    }
    else if (ntu <= 25) {
        ntuRisk += 20;
    }
    else if (ntu <= 50) {
        ntuRisk += 50;
    }
    else if (ntu <= 100) {
        ntuRisk += 80;
    }
    else {
        ntuRisk += 100;
    }

 // water temp / outside temp
    if (waterTempC <= 4) {
        TempRisk += 0;
    }
    else if (waterTempC <= 10) {
        TempRisk += 20;
    }
    else if (waterTempC <= 16) {
        TempRisk += 40;
    }
    else if (waterTempC <= 20) {
        TempRisk += 80;
    }
    else {
        TempRisk += 100;
    }

   tempDiff = outsideTempC - waterTempC;
   if (tempDiff > 1 ) TempRisk += 10;
   else if (tempDiff < -1) { TempRisk -= 10;}

 swimRisk = TempRisk * 0.4 + ntuRisk * 0.6; // ntu belangerijker voor zwemmers
 fishRisk = TempRisk * 0.7 + ntuRisk * 0.3; // temp belangerijker voor vissen

}

void serialMonitorLoop() {

  byte swimRiskSerial = 0;
  byte fishRiskSerial = 0;
  byte waterPrediction = 0;
  //ntu
  //outsideTempC
  //waterTempC
  //pressure
  //humidity

  // risk table
  if(swimRisk <= 20)        {swimRiskSerial = 1;}
  else if(swimRisk <= 50)   {swimRiskSerial = 2;}
  else if(swimRisk > 50)    {swimRiskSerial = 3;}

  if(fishRisk <= 20)        {fishRiskSerial = 1;}
  else if(fishRisk <= 40)   {fishRiskSerial = 2;}
  else if(fishRisk <= 60)   {fishRiskSerial = 3;}
  else if(fishRisk >= 80)   {fishRiskSerial = 4;}
  else if(fishRisk >  80)   {fishRiskSerial = 5;}

  if( tempDiff > 1)         {waterPrediction = 1;}
  else if(tempDiff < -1)    {waterPrediction = 2;}
  else                      {waterPrediction = 3;}



  // serial print voor zwem advies
  Serial.print("water kwaliteit is ");

  switch(swimRiskSerial){
    case 1:
    Serial.println("veilig voor zwemmen");
    break;
    case 2:
    Serial.println(" veilig voor zwemmen maar hou de waterkwaliteit in de gaten");
    break;
    case 3:
    Serial.println(" slecht. zwemmen word afgeraden");
    break;
  }

  // serialprint voor vis advies
  Serial.print("water kwaliteit is ");

  switch(fishRiskSerial){
    case 1:
    Serial.println("goed voor vissen");
    break;
    case 2:
    Serial.println("kan lichte stress veroorzaken voor vissen");
    break;
    case 3:
    Serial.println("matig voor vissen");
    break;
    case 4:
    Serial.println("ongezond voor vissen");
    break;
    case 5:
    Serial.println("Gevaarlijk voor vissen");
    break;
  }

  // water kwaliteit voorspelling
  Serial.print("waterkwaliteit voorspelling = ");

  switch(waterPrediction){
    case 1:
    Serial.println("risico neemt toe");
    break;
    case 2:
    Serial.println("risico neemt af");
    break;
    case 3:
    Serial.println("risico blijft gelijk");
    break;
  }

  Serial.println(" ");

  // overige calculaties
  Serial.print("waterTemp = " );
  Serial.print(waterTempC);
  Serial.println("°C");

  Serial.print("buitenTemp = " );
  Serial.print(outsideTempC);
  Serial.println("°C");

  Serial.print("ntu = ");
  Serial.println(ntu);

  Serial.print("pressure = " );
  Serial.println(pressure);


  Serial.print("humidity = " );
  Serial.println(humidity);

  Serial.println(" ");
}
