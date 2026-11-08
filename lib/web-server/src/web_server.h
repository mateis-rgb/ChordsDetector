#pragma once
/*
 * web_server.h — Serveur HTTP + WebSocket temps réel (interface)
 *
 * Implémentation : lib/web-server/src/web_server.cpp
 */

namespace web {

// Démarre le serveur : page web (src/web_page.cpp), WebSocket /ws,
// route /health. À appeler après wifi::setup().
void setup();

// Pousse l'état complet (accord, notes, WiFi, clavier) à tous les
// clients WebSocket connectés. À appeler dès qu'une note change.
void broadcastState();

// À appeler dans loop() : broadcast périodique (BROADCAST_PERIOD_MS,
// cf. config.h) pour rafraîchir le statut + nettoyage des clients morts.
void loop();

} // namespace web