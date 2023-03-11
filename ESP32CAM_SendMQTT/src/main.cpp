/*

TITRE          : Test de la communication MQTT sur NodeRED
AUTEUR         : Franck Nkeubou Awougang
DATE           : 10/03/2023
DESCRIPTION    : Communication MQTT avec NodeRED à l'aide de Mosquitto installé sur le raspberry Pi
VERSION        : 0.0.1

*/



#include <Arduino.h>
#include "Timer.h"

#include <WiFi.h>
#include <PubSubClient.h>

const char* ssid = "FibreOP532";
const char* password = "9PXPE66PM6XM55M8";
const char* mqtt_server = "192.168.2.75";
const char* mqtt_username = "ubuntu";
const char* mqtt_password = "ubuntu";

Timer temps;
WiFiClient espClient;
PubSubClient client(espClient);
void callback(char* topic, byte* payload, unsigned int length);
void reconnect();

void setup() {
  Serial.begin(9600);
  delay(100);
  Serial.println();
  Serial.print("Connexion au réseau WiFi ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connecté");
  Serial.println("Adresse IP : ");
  Serial.println(WiFi.localIP());

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  Serial.println("Connexion au broker MQTT...");
  while (!client.connected()) {
    if (client.connect("ESP32Client", mqtt_username, mqtt_password )) {
      Serial.println("Connecté au broker MQTT");
      
    } else {
      Serial.print("Échec de connexion au broker MQTT, code erreur = ");
      Serial.print(client.state());
      delay(2000);
    }
  }
  temps.startTimer(10000);
}

void loop() {

  if (!client.connected()) {
    reconnect();
  }
  if (temps.isTimerReady())
  {
  Serial.println("loop");
    client.publish("test", "Hello from ESP32, loop");
    Serial.println("Hello from ESP32, loop");
    client.subscribe("test/state");
    temps.startTimer(10000);
  }
  client.loop();
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message reçu [");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.print("] ");
  
  
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Tentative de reconnexion au broker MQTT...");
    if (client.connect("ESP32Client", mqtt_username, mqtt_password )) {
      Serial.println("Connecté");
      client.subscribe("test");
      client.publish("test", "Hello from ESP32, reconnect");
    } else {
      Serial.print("Échec de connexion au broker MQTT, code erreur = ");
      Serial.print(client.state());
      
    }
  }
}
