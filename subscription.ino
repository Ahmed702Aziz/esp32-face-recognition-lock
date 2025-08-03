#include <ESP8266WiFi.h>
#include <WiFiManager.h>          // https://github.com/tzapu/WiFiManager
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

// MQTT config
const char* mqtt_server = "94fbaebfe12e4662be30e6222d1af83d.s1.eu.hivemq.cloud";
const int mqtt_port = 8883;
const char* mqtt_user = "ahmedaziz";
const char* mqtt_pass = "Aziz1234567";
const char* mqtt_topic = "door/control";  // Unified topic

// Relay pin config
#define RELAY_PIN 2  // ESP-01 GPIO2 (D4)
#define MQTT_LED 0
#define ALERT_LED 3
// Secure client and MQTT client
WiFiClientSecure espClient;
PubSubClient client(espClient);

// Door timing
unsigned long doorOpenTime = 0;
bool doorOpened = false;

void setup_wifi() {
  WiFiManager wm;

  // Set static IP for AP mode
  IPAddress apIP(192, 168, 11, 1);
  IPAddress netMsk(255, 255, 255, 0);
  wm.setAPStaticIPConfig(apIP, apIP, netMsk);

  // Try saved network or show config portal
  bool res = wm.autoConnect("ESP01-Relay-Setup");
  if (!res) {
    Serial.println("❌ Failed to connect or timeout");
    ESP.restart();
  }

  Serial.print("✅ Connected to WiFi: ");
  Serial.println(WiFi.SSID());
}

void mqtt_callback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  Serial.print("📩 Message arrived [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.println(message);

  if (message == "door/open") {
    digitalWrite(RELAY_PIN, HIGH);
    Serial.println("🚪 Door Opened");
    digitalWrite(MQTT_LED,1);
    doorOpenTime = millis();
    doorOpened = true;
  } else if (message == "door/close") {
    digitalWrite(RELAY_PIN, LOW);
    digitalWrite(MQTT_LED,0);
    digitalWrite(ALERT_LED,1);
    delay(500);
    digitalWrite(ALERT_LED,0);
    Serial.println("🔒 Door Closed");
    doorOpened = false;
  }
}

void mqtt_reconnect() {
  while (!client.connected()) {
    Serial.print("🔄 Connecting to MQTT...");
    String clientId = "ESP01Client-" + String(random(0xffff), HEX);
    if (client.connect(clientId.c_str(), mqtt_user, mqtt_pass)) {
      Serial.println("✅ Connected");
      if (client.subscribe(mqtt_topic)) {
        Serial.println("📡 Subscribed to topic: door/control");
      } else {
        Serial.println("❌ Subscription failed");
      }
    } else {
      Serial.print("❌ Failed, rc=");
      Serial.print(client.state());
      Serial.println(". Retrying in 5s...");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(MQTT_LED,OUTPUT);
  pinMode(ALERT_LED,OUTPUT);
  digitalWrite(RELAY_PIN, LOW); // Door closed initially
  digitalWrite(MQTT_LED, LOW);
  digitalWrite(ALERT_LED, LOW);
  setup_wifi();

  espClient.setInsecure();  // Skip certificate validation (for test/dev)
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(mqtt_callback);
}

void loop() {
  if (!client.connected()) {
    mqtt_reconnect();
  }
  client.loop();

  // Auto-close after 10 seconds
  if (doorOpened && (millis() - doorOpenTime > 10000)) {
    digitalWrite(RELAY_PIN, LOW);
    digitalWrite(MQTT_LED,0);
    Serial.println("⏱️ Door auto-closed after 10 seconds");
    doorOpened = false;
  }
}
