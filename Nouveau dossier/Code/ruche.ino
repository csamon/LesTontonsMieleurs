#include <BLEDevice.h>
#include <BLEClient.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <esp_sleep.h>
#include <Wire.h>
#include <time.h>

// UUIDs pour BLE - Identifiants uniques pour le service et la caractéristique BLE
#define SERVICE_UUID "91bad492-b950-4226-aa2b-4ede9fa42f59"
#define CHARACTERISTIC_UUID "cba1d466-344c-4be3-ab3f-189f80dd7518"

// Intervalle de lecture des capteurs (30 secondes) et transmission (10 minutes)
#define SENSOR_READ_INTERVAL 30000 // 30 secondes en millisecondes
#define TRANSMISSION_INTERVAL 600000 // 10 minutes en millisecondes

// Configuration des capteurs et de la LED
#define SHT30_ADDR 0x44 // Adresse I2C du capteur SHT30
#define VIBRATION_PIN 34 // Broche pour le capteur de vibration
#define SOUND_PIN 35 // Broche pour le capteur de son
#define WEIGHT_PIN 32 // Broche pour le capteur de poids
#define LED_PIN 2 // Broche pour la LED

// Seuils arbitraires pour la détection d'alarme
#define TEMP_THRESHOLD 5.0      // Variation rapide de température en °C
#define HUM_THRESHOLD 10.0      // Variation rapide d'humidité en %
#define VIB_THRESHOLD 1         // Changement de vibration
#define SOUND_THRESHOLD 200     // Variation rapide du niveau sonore
#define WEIGHT_THRESHOLD 10     // Variation rapide du poids

BLEClient* pClient;
BLERemoteCharacteristic* pRemoteCharacteristic;
time_t lastSyncTime = 0; // Dernière synchronisation avec le hub
uint32_t lastSensorReadTime = 0; // Dernière lecture des capteurs
uint32_t lastTransmissionTime = 0; // Dernière transmission des données
int dephasingTime = 0; // Déphasage aléatoire pour éviter les conflits de transmission
bool alarmTriggered = false; // Indicateur d'alarme déclenchée
uint32_t alarmStartTime = 0; // Heure de début de l'alarme
String alarmParameter = ""; // Paramètre ayant déclenché l'alarme
float alarmThreshold = 0.0; // Seuil ayant déclenché l'alarme

// Variables pour stocker les dernières valeurs des capteurs
float lastTemperature = 0;
float lastHumidity = 0;
int lastVibration = 0;
int lastSoundLevel = 0;
int lastWeight = 0;

// Variables pour stocker les 20 dernières lectures
float temperatureReadings[20] = {0};
float humidityReadings[20] = {0};
int vibrationReadings[20] = {0};
int soundReadings[20] = {0};
int weightReadings[20] = {0};
int readingIndex = 0; // Index pour les lectures circulaires
int totalReadings = 0; // Nombre total de lectures effectuées

// Fonction pour lire les capteurs
void readSensors(float &temperature, float &humidity, int &vibration, int &soundLevel, int &weight) {
  // Lecture du capteur SHT30 pour la température et l'humidité
  Wire.beginTransmission(SHT30_ADDR);
  Wire.write(0x2C);
  Wire.write(0x06);
  Wire.endTransmission();
  delay(100);
  Wire.requestFrom(SHT30_ADDR, 6);
  if (Wire.available() == 6) {
    uint16_t t = Wire.read() << 8 | Wire.read();
    uint16_t h = Wire.read() << 8 | Wire.read();
    temperature = -45 + 175 * (t / 65535.0);
    humidity = 100 * (h / 65535.0);
    Serial.print("Temperature: "); Serial.println(temperature);
    Serial.print("Humidity: "); Serial.println(humidity);
  }

  // Lecture du capteur de vibration
  vibration = digitalRead(VIBRATION_PIN);
  Serial.print("Vibration: "); Serial.println(vibration);

  // Lecture du capteur de son
  soundLevel = analogRead(SOUND_PIN);
  Serial.print("Sound Level: "); Serial.println(soundLevel);

  // Lecture du capteur de poids
  weight = analogRead(WEIGHT_PIN); // Remplacer par le code spécifique à HX711 si nécessaire
  Serial.print("Weight: "); Serial.println(weight);

  // Stocker les lectures dans les tableaux circulaires
  temperatureReadings[readingIndex] = temperature;
  humidityReadings[readingIndex] = humidity;
  vibrationReadings[readingIndex] = vibration;
  soundReadings[readingIndex] = soundLevel;
  weightReadings[readingIndex] = weight;

  // Mettre à jour l'index pour la prochaine lecture
  readingIndex = (readingIndex + 1) % 20;
  if (totalReadings < 20) {
    totalReadings++;
  }
}

