#include <Wire.h>
#include <DHT.h>
#include <Adafruit_Sensor.h>
//#include <Adafruit_BME280.h>
#include <WiFi.h> 
#include <PubSubClient.h>
#include <ESP32Servo.h>

const char* ssid = "RedClaro2Home";    
const char* password = "abcd$w"; 

const char* broker = "iot.iartificial.io";
const int port = 1883;
const char* topic_temp = "sensor/wr/temp";
const char* topic_pressure = "sensor/wr/pressure";
const char* topic_humidity = "sensor/wr/humidity";
const char* topic_light = "sensor/wr/light";
const char* topic_water_flow = "sensor/wr/water_flow";

WiFiClient espClient;
PubSubClient client(espClient);

#define DO_SERVO 18
#define DO_PIN 13
#define DHTPIN 4

DHT dht(DHTPIN, DHT11);
Servo servo;

void setup() {
  dht.begin();
  pinMode(DO_PIN, INPUT);
  servo.attach(DO_SERVO);

  Serial.begin(9600);

  connectToWiFi();
  client.setServer(broker, port);
}

void loop() {
  if (!client.connected()) {
    reconnectToMQTT();
  }
  client.loop();

  int light = digitalRead(DO_PIN);
  float humidity = dht.readHumidity();
  float dht_temp = dht.readTemperature();

  if (!isnan(humidity) && !isnan(dht_temp)) {
    publishToMQTT(topic_light, (light == HIGH) ? "0" : "1");
    publishToMQTT(topic_humidity, String(humidity).c_str());
    publishToMQTT(topic_temp, String(dht_temp).c_str());

    Serial.print(F(">>Sensors LDR Light = "));
    if (light == HIGH) Serial.print("Off"); else Serial.print("On");
    Serial.print(F(", DHT11 Temperature = "));
    Serial.print(dht_temp);
    Serial.print(" *C, ");
    Serial.print(F("DHT11 Humidity = "));
    Serial.print(humidity);
    Serial.println(" %");

    // Controlar el servomotor basado en la luz, temperatura y humedad
    int flowStatus = controlWaterFlow(light, dht_temp, humidity);  // Obtiene el estado del flujo de agua
    publishToMQTT(topic_water_flow, String(flowStatus).c_str());
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

int controlWaterFlow(int light, float temperature, float humidity) {
  int angle = 0;  // Variable para almacenar el ángulo del servomotor
  int flowStatus = 0;  // Estado del flujo de agua (0 = cerrado, 1 = parcialmente abierto, 2 = completamente abierto)
  String strControlWaterFlow;
  // Lógica para determinar el ángulo del servomotor y el estado del flujo de agua
  if (humidity < 40) {
    angle = map(temperature, 20, 35, 0, 180); // Rango de temperatura para controlar la apertura
    flowStatus = 2;  // Totalmente abierto
    strControlWaterFlow = "Fully Open";
  } else if (humidity > 70) {
    angle = 0;  // Cerrar las compuertas si la humedad es alta
    flowStatus = 0;  // Cerrado
    strControlWaterFlow = "Closed";
  } else if (light == HIGH && temperature > 25) {
    angle = 90;  // Si hace calor y está oscuro, abrir parcialmente
    flowStatus = 1;  // Parcialmente abierto
    strControlWaterFlow = "Partially Open";
  } else {
    angle = 45;  // En caso contrario, abrir parcialmente
    flowStatus = 1;  // Parcialmente abierto
    strControlWaterFlow = "Partially Open";
  }

  servo.write(angle);  // Mover el servomotor al ángulo calculado
  Serial.print("<<Actuator Control Water Flow = " + strControlWaterFlow + ", Servo Rotation Angle: ");
  Serial.println(angle);
  return flowStatus;  // Retorna el estado del flujo de agua
}
