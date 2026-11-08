# status-led — LED RGB d'état

Pilote la WS2812 intégrée de l'ESP32-S3 comme reflet immédiat de l'état système (WiFi + clavier). Machine d'état **sans blocage**.

## 📄 Fichiers

| Fichier | Rôle |
|---|---|
| `src/status_led.h` | Interface publique (`namespace statusled`) |
| `src/status_led.cpp` | Couleurs d'état + clignotement non bloquant |

## 🔗 Dépendances

- Arduino core (`neopixelWrite`) ;
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
| `void update(bool wifiOk, bool comOk)` | À appeler dans `loop()` : applique la couleur d'état correspondante. |

### Couleurs d'état

| État | Couleur | Effet |
|---|---|---|
| WiFi HS + clavier HS | 🔴 rouge | clignote |
| WiFi OK + clavier HS | 🟢 vert | clignote |
| WiFi OK + clavier OK | 🔵 bleu | **fixe** (état nominal) |
| WiFi HS + clavier OK | 🟡 jaune | clignote |

## ⚙️ Implémentation

- `set(r, g, b)` — un simple `neopixelWrite(RGB_LED_PIN, …)` ;
- `update()` : machine d'état sans blocage, clignotement par `millis()` modulo `LED_BLINK_PERIOD_MS` ; le bleu est fixe, les autres couleurs clignotent.

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