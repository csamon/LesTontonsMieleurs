#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <esp_sleep.h>
#include <Wire.h>
#include <time.h>
#include <Meshtastic.h>

// UUIDs pour BLE - Identifiants uniques pour le service et la caractéristique BLE
#define SERVICE_UUID "91bad492-b950-4226-aa2b-4ede9fa42f59"
#define CHARACTERISTIC_UUID "cba1d466-344c-4be3-ab3f-189f80dd7518"

// Intervalle de transmission (10 minutes)
#define TRANSMISSION_INTERVAL 600000 // 10 minutes en millisecondes
#define LISTEN_INTERVAL 60000 // 1 minute en millisecondes
#define SYNC_INTERVAL 3600000 // 1 heure en millisecondes

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
uint32_t lastTransmissionTime = 0;
uint32_t lastSyncTime = 0;
float temperature = 0;
float humidity = 0;
float pressure = 0;
int lightLevel = 0;
int soilMoisture = 0;

// Variables pour stocker les données des ruches
struct RucheData {
  float temperature;
  float humidity;
  int vibration;
  int soundLevel;
  int weight;
  bool dataReceived;
};

RucheData ruches[5]; // Stockage pour 5 ruches

// Fonction pour lire les capteurs environnementaux
void readEnvironmentalSensors() {
  // Exemple de lecture de capteur : BME280 (Température, Humidité, Pression)
  Wire.beginTransmission(BME280_ADDR);
  // Ajouter ici le code spécifique pour lire les données du BME280
  Wire.endTransmission();
  
  // Exemple de lecture de capteur : Niveau de lumière (BH1750)
  lightLevel = analogRead(LIGHT_SENSOR_PIN);
  Serial.print("Light Level: "); Serial.println(lightLevel);

  // Exemple de lecture de capteur : Humidité du sol
  soilMoisture = analogRead(SOIL_MOISTURE_PIN);
  Serial.print("Soil Moisture: "); Serial.println(soilMoisture);
}

// Fonction pour envoyer les données via Meshtastic
void sendDataToGateway() {
  String message = "Env Temp: " + String(temperature) +
                   ", Env Hum: " + String(humidity) +
                   ", Env Press: " + String(pressure) +
                   ", Light: " + String(lightLevel) +
                   ", Soil: " + String(soilMoisture);

  for (int i = 0; i < 5; i++) {
    message += ", Ruche " + String(i + 1);
    if (ruches[i].dataReceived) {
      message += " Temp: " + String(ruches[i].temperature) +
                 ", Hum: " + String(ruches[i].humidity) +
                 ", Vib: " + String(ruches[i].vibration) +
                 ", Sound: " + String(ruches[i].soundLevel) +
                 ", Weight: " + String(ruches[i].weight);
    } else {
      message += " No Data";
    }
  }

  // Envoyer le message au réseau Meshtastic
  sendtext(message);
}

// Callback pour les événements BLE
class MyServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    Serial.println("Device connected");
  }

  void onDisconnect(BLEServer* pServer) {
    Serial.println("Device disconnected");
  }
};

// Callback pour recevoir les données des ruches via BLE
class MyCharacteristicCallbacks: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pCharacteristic) {
    std::string value = pCharacteristic->getValue();
    Serial.print("Received data: "); Serial.println(value.c_str());

    // Analyse et stockage des données reçues
    // Format des données attendu: "RucheX,Temp,Y,Hum,Z,Vib,A,Sound,B,Weight,C"
    int rucheIndex;
    float temp, hum;
    int vib, sound, weight;
    sscanf(value.c_str(), "Ruche%d,Temp,%f,Hum,%f,Vib,%d,Sound,%d,Weight,%d",
           &rucheIndex, &temp, &hum, &vib, &sound, &weight);

    // Stockage des données
    if (rucheIndex > 0 && rucheIndex <= 5) {
      ruches[rucheIndex - 1].temperature = temp;
      ruches[rucheIndex - 1].humidity = hum;
      ruches[rucheIndex - 1].vibration = vib;
      ruches[rucheIndex - 1].soundLevel = sound;
      ruches[rucheIndex - 1].weight = weight;
      ruches[rucheIndex - 1].dataReceived = true;
    }
  }
};

void setup() {
  Serial.begin(115200);
  Wire.begin();

  // Initialisation BLE
  BLEDevice::init("Hub");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ |
                      BLECharacteristic::PROPERTY_WRITE
                    );
  pCharacteristic->setCallbacks(new MyCharacteristicCallbacks());
  pService->start();
  pServer->getAdvertising()->start();

  // Initialisation Meshtastic
  startMeshtastic();

  // Configuration pour le deep sleep
  esp_sleep_enable_timer_wakeup(TRANSMISSION_INTERVAL * 1000); // Réveil pour transmission des données
}

void loop() {
  // Obtenir le temps actuel
  uint32_t currentTime = millis();

  // Lire les capteurs environnementaux et envoyer les données toutes les 10 minutes
  if (currentTime - lastTransmissionTime >= TRANSMISSION_INTERVAL) {
    // Réinitialiser l'indicateur de réception de données pour chaque ruche
    for (int i = 0; i < 5; i++) {
      ruches[i].dataReceived = false;
    }

    // Période d'écoute pour les données des ruches via BLE
    uint32_t listenStartTime = millis();
    while (millis() - listenStartTime < LISTEN_INTERVAL) {
      // Recevoir les données des ruches
      // La fonction onWrite est appelée lors de la réception des données
    }

    // Lire les capteurs environnementaux
    readEnvironmentalSensors();

    // Envoyer les données agrégées au réseau Meshtastic
    sendDataToGateway();

    // Mettre à jour le temps de la dernière transmission
    lastTransmissionTime = currentTime;
  }

  // Envoyer périodiquement des messages de synchronisation
  if (currentTime - lastSyncTime >= SYNC_INTERVAL) {
    time_t now;
    time(&now);
    String syncMessage = "SYNC," + String(now);
    pCharacteristic->setValue(syncMessage);
    pServer->getAdvertising()->start();
    delay(100); // Délai pour assurer la transmission
    pServer->getAdvertising()->stop();
    lastSyncTime = currentTime;
  }

  // Entrer en mode deep sleep
  esp_deep_sleep_start();
}
