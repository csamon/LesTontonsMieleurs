# Diagramme Synoptique du Projet de Ruche Connectée

## Configuration Complète
- **15 Ruches** réparties en 3 Clusters (5 à 10 ruches par cluster)
- **3 Hubs** (1 par cluster)
- **1 Serre**
- **1 Passerelle**

## Schéma Simplifié
### Passerelle, Serre, Hub et 2 Ruches

```plaintext
                           ----------------------
                          |       Passerelle      |
                          |  (Gateway - LoRaWAN)  |
                          ----------------------
                                   |
                                   |
                         ---------------------
                        |       LoRa         |
                        |    Meshtastic      |
                        ---------------------
                                   |
                                   |
         -------------------------------------------------
        |                                                 |
        |                                                 |
  -------------                                    -------------
 |    Serre    |                                  |    Hub      |
 |-------------|                                  |-------------|
 | BME280: Temp, Hum, Pression (I2C)              | BME280: Temp, Hum, Pression (I2C) |
 | BH1750: Lumière (I2C)                          | Capteur d'humidité du sol (Analogique) |
 | Humidité du sol (Analogique)                   | LoRa Meshtastic                       |
 | LoRa Meshtastic                                | BLE                                   |
 -------------                                    -------------
        |                                                 |
        |                                                 |
        |                      ---------------------------|-------------------------------
        |                      |                                                   |
        |                      |                                                   |
  -------------           -------------                                      -------------
 |   Ruche 1   |         |   Ruche 2   |                                    |   Ruche 3   |
 |-------------|         |-------------|                                    |-------------|
 | SHT30: Temp, Hum (I2C)| SHT30: Temp, Hum (I2C)                           | SHT30: Temp, Hum (I2C) |
 | SW-420: Vibration (Analogique) | SW-420: Vibration (Analogique)          | SW-420: Vibration (Analogique) |
 | MAX4466: Son (Analogique)      | MAX4466: Son (Analogique)               | MAX4466: Son (Analogique) |
 | HX711: Poids (I2C)             | HX711: Poids (I2C)                      | HX711: Poids (I2C) |
 | BLE                            | BLE                                      | BLE |
 -------------                    -------------                              -------------

Timings et Fréquences

    Lecture des capteurs (Ruches): Toutes les 30 secondes
    Envoi des données (Ruches -> Hub): Toutes les 10 minutes
    Envoi des données (Hub -> Passerelle): Toutes les 10 minutes
    Lecture des capteurs (Hub): Toutes les 10 minutes
    Lecture des capteurs (Serre): Toutes les 10 minutes
    Déclenchement d'alarme: Immédiat si variation rapide détectée

Notes

    Technologies de Communication:
        Ruches -> Hubs: BLE (Bluetooth Low Energy)
        Hubs -> Passerelle: LoRa Meshtastic
        Serre -> Passerelle: LoRa Meshtastic

Fréquences d'envoi
Avec 2500 Datapoints/jour

    5 ruches, 1 hub: toutes les 17.35 minutes
    5 ruches, 1 hub, 1 serre: toutes les 19.73 minutes
    10 ruches, 2 hubs, 1 serre: toutes les 37.89 minutes
    15 ruches, 3 hubs, 1 serre: toutes les 53.33 minutes

Avec 7500 Datapoints/jour

    5 ruches, 1 hub: toutes les 5.76 minutes
    5 ruches, 1 hub, 1 serre: toutes les 6.55 minutes
    10 ruches, 2 hubs, 1 serre: toutes les 12.74 minutes
    15 ruches, 3 hubs, 1 serre: toutes les 17.35 minutes

Instructions pour Utilisation

    Copier le texte ci-dessus dans un fichier Markdown (.md) dans VSCode.
    Utiliser l'extension Markdown Preview Enhanced pour visualiser le diagramme en temps réel.
    **Modifier les configurations spécifiques ou ajouter des éléments supplémentaires selon les besoins.

Légende

    Ruches:
        SHT30: Température, Humidité (I2C)
        SW-420: Vibration (Analogique)
        MAX4466: Son (Analogique)
        HX711: Poids (I2C)

    Hubs:
        BME280: Température, Humidité, Pression (I2C)
        Capteur d'humidité du sol: (Analogique)

    Serre:
        BME280: Température, Humidité, Pression (I2C)
        BH1750: Lumière (I2C)
        Capteur d'humidité du sol: (Analogique)

    Communication:
        Ruches -> Hubs: BLE (Bluetooth Low Energy)
        Hubs -> Passerelle: LoRa Meshtastic
        Serre -> Passerelle: LoRa Meshtastic

Ce diagramme et ces instructions permettent de visualiser et de gérer facilement la structure du projet de ruche connectée, tout en surveillant les différentes actions et événements via le BLE et LoRa Meshtastic.