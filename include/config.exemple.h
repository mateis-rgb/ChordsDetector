#pragma once
/*
 * config.h — Configuration du détecteur d'accords
 * C'est LE fichier à adapter avant de flasher.
 */

// ---------------- WiFi (station) ----------------
const char* WIFI_SSID     = "SSID";
const char* WIFI_PASSWORD = "PASSWORD";

// Adresse IPv4 fixe en notation CIDR, ex. "192.168.1.60/24"
// (masque déduit du /xx ; si la syntaxe est invalide, on retombe en DHCP)
const char* STATIC_IP_CIDR = "";

// Passerelle : par défaut la première adresse du réseau (x.x.x.1).
// Pour la forcer, mettre "192.168.1.254" ; sinon laisser "".
const char* GATEWAY_OVERRIDE = "";

// Nom mDNS -> http://accords.local (fonctionne dans Safari iOS et macOS)
const char* HOSTNAME = "accords";

// ---------------- LED RGB intégrée ----------------
// C'est une WS2812 soudée sur la carte :
//   - la plupart des cartes N16R8 et DevKitC-1 v1.0 : GPIO48
//   - DevKitC-1 v1.1 et certains clones               : GPIO38
// Si aucune LED ne s'allume, essayez l'autre valeur.
int RGB_LED_PIN = 48;

char LED_BRIGHTNESS     = 50;   // 0-255 (50 suffit largement)
int LED_BLINK_PERIOD_MS = 400; // période du clignotement

// ---------------- Divers ----------------
int WIFI_CONNECT_TIMEOUT_MS = 15000; // attente connexion WiFi
int BROADCAST_PERIOD_MS     = 2000; // état poussé aux clients toutes les 2 s