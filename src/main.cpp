/*
 * main.cpp — Détecteur d'accords : ESP32-S3 + clavier MIDI USB + page web
 *
 * Orchestration uniquement : chaque module vit dans include/<module>.h
 * (déclarations) et src/<module>.cpp (implémentation).
 *
 *   wifi_manager   : connexion WiFi, IP fixe CIDR, mDNS
 *   midi_input     : USB Host MIDI, suivi des notes actives
 *   chord_detector : reconnaissance d'accords (utilisé par web_server)
 *   web_server     : serveur HTTP + WebSocket temps réel
 *   web_page       : page HTML « Scène Néon » embarquée
 *   status_led     : LED RGB d'état (WiFi / clavier)
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
    statusled::update(wifi::connected, midi::keyboardConnected());

    if (notesChanged) web::broadcastState();   // réaction immédiate à l'appui
    web::loop();                               // broadcast périodique + nettoyage
}