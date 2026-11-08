#pragma once
/*
 * status_led.h — LED RGB d'état (interface)
 *
 *  ROUGE  (clignote) : WiFi HS + clavier HS
 *  VERT   (clignote) : WiFi OK + clavier HS
 *  BLEU   (fixe)     : WiFi OK + clavier OK
 *  JAUNE  (clignote) : WiFi HS + clavier OK (cas complémentaire)
 *
 * Implémentation : lib/status-led/src/status_led.cpp
 */

namespace statusled {

void begin();
void update(bool wifiOk, bool comOk);

} // namespace statusled