// Fonction pour calculer les moyennes glissantes
void calculateAverages(float &avgTemperature, float &avgHumidity, int &avgVibration, int &avgSoundLevel, int &avgWeight) {
  float sumTemperature = 0;
  float sumHumidity = 0;
  int sumVibration = 0;
  int sumSoundLevel = 0;
  int sumWeight = 0;

  for (int i = 0; i < totalReadings; i++) {
    sumTemperature += temperatureReadings[i];
    sumHumidity += humidityReadings[i];
    sumVibration += vibrationReadings[i];
    sumSoundLevel += soundReadings[i];
    sumWeight += weightReadings[i];
  }

  avgTemperature = sumTemperature / totalReadings;
  avgHumidity = sumHumidity / totalReadings;
  avgVibration = sumVibration / totalReadings;
  avgSoundLevel = sumSoundLevel / totalReadings;
  avgWeight = sumWeight / totalReadings;
}

// Fonction pour vérifier les conditions d'alarme
bool checkForAlarm(float temperature, float humidity, int vibration, int soundLevel, int weight) {
  // Vérifier les variations rapides des capteurs par rapport aux dernières valeurs lues
  if (abs(temperature - lastTemperature) > TEMP_THRESHOLD) {
    alarmParameter = "Temperature";
    alarmThreshold = TEMP_THRESHOLD;
    return true;
  } else if (abs(humidity - lastHumidity) > HUM_THRESHOLD) {
    alarmParameter = "Humidity";
    alarmThreshold = HUM_THRESHOLD;
    return true;
  } else if (abs(vibration - lastVibration) > VIB_THRESHOLD) {
    alarmParameter = "Vibration";
    alarmThreshold = VIB_THRESHOLD;
    return true;
  } else if (abs(soundLevel - lastSoundLevel) > SOUND_THRESHOLD) {
    alarmParameter = "Sound";
    alarmThreshold = SOUND_THRESHOLD;
    return true;
  } else if (abs(weight - lastWeight) > WEIGHT_THRESHOLD) {
    alarmParameter = "Weight";
    alarmThreshold = WEIGHT_THRESHOLD;
    return true;
  }

  // Mettre à jour les dernières valeurs lues
  lastTemperature = temperature;
  lastHumidity = humidity;
  lastVibration = vibration;
  lastSoundLevel = soundLevel;
  lastWeight = weight;

  return false; // Aucune alarme déclenchée
}

// Fonction pour envoyer une alarme via BLE et déclencher la LED
void sendAlarm() {
  // Créer un message d'alarme avec l'identifiant de la ruche, le paramètre ayant déclenché l'alarme et le seuil
  std::string alarmMessage = "Ruche1,Alarm," + alarmParameter + "," + String(alarmThreshold);
  pRemoteCharacteristic->writeValue(alarmMessage);
  alarmTriggered = true;
  alarmStartTime = millis();
  digitalWrite(LED_PIN, HIGH); // Allumer la LED
}

// Fonction pour envoyer des données périodiques via BLE
void sendPeriodicData(float temperature, float humidity, int vibration, int soundLevel, int weight) {
  // Créer un message de données périodiques avec les valeurs moyennes des capteurs
  std::string dataMessage = "Ruche1,Temp," + String(temperature) +
                            ",Hum," + String(humidity) +
                            ",Vib," + String(vibration) +
                            ",Sound," + String(soundLevel) +
                            ",Weight," + String(weight);
  pRemoteCharacteristic->writeValue(dataMessage);
}

