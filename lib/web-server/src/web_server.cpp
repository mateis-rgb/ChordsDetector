#include "web_server.h"
#include "config.h"
#include "chord_detector.h"
#include "midi_input.h"
#include "web_page.h"
#include "wifi_manager.h"
#include "settings_store.h"

#include <Arduino.h>
#include <ESPAsyncWebServer.h>

#include <unordered_set>

namespace web {

namespace {

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
uint32_t lastBroadcastMs = 0;

// Redémarrage demandé par une app (effectué dans loop(), pas dans le
// callback WebSocket : on laisse la réponse partir avant l'arrêt).
bool rebootRequested = false;

// Clients identifiés comme « app native » (message hello) — pour la LED
std::unordered_set<uint32_t> appClients;

// État complet -> JSON :
// {"type":"state","count":N,"chord":"Cmaj7","wifi":true,"com":true,
//  "notes":[{"n":"C4","m":60},…]}
String buildStateJson() {
    uint8_t notes[32];
    uint8_t count = midi::activeNotes(notes, sizeof(notes));

    uint16_t pcMask = 0;
    for (uint8_t i = 0; i < count; i++) pcMask |= 1u << (notes[i] % 12);
    int8_t lowest = (count > 0) ? notes[0] : -1;

    chords::ChordInfo info = chords::detect(pcMask, lowest, count);

    String s;
    s.reserve(320);
    s += "{\"type\":\"state\",\"count\":";  s += count;
    s += ",\"chord\":\"";                   s += info.name;
    s += "\",\"wifi\":";                   s += (wifi::connected ? "true" : "false");
    s += ",\"com\":";                       s += (midi::keyboardConnected() ? "true" : "false");
    s += ",\"notes\":[";
    for (uint8_t i = 0; i < count; i++) {
        char n[6];
        chords::noteName(notes[i], n, sizeof(n));
        if (i) s += ',';
        s += "{\"n\":\""; s += n; s += "\",\"m\":"; s += notes[i]; s += '}';
    }
    s += "]}";
    return s;
}

// ---- Protocole v1.2 : réglages du boîtier ----

// Échappe " et \ pour un contenu JSON sûr.
String jsonEscape(const String& in) {
    String out;
    out.reserve(in.length());
    for (unsigned i = 0; i < in.length(); i++) {
        char c = in[i];
        if (c == '"' || c == '\\') out += '\\';
        out += c;
    }
    return out;
}

// Extrait la valeur chaîne de « "clé":"valeur" » ("" si absente).
String jsonStr(const String& msg, const char* key) {
    String k = String("\"") + key + "\":\"";
    int i = msg.indexOf(k);
    if (i < 0) return "";
    i += k.length();
    int j = msg.indexOf('"', i);
    return msg.substring(i, j);
}

// Configuration réseau -> JSON (protocole v1.2, réponse à get-config).
// Le mot de passe n'est JAMAIS renvoyé : seulement son existence.
String buildConfigJson() {
    const settings::Config& c = settings::get();
    String s;
    s += "{\"type\":\"config\",\"v\":2";
    s += ",\"ssid\":\"";         s += jsonEscape(c.ssid);
    s += "\",\"hasPass\":";      s += (c.password.length() > 0) ? "true" : "false";
    s += ",\"ip\":\"";           s += jsonEscape(c.ipCidr);
    s += "\",\"gateway\":\"";    s += jsonEscape(c.gateway);
    s += "\",\"hostname\":\"";   s += jsonEscape(c.hostname);
    s += "\"}";
    return s;
}

void onWsEvent(AsyncWebSocket*, AsyncWebSocketClient* client,
               AwsEventType type, void* arg, uint8_t* data, size_t len) {
    if (type == WS_EVT_CONNECT) {
        Serial.printf("[WS] client %u connecté\n", client->id());
        client->text(buildStateJson());          // état complet au nouvel arrivant
    } else if (type == WS_EVT_DISCONNECT) {
        appClients.erase(client->id());          // l'app part -> LED revient au bleu fixe
    } else if (type == WS_EVT_DATA) {
        AwsFrameInfo* f = (AwsFrameInfo*)arg;
        if (!(f->final && f->index == 0 && f->len == len)) return;
        String msg;
        msg.reserve(len);
        for (size_t i = 0; i < len; i++) msg += (char)data[i];
        if (msg.startsWith("{\"type\":\"ping\"")) {   // écho immédiat (test réactivité)
            int id = -1;
            sscanf(msg.c_str(), "{\"type\":\"ping\",\"id\":%d", &id);
            client->printf("{\"type\":\"pong\",\"id\":%d}", id);
        } else if (msg.indexOf("\"type\":\"hello\"") >= 0) {
            // Identification d'une app native -> LED bleue clignotante
            if (msg.indexOf("chordsdetector-app") >= 0) appClients.insert(client->id());
        } else if (msg.indexOf("\"type\":\"get-config\"") >= 0) {
            client->text(buildConfigJson());
        } else if (msg.indexOf("\"type\":\"set-config\"") >= 0) {
            // Champs présents = mis à jour, absents = inchangés.
            settings::Config c = settings::get();
            String v;
            if (!(v = jsonStr(msg, "ssid")).isEmpty())     c.ssid = v;
            if (!(v = jsonStr(msg, "pass")).isEmpty())     c.password = v;
            if (msg.indexOf("\"ip\":\"\"") >= 0)           c.ipCidr = "";   // "" explicite -> DHCP
            else if (!(v = jsonStr(msg, "ip")).isEmpty())  c.ipCidr = v;
            if (msg.indexOf("\"gateway\":") >= 0)          c.gateway = jsonStr(msg, "gateway");
            if (!(v = jsonStr(msg, "hostname")).isEmpty()) c.hostname = v;
            bool ok = settings::save(c);
            Serial.printf("[WS] set-config : ssid=%s ip=%s host=%s (nv=%d)\n",
                          c.ssid.c_str(),
                          c.ipCidr.isEmpty() ? "DHCP" : c.ipCidr.c_str(),
                          c.hostname.c_str(), ok ? 1 : 0);
            client->printf("{\"type\":\"config-result\",\"ok\":%s,\"rebootRequired\":true}",
                           ok ? "true" : "false");
        } else if (msg.indexOf("\"type\":\"reboot\"") >= 0) {
            Serial.println("[WS] Redémarrage demandé par l'app");
            client->printf("{\"type\":\"config-result\",\"ok\":true,\"rebooting\":true}");
            rebootRequested = true;   // effectué dans loop(), après l'envoi
        }
    }
}

} // namespace

void setup() {
    ws.onEvent(onWsEvent);
    server.addHandler(&ws);

    server.on("/", HTTP_GET, [](AsyncWebServerRequest* req) {
        req->send(200, "text/html", web_page::INDEX_HTML);
    });
    server.on("/health", HTTP_GET, [](AsyncWebServerRequest* req) {
        req->send(200, "application/json",
                  String("{\"wifi\":") + (wifi::connected ? "true" : "false") +
                  ",\"com\":" + (midi::keyboardConnected() ? "true" : "false") + "}");
    });
    server.begin();
    Serial.println("[Web] Serveur démarré sur le port 80");
}

void broadcastState() {
    if (ws.count() > 0) ws.textAll(buildStateJson());
}

void loop() {
    if (rebootRequested) {
        Serial.println("[Web] Redémarrage…");
        delay(250);          // laisse partir la réponse WebSocket
        ESP.restart();
    }
    if (millis() - lastBroadcastMs >= BROADCAST_PERIOD_MS) {
        lastBroadcastMs = millis();
        broadcastState();      // rafraîchit le statut (WiFi/clavier) des clients
    }
    ws.cleanupClients();
}

bool appConnected() { return !appClients.empty(); }

} // namespace web