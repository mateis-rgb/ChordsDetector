#include "settings_store.h"
#include "config.h"

#include <Arduino.h>
#include <Preferences.h>

namespace settings {

namespace {
	Preferences prefs;
	Config current;
	bool loaded = false;
}

void setup() {
    if (loaded) return;
    prefs.begin("chords", true /* lecture seule */);
    current.ssid     = prefs.getString("ssid",  WIFI_SSID);
    current.password = prefs.getString("pass",  WIFI_PASSWORD);
    current.ipCidr   = prefs.getString("ip",    STATIC_IP_CIDR);
    current.gateway  = prefs.getString("gw",    GATEWAY_OVERRIDE);
    current.hostname = prefs.getString("host",  HOSTNAME);
    prefs.end();
    loaded = true;
    Serial.printf("[Config] ssid=\"%s\" ip=%s host=%s\n",
                  current.ssid.c_str(),
                  current.ipCidr.isEmpty() ? "DHCP" : current.ipCidr.c_str(),
                  current.hostname.c_str());
}

const Config& get() { return current; }

bool save(const Config& c) {
    current = c;
    if (!prefs.begin("chords", false)) return false;
    prefs.putString("ssid", c.ssid);
    prefs.putString("pass", c.password);
    prefs.putString("ip",   c.ipCidr);
    prefs.putString("gw",   c.gateway);
    prefs.putString("host", c.hostname);
    prefs.end();
    Serial.println("[Config] Réglages enregistrés en NVS (appliqués au redémarrage)");
    return true;
}

} // namespace settings