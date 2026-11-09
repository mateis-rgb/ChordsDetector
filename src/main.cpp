/*
 * main.cpp — Détecteur d'accords : ESP32-S3 + clavier MIDI USB + page web
 *
 * Orchestration uniquement : chaque module est une bibliothèque privée
 * PlatformIO dans lib/<nom>/src/ (interface + implémentation).
 *
 *   wifi_manager   : connexion WiFi, IP fixe CIDR, mDNS (+ service _chordsdetector._tcp)
 *   midi_input     : USB Host MIDI, suivi des notes actives
 *   chord_detector : reconnaissance d'accords (utilisé par web_server)
 *   web_server     : serveur HTTP + WebSocket temps réel (+ hello app → appConnected)
 *   web_page       : page HTML « Scène Néon » embarquée
 *   status_led     : LED RGB d'état (WiFi / clavier / app connectée)
 */

#include <Arduino.h>

#include "midi_input.h"
#include "status_led.h"
#include "web_server.h"
#include "wifi_manager.h"

void setup() {
    Serial.begin(115200);
    delay(200);
    Serial.println("\n=== Détecteur d'accords — ESP32-S3 ===");

    statusled::begin();
    wifi::setup();
    web::setup();
    midi::setup();
}

void loop() {
    const bool notesChanged = midi::poll();

    wifi::maintain();
    statusled::update(wifi::connected, midi::keyboardConnected(), web::appConnected());

    if (notesChanged) web::broadcastState();   // réaction immédiate à l'appui
    web::loop();                               // broadcast périodique + nettoyage
}