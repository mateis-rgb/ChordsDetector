# status-led — LED RGB d'état

Pilote la WS2812 intégrée de l'ESP32-S3 comme reflet immédiat de l'état système (WiFi + clavier). Machine d'état **sans blocage**.

## 📄 Fichiers

| Fichier | Rôle |
|---|---|
| `src/status_led.h` | Interface publique (`namespace statusled`) |
| `src/status_led.cpp` | Couleurs d'état + clignotement non bloquant |

## 🔗 Dépendances

- Arduino core (`rgbLedWrite`, ex-`neopixelWrite`, déprécié dans le core 3.3.x) ;
- `include/config.h` :

| Constante | Rôle |
|---|---|
| `RGB_LED_PIN` | Broche de la WS2812 (48 la plupart des N16R8, 38 sur DevKitC-1 v1.1 et certains clones) |
| `LED_BRIGHTNESS` | Luminosité 0–255 (50 conseillé) |
| `LED_BLINK_PERIOD_MS` | Demi-période du clignotement (400 ms) |

## 🧩 API (`namespace statusled`)

| Signature | Description |
|---|---|
| `void begin()` | LED éteinte au démarrage. |
| `void update(bool wifiOk, bool comOk, bool appOk)` | À appeler dans `loop()` : applique la couleur d'état (bleu clignotant si une app est connectée). |

### Couleurs d'état

| État | Couleur | Effet |
|---|---|---|
| WiFi HS + clavier HS | 🔴 rouge | clignote |
| WiFi OK + clavier HS | 🟢 vert | clignote |
| WiFi OK + clavier OK | 🔵 bleu | **fixe** (état nominal) |
| WiFi OK + clavier OK + app connectée | 🔵 bleu | clignote |
| WiFi HS + clavier OK | 🟡 jaune | clignote |

## ⚙️ Implémentation

- `set(r, g, b)` — un simple `rgbLedWrite(RGB_LED_PIN, …)` ;
- `update()` : machine d'état sans blocage, clignotement par `millis()` modulo `LED_BLINK_PERIOD_MS` ; le bleu est fixe sans app connectée et **clignote quand une app est connectée** (3e paramètre `appOk`), les autres couleurs clignotent.

## 💡 Exemple

```cpp
#include "status_led.h"

statusled::begin();
// … dans loop() :
statusled::update(wifi::connected, midi::keyboardConnected());
```

## 🔌 Intégration

- Incluse directement par `main.cpp`, appelée à chaque `loop()` ;
- Aucun autre module ne pilote la LED ;
- LED jamais allumée → changer `RGB_LED_PIN` (48 puis 38) : voir le dépannage du README du projet.