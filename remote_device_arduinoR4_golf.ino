#include <Wire.h>
#include <DHT.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <WiFi.h> 
#include <PubSubClient.h>

WiFiClient espClient;
PubSubClient client(espClient);

Adafruit_BME280 bme; 
DHT dht(5, DHT11);

const char* ssid = "MOVISTAR_FFA8";    
const char* password = "tBEHGtE4v3LHuvRTt4zt"; 

const char* broker = "iot.iartificial.io";
const int port = 1883;

const char* topic_temp = "sensor/golf/temp";
const char* topic_pressure = "sensor/golf/pressure";
const char* topic_humidity = "sensor/golf/humidity";
const char* topic_light = "sensor/golf/light";
const char* topic_water_flow = "sensor/golf/water_flow";

const int analogPin = A2;
const int digitalPin = D2;

int waterFlow = 0;

void setup() {
  dht.begin();

  pinMode(digitalPin, INPUT);

  Serial.begin(9600);
  bool status = bme.begin(0x76); // Use 0x77 if needed
  if (!status) {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    while (1); // Halt program if sensor not found
  }  
  Serial.println();
  
  connectToWiFi();
  client.setServer(broker, port);
}

void loop() {
  if (!client.connected()) {
    reconnectToMQTT();
  }
  client.loop();

  int analogValue = analogRead(analogPin);
  int digitalValue = digitalRead(digitalPin);
  Serial.print("LDR Analog Value (A2): ");
  Serial.println(analogValue);
  Serial.print("LDR Digital Value (D2): ");
  Serial.println(digitalValue);

  if (digitalValue == HIGH) {
    publishToMQTT(topic_light, "0");
  } else {
    publishToMQTT(topic_light, "1");
  }

  float humidity = dht.readHumidity();
  float dht_temp = dht.readTemperature();
  float bme_temp = bme.readTemperature();
  float pressure = bme.readPressure() / 100; 

  if (digitalValue == LOW) {
    waterFlow = 1;
  } else if (digitalValue == HIGH && dht_temp < 22) {
    waterFlow = 2;
  } else {
    waterFlow = 0;
  }

  Serial.print("Water Flow Value: ");
  Serial.println(waterFlow);
  publishToMQTT(topic_water_flow, String(waterFlow).c_str());

  if (!isnan(humidity) && !isnan(dht_temp) && !isnan(bme_temp) && !isnan(pressure)) {
    publishToMQTT(topic_humidity, String(humidity).c_str());
    publishToMQTT(topic_temp, String(bme_temp).c_str());
    publishToMQTT(topic_pressure, String(pressure).c_str());
         
    Serial.print(F("BME280 Temperature  = "));
    Serial.print(bme_temp);
    Serial.println(" *C");
    Serial.print(F("BME280 Pressure = "));
    Serial.print(pressure); 
    Serial.println("  hPa");
    Serial.print(F("DHT11 Temperature = "));
    Serial.print(dht_temp);
    Serial.println(" *C");
    Serial.print(F("DHT11 Humidity = "));
    Serial.print(humidity);
    Serial.println(" %"); 
  } else {
    Serial.println("Failed to read from sensors!");
  }

  delay(5000);
}


void connectToWiFi() {
  Serial.print("Connecting to Wi-Fi");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWi-Fi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnectToMQTT() {
  while (!client.connected()) {
    Serial.print("Connecting to MQTT...");
    if (client.connect("ArduinoClient")) { 
      Serial.println("connected!");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" trying again in 5 seconds");
      delay(5000);
    }
  }
}

void publishToMQTT(const char* topic, const char* payload) {
  if (client.publish(topic, payload)) {
    Serial.print("Published to ");
    Serial.print(topic);
    Serial.print(": ");
    Serial.println(payload);
  } else {
    Serial.println("Publish failed!");
  }
}