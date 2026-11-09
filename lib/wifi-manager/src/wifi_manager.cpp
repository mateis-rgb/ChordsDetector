#include "wifi_manager.h"
#include "config.h"
#include "settings_store.h"

#include <Arduino.h>
#include <ESPmDNS.h>
#include <WiFi.h>

namespace wifi {

bool connected = false;

namespace {

uint32_t lastTryMs = 0;

// "192.168.1.60/24" -> ip / masque / passerelle (première adresse du réseau).
bool parseCidr(const char* cidr, IPAddress& ip, IPAddress& mask, IPAddress& gw) {
    String s(cidr);
    int slash = s.indexOf('/');
    if (slash < 0 || !ip.fromString(s.substring(0, slash))) return false;
    int p = s.substring(slash + 1).toInt();
    if (p < 0 || p > 32) return false;

    uint32_t m = (p == 0) ? 0u : (0xFFFFFFFFu << (32 - p));
    uint8_t mb[4], gb[4];
    for (int i = 0; i < 4; i++) {
        mb[i] = (m >> (8 * (3 - i))) & 0xFF;
        gb[i] = ip[i] & mb[i];
    }
    gb[3] |= 1;   // passerelle par défaut : x.x.x.1
    mask = IPAddress(mb[0], mb[1], mb[2], mb[3]);
    gw   = IPAddress(gb[0], gb[1], gb[2], gb[3]);
    return true;
}

} // namespace

void setup() {
    // Réglages persistants (NVS) > défauts de config.h.
    // settings::setup() charge la configuration appliquée depuis l'app.
    settings::setup();
    const settings::Config& cfg = settings::get();

    IPAddress ip, mask, gw;
    if (cfg.ipCidr.isEmpty()) {
        Serial.println("[WiFi] DHCP (aucune IP fixe configurée)");
    } else if (parseCidr(cfg.ipCidr.c_str(), ip, mask, gw)) {
        if (cfg.gateway.length() > 0) gw.fromString(cfg.gateway);
        WiFi.config(ip, gw, mask, gw);   // DNS = passerelle
        Serial.printf("[WiFi] IP fixe : %s (gw %s)\n",
                      ip.toString().c_str(), gw.toString().c_str());
    } else {
        Serial.println("[WiFi] CIDR invalide -> DHCP");
    }

    WiFi.mode(WIFI_STA);
    WiFi.setHostname(cfg.hostname.c_str());
    WiFi.begin(cfg.ssid.c_str(), cfg.password.c_str());

    Serial.printf("[WiFi] Connexion à \"%s\" ", cfg.ssid.c_str());
    uint32_t t0 = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - t0 < WIFI_CONNECT_TIMEOUT_MS) {
        delay(100);
        Serial.print(".");
    }
    connected = (WiFi.status() == WL_CONNECTED);
    Serial.println(connected ? " OK" : " ÉCHEC");

    if (connected) {
        Serial.printf("[WiFi] http://%s.local ou http://%s\n",
                      cfg.hostname.c_str(), WiFi.localIP().toString().c_str());
        if (MDNS.begin(cfg.hostname.c_str())) {
            MDNS.addService("http", "tcp", 80);
            // Service dédié à la découverte automatique par l'app
            // (cf. DiscoveryService.swift — NWBrowser _chordsdetector._tcp)
            MDNS.addService("chordsdetector", "tcp", 80);
            Serial.printf("[mDNS] http://%s.local (+ service _chordsdetector._tcp)\n",
                          cfg.hostname.c_str());
        }
    }
}

void maintain() {
    connected = (WiFi.status() == WL_CONNECTED);
    if (!connected && millis() - lastTryMs > 5000) {
        lastTryMs = millis();
        WiFi.disconnect();
        WiFi.reconnect();
    }
}

} // namespace wifi