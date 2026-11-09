#pragma once
/*
 * settings_store.h — Réglages persistants du boîtier (interface)
 *
 * Stocke en NVS (flash) les réglages réseau appliqués depuis l'app
 * (protocole v1.2) : SSID, mot de passe, IP fixe, passerelle, nom mDNS.
 * config.h ne fournit plus que les DÉFAUTS : dès qu'un réglage a été
 * modifié depuis l'app, la NVS prime sur config.h.
 *
 * Implémentation : lib/settings-store/src/settings_store.cpp
 */

#include <Arduino.h>

namespace settings {

struct Config {
    String ssid;      // nom du réseau WiFi
    String password;  // mot de passe WiFi
    String ipCidr;    // "192.168.1.60/24" ; vide -> DHCP
    String gateway;   // "" -> passerelle auto (x.x.x.1)
    String hostname;  // nom mDNS ("accords" -> http://accords.local)
};

// Charge la configuration depuis la NVS (défauts de config.h si vierge).
// À appeler une fois au démarrage, avant wifi::setup().
void setup();

// Configuration active (celle chargée au dernier démarrage).
const Config& get();

// Persiste une nouvelle configuration en NVS et la rend active.
// Retourne false si l'écriture NVS a échoué.
bool save(const Config& c);

} // namespace settings