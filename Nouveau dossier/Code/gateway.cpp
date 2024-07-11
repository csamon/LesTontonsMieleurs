#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <HTTPClient.h>
#include <Meshtastic.h>

// Configuration WiFi
const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";

// Configuration DataCake
const char* dataCakeApiKey = "YOUR_DATACAKE_API_KEY";
const char* dataCakeDeviceId = "YOUR_DATACAKE_DEVICE_ID";
const char* dataCakeUrl = "https://api.datacake.co/v1/devices";

// Initialisation du serveur Web pour les mises à jour OTA (Over-the-Air)
AsyncWebServer server(80);

void setup() {
  Serial.begin(115200);

  // Connexion au WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // Initialisation Meshtastic
  startMeshtastic();

  // Initialisation du serveur Web
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", "Hello, this is the gateway!");
  });
  server.begin();
}

void loop() {
  // Vérifier les messages Meshtastic
  while (Serial.available() > 0) {
    String message = Serial.readStringUntil('\n');
    Serial.println("Received message from Meshtastic: " + message);

    // Envoyer les données à DataCake
    sendToDataCake(message);
  }
}

// Fonction pour envoyer les données à DataCake
void sendToDataCake(String message) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(dataCakeUrl + String("/") + dataCakeDeviceId + String("/data"));
    http.addHeader("Content-Type", "application/json");
    http.addHeader("X-API-KEY", dataCakeApiKey);

    String jsonData = formatDataForDataCake(message);
    int httpResponseCode = http.POST(jsonData);

    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println("HTTP Response code: " + String(httpResponseCode));
      Serial.println("Response: " + response);
    } else {
      Serial.println("Error on sending POST: " + String(httpResponseCode));
    }

    http.end();
  } else {
    Serial.println("WiFi Disconnected");
  }
}

// Fonction pour formater les données pour DataCake
String formatDataForDataCake(String message) {
  // Analyser le message reçu et formater les données pour DataCake
  // Format attendu du message : "Ruche1,Temp,25.6,Hum,60.2,Vib,0,Sound,50,Weight,15.2"
  // Ou : "Serre,Temp,25.6,Hum,60.2,Press,1013.2,Light,300,Soil,40"
  String jsonData = "{";

  if (message.startsWith("Ruche")) {
    int rucheIndex;
    float temp, hum, weight;
    int vib, sound;
    sscanf(message.c_str(), "Ruche%d,Temp,%f,Hum,%f,Vib,%d,Sound,%d,Weight,%f",
           &rucheIndex, &temp, &hum, &vib, &sound, &weight);

    jsonData += "\"ruche_index\":" + String(rucheIndex) + ",";
    jsonData += "\"temperature\":" + String(temp) + ",";
    jsonData += "\"humidity\":" + String(hum) + ",";
    jsonData += "\"vibration\":" + String(vib) + ",";
    jsonData += "\"sound_level\":" + String(sound) + ",";
    jsonData += "\"weight\":" + String(weight);
  } else if (message.startsWith("Serre")) {
    float temp, hum, press;
    int light, soil;
    sscanf(message.c_str(), "Serre,Temp,%f,Hum,%f,Press,%f,Light,%d,Soil,%d",
           &temp, &hum, &press, &light, &soil);

    jsonData += "\"temperature\":" + String(temp) + ",";
    jsonData += "\"humidity\":" + String(hum) + ",";
    jsonData += "\"pressure\":" + String(press) + ",";
    jsonData += "\"light_level\":" + String(light) + ",";
    jsonData += "\"soil_moisture\":" + String(soil);
  }

  jsonData += "}";
  return jsonData;
}
