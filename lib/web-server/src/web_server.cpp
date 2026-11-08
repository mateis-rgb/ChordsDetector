#include "web_server.h"
#include "config.h"
#include "chord_detector.h"
#include "midi_input.h"
#include "web_page.h"
#include "wifi_manager.h"

#include <Arduino.h>
#include <ESPAsyncWebServer.h>

namespace web {

namespace {

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
uint32_t lastBroadcastMs = 0;

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

void onWsEvent(AsyncWebSocket*, AsyncWebSocketClient* client,
               AwsEventType type, void* arg, uint8_t* data, size_t len) {
    if (type == WS_EVT_CONNECT) {
        Serial.printf("[WS] client %u connecté\n", client->id());
        client->text(buildStateJson());          // état complet au nouvel arrivant
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
    if (millis() - lastBroadcastMs >= BROADCAST_PERIOD_MS) {
        lastBroadcastMs = millis();
        broadcastState();      // rafraîchit le statut (WiFi/clavier) des clients
    }
    ws.cleanupClients();
}

} // namespace web