// Fonction pour gérer le clignotement de la LED
void handleLED() {
  if (alarmTriggered) {
    uint32_t currentTime = millis();
    if (currentTime - alarmStartTime >= 3600000) { // Arrêter le clignotement après 1 heure
      alarmTriggered = false;
      digitalWrite(LED_PIN, LOW);
    } else {
      // Faire clignoter la LED à 1 Hz
      if ((currentTime / 500) % 2 == 0) {
        digitalWrite(LED_PIN, HIGH);
      } else {
        digitalWrite(LED_PIN, LOW);
      }
    }
  }
}

// Callback pour recevoir les notifications BLE
void notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
  std::string value((char*)pData, length);
  Serial.print("Received notification: "); Serial.println(value.c_str());

  if (value.starts_with("SYNC,")) {
    String timestampStr = value.substring(5);
    time_t timestamp = (time_t)timestampStr.toInt();
    lastSyncTime = timestamp;
    Serial.print("Synchronized time: "); Serial.println(lastSyncTime);
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT); // Configurer la broche de la LED comme sortie

  // Initialisation de la communication BLE
  BLEDevice::init("");
  pClient = BLEDevice::createClient();
  pClient->setClientCallbacks(new MyClientCallbacks());

  // Connexion au hub via son adresse MAC
  pClient->connect(BLEAddress("XX:XX:XX:XX:XX:XX")); // Remplacer par l'adresse MAC du hub

  // Recherche du service et de la caractéristique BLE
  BLERemoteService* pRemoteService = pClient->getService(SERVICE_UUID);
  if (pRemoteService == nullptr) {
    Serial.print("Failed to find service UUID: ");
    Serial.println(SERVICE_UUID);
    pClient->disconnect();
    return;
  }

  pRemoteCharacteristic = pRemoteService->getCharacteristic(CHARACTERISTIC_UUID);
  if (pRemoteCharacteristic == nullptr) {
    Serial.print("Failed to find characteristic UUID: ");
    Serial.println(CHARACTERISTIC_UUID);
    pClient->disconnect();
    return;
  }

  if (pRemoteCharacteristic->canNotify())
    pRemoteCharacteristic->registerForNotify(notifyCallback);

  randomSeed(analogRead(0)); // Initialiser le générateur de nombres aléatoires
  dephasingTime = random(0, 60000); // Déphasage entre 0 et 60 secondes
}

void loop() {
  // Obtenir le temps actuel
  uint32_t currentTime = millis();

  // Variables pour stocker les valeurs lues des capteurs
  float temperature = 0;
  float humidity = 0;
  int vibration = 0;
  int soundLevel = 0;
  int weight = 0;

  // Lire les capteurs toutes les 30 secondes
  if (currentTime - lastSensorReadTime >= SENSOR_READ_INTERVAL) {
    readSensors(temperature, humidity, vibration, soundLevel, weight);
    lastSensorReadTime = currentTime;

    // Vérifier les conditions d'alarme
    if (checkForAlarm(temperature, humidity, vibration, soundLevel, weight)) {
      sendAlarm();
    }
  }

  // Envoyer des données périodiques toutes les 10 minutes avec déphasage
  if (currentTime - lastTransmissionTime >= TRANSMISSION_INTERVAL + dephasingTime) {
    float avgTemperature = 0;
    float avgHumidity = 0;
    int avgVibration = 0;
    int avgSoundLevel = 0;
    int avgWeight = 0;

    // Calculer les moyennes glissantes
    calculateAverages(avgTemperature, avgHumidity, avgVibration, avgSoundLevel, avgWeight);

    sendPeriodicData(avgTemperature, avgHumidity, avgVibration, avgSoundLevel, avgWeight);
    lastTransmissionTime = currentTime;
  }

  // Gérer le clignotement de la LED
  handleLED();

  // Entrer en mode deep sleep
  esp_deep_sleep_start();
}
