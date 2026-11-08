#pragma once
/*
 * wifi_manager.h — WiFi station, IP fixe CIDR et mDNS (interface)
 *
 * Implémentation : lib/wifi-manager/src/wifi_manager.cpp
 */

namespace wifi {

// État de connexion (mis à jour par setup() et maintain())
extern bool connected;

// IP fixe depuis STATIC_IP_CIDR (config.h) puis connexion au réseau.
// Fallback DHCP si le CIDR est invalide. Démarre mDNS si connecté.
void setup();

// À appeler dans loop() : met à jour `connected`, relance la connexion
// toutes les 5 s en cas de perte.
void maintain();

} // namespace wifi