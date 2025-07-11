#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// ====== Wi-Fi creds ======
const char* ssid = "Wife Name";
const char* password = "Wifi Password";
// ====== Backend ======
const char* serverBase = "backend API";


#define TRIG 5
#define ECHO 18
#define EAU 13  // GPIO qui contrôle la pompe via un relais
#define MIXEUR 27
#define DRAIN 26
#define PH 34
#define ACIDE 12
#define BASE 14
#define RELAY 25

float distance;
int pompe_fill = 0;
int pompe_drain = 0;
int mixeur = 0;
int pompe_base = 0;
int pompe_acide = 0;

void setup() {
  Serial.begin(115200);

  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);
  pinMode(MIXEUR, OUTPUT);
  pinMode(EAU, OUTPUT);
  pinMode(DRAIN, OUTPUT);
  pinMode(RELAY, OUTPUT);
  pinMode(ACIDE, OUTPUT);
  pinMode(BASE, OUTPUT);
  pinMode(PH, OUTPUT);

  digitalWrite(EAU, LOW); // Pompe éteinte au démarrage
  digitalWrite(MIXEUR, HIGH);
  digitalWrite(ACIDE, HIGH);
  digitalWrite(BASE, HIGH);
  digitalWrite(DRAIN, HIGH);
  digitalWrite(RELAY, HIGH);
  
  connectToWiFi();
}
int mesureDistance(){
  // Envoi de l'impulsion ultrasonique
  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);

  // Mesure de la durée de l'écho
  int duree = pulseIn(ECHO, HIGH);
  return duree * 0.034 / 2;
}
void loop() {
  distance = mesureDistance();
  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");
  // Condition de remplissage : distance > 20 cm → cuve vide
  if (distance > 12) {
    digitalWrite(EAU, LOW);
    pompe_fill = 1;
    while (distance > 5) {
      distance = mesureDistance();
      sendDataToBackend(distance,pompe_fill,pompe_drain,mixeur);
      Serial.println("Pompe ACTIVÉE");
    }
    
  } else if (distance < 5) {
    digitalWrite(EAU, HIGH);
    pompe_fill = 0;
    Serial.println("Pompe ARRÊTÉE");
    digitalWrite(MIXEUR, LOW);
    digitalWrite(RELAY, LOW);
    mixeur = 1;
    Serial.println("Mixeur a démarer");
    sendDataToBackend(distance,pompe_fill,pompe_drain,mixeur);
    delay(60000);
    digitalWrite(MIXEUR, HIGH);
    digitalWrite(RELAY, HIGH);
    mixeur = 0;
    sendDataToBackend(distance,pompe_fill,pompe_drain,mixeur);
    Serial.println("MIXEUR est arreté");
    while (true) {
      distance = mesureDistance();
      Serial.print("Distance lors de drenage: ");
      Serial.println(distance);
      digitalWrite(DRAIN, LOW);
      pompe_drain = 1;
      sendDataToBackend(distance,pompe_fill,pompe_drain,mixeur);
      Serial.println("Drenage est demarer");
      if (distance > 12){
          pompe_drain = 0;
          digitalWrite(DRAIN, HIGH);
          Serial.println("Drenage est arreté");
          break;
      }
      delay(2000);
    }
  }
  Serial.println("\nWiFi connected.");
  Serial.print("Arduino IP Address: ");
  Serial.println(WiFi.localIP());

  delay(5000); 
}
void connectToWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
}


void sendDataToBackend(float distance, int pompe_fill, int pompe_drain, int mixeur) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = String(serverBase) + "/api/data";
    http.begin(url);
    http.addHeader("Content-Type", "application/json");

    // Prepare JSON using ArduinoJson
    StaticJsonDocument<128> doc;
    doc["distance"] = distance;
    doc["pompe_fill"] = pompe_fill;
    doc["pompe_drain"] = pompe_drain;
    doc["mixeur"] = mixeur;

    String payload;
    serializeJson(doc, payload);

    int httpCode = http.POST(payload);

    Serial.printf("POST %s -> HTTP code: %d\n", url.c_str(), httpCode);
    http.end();
  } else {
    Serial.println("WiFi not connected. Skipping POST.");
  }
}

