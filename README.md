# LesTontonsMieleurs
 Ruches connectées DIY, Lora, BLE

## Description

Ce projet de ruches connectées utilise des cartes HelTec LoRa pour surveiller et gérer des ruches situées dans un environnement rural en Bretagne. Les ruches sont équipées de divers capteurs pour mesurer :

- **Température et Humidité**
- **Poids**
- **Activité sonore**
- **(CO2)**
- **(Qualité de l'air)**
- **(Vibrations)**

## Fonctionnement

1. **Transmission des Données** :
   - Les ruches transmettent leurs relevés toutes les 10 minutes via BLE aux hubs situés au milieu des zones de ruches.
   - Les hubs collectent et formatent les données de 5 ruches, puis envoient un paquet de données via LoRa à une passerelle Wi-Fi dans la maison.

2. **Infrastructure** :
   - **Maison** : Passerelle Wi-Fi pour recevoir et transmettre les données au cloud.
   - **Serre** : Relais pour assurer une connexion stable entre les hubs et la passerelle.
   - **Zones de Ruches** : Hubs pour collecter les données des capteurs.

## Objectifs

- Améliorer la surveillance et la gestion des ruches.
- Utiliser des technologies de pointe pour assurer la santé des abeilles.
- S'assurer de la bonne santé des abeilles grâce à une analyse détaillée des données collectées.

## Technologies Utilisées

- **Cartes HelTec LoRa**
- **BLE (Bluetooth Low Energy)**
- **Capteurs de température, humidité, poids, sonore, (CO2, qualité de l'air et vibrations)**
- **ThingsBoard pour le stockage des données**
- **(Panneaux solaires pour une alimentation énergétique durable)**

## Auteurs

- [Ton Nom]
- [Collaborateurs]

Pour plus de détails, veuillez consulter la documentation complète dans le dépôt.