#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <LiquidCrystal_I2C.h>
    
LiquidCrystal_I2C lcd(0x27,16,2);

int metal = D0;
int buzzer = D3;
int relay = D5;
int relay1 = D6;
int metalv;

#include "DHT.h"
#define DHTPIN D7
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

WiFiClient espClient;
PubSubClient client(espClient);

const char* ssid = "ACTFIBERNET";
const char* password = "act12345";
const char* mqtt_server = "test.mosquitto.org";

void connectwifi() {
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected");
  Serial.println("IP Address: " + WiFi.localIP().toString());
}

void callback(char* topic, byte* payload, unsigned int length) {
  // Handle MQTT messages if needed
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP8266Client-" + String(random(0xFFFF), HEX);

    if (client.connect(clientId.c_str())) {
      Serial.println("Connected to MQTT");
      client.subscribe("soldier");
    } else {
      Serial.print("Failed, rc=");
      Serial.print(client.state());
      Serial.println(" Trying again in 5 seconds...");
      delay(5000);
    }
  }
}

void mqtt() {
  client.setCallback(callback);
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}

void setup() {
  pinMode(metal, INPUT);
  pinMode(buzzer, OUTPUT);
  pinMode(relay, OUTPUT);
  pinMode(relay1, OUTPUT);

  Serial.begin(9600);
  dht.begin();
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.print("E UNIFORM");
  lcd.setCursor(0,1);
  lcd.print("SOLDIERS");
  delay(3000);
  lcd.clear();

  connectwifi();
  client.setServer(mqtt_server, 1883);
}

void loop() {
  mqtt();

  metalv = digitalRead(metal);
  Serial.print("METAL: ");
  Serial.println(metalv);

  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (isnan(t) || isnan(h)) {
    Serial.println("Failed to read from DHT sensor!");
    return;  // Skip the loop iteration if readings are NaN
  }

  Serial.print("TEMPERATURE: ");
  Serial.println(t);
  Serial.print("HUMIDITY: ");
  Serial.println(h);

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("TEMP=");
  lcd.setCursor(5,0);
  lcd.print(t);
  lcd.setCursor(7,0);
  lcd.print("METAL=");
  lcd.setCursor(13,0);
  lcd.print(metalv);

  // Publish Temperature Alerts
  if (t > 33) {
    digitalWrite(relay1, HIGH); // Turn on fan
    digitalWrite(relay, LOW);   // Turn off Peltier module
    client.publish("SOLDIER", "HIGH TEMPERATURE");
    lcd.setCursor(0, 1);
    lcd.print("HIGH TEMP");
  } else {
    digitalWrite(relay1, LOW);  // Turn off fan
    digitalWrite(relay, HIGH);  // Turn on Peltier module
    client.publish("SOLDIER", "LOW TEMPERATURE");
    lcd.setCursor(0, 1);
    lcd.print("LOW TEMP");
  }

  // Publish Metal Detection Alerts
  if (metalv == 1) {
    digitalWrite(buzzer, HIGH);
    client.publish("SOLDIER", "METAL DETECTED");
    lcd.setCursor(8, 1);
    lcd.print("METAL");
    Serial.println("metal detected");
  } else {
    digitalWrite(buzzer, LOW);
    lcd.setCursor(8, 1);
    lcd.print("NO METAL ");
    Serial.println("no metal detected");
  }

  delay(2000);  // Ensure DHT sensor gets stable readings
}
