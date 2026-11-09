#pragma once
/*
 * status_led.h — LED RGB d'état (interface)
 *
 *  ROUGE  (clignote) : WiFi HS + clavier HS
 *  VERT   (clignote) : WiFi OK + clavier HS
 *  BLEU   (fixe)     : WiFi OK + clavier OK, aucune app connectée
 *  BLEU   (clignote) : WiFi OK + clavier OK + app connectée (hello)
 *  JAUNE  (clignote) : WiFi HS + clavier OK (cas complémentaire)
 *
 * Implémentation : lib/status-led/src/status_led.cpp
 */

namespace statusled {

void begin();
void update(bool wifiOk, bool comOk, bool appOk);

} // namespace statusled