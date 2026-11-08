# web-server — Serveur HTTP + WebSocket temps réel

Serveur ESPAsyncWebServer : sert la page web, construit l'état JSON à partir des autres modules et le pousse aux navigateurs connectés via WebSocket.

## 📄 Fichiers

| Fichier | Rôle |
|---|---|
| `src/web_server.h` | Interface publique (`namespace web`) |
| `src/web_server.cpp` | Routes, WebSocket, sérialisation JSON |

## 🔗 Dépendances

- `mathieucarbou/ESPAsyncWebServer @^3.0.0` + `mathieucarbou/AsyncTCP @^3.0.0` (dans `lib_deps`) ;
- Modules internes : **midi-input** (`midi::activeNotes()`), **chord-detector** (`chords::detect()`, `chords::noteName()`), **web-page** (`web_page::INDEX_HTML`) ;
- `include/config.h` : `BROADCAST_PERIOD_MS` (période du broadcast périodique, 2 000 ms).

## 🧩 API (`namespace web`)

| Signature | Description |
|---|---|
| `void setup()` | Déclare les routes (`/`, `/health`), le WebSocket `/ws` et démarre le serveur. À appeler après `wifi::setup()`. |
| `void broadcastState()` | Pousse l'état complet (accord, notes, WiFi, clavier) à tous les clients WebSocket. À appeler dès qu'une note change. |
| `void loop()` | Broadcast périodique (toutes les `BROADCAST_PERIOD_MS`) pour rafraîchir le statut + nettoyage des clients morts. À appeler dans `loop()`. |

## 🌐 Routes HTTP

| Route | Méthode | Réponse |
|---|---|---|
| `/` | GET | `web_page::INDEX_HTML` (`text/html`, PROGMEM) |
| `/health` | GET | `{"wifi": true, "com": true}` (`application/json`) |

## 🔌 Protocole WebSocket (`/ws`)

**ESP32 → navigateur** (à chaque changement de notes **et** toutes les `BROADCAST_PERIOD_MS`) :

```json
{
  "type": "state",
  "count": 4,
  "chord": "Cmaj7",
  "wifi": true,
  "com": true,
  "notes": [{"n": "C4", "m": 60}, {"n": "E4", "m": 64}, {"n": "G4", "m": 67}, {"n": "B4", "m": 71}]
}
```

- `chord` : nom de l'accord, `""` si rien ou non reconnu ;
- `notes[].n` : nom + octave, `notes[].m` : numéro MIDI.

**Navigateur → ESP32** (test de réactivité) :

```json
{"type": "ping", "id": 7}
```

**ESP32 → navigateur** (écho immédiat au client, pas de diffusion) :

```json
{"type": "pong", "id": 7}
```

## ⚙️ Implémentation

- Interne : `AsyncWebServer server(80)`, `AsyncWebSocket ws("/ws")`, `lastBroadcastMs` (périodicité) ;
- `buildStateJson()` : récupère les notes via `midi::activeNotes()`, calcule le masque de classes de hauteur et la basse, appelle `chords::detect()`, sérialise — **pas de ArduinoJson**, allocation unique via `String::reserve` ;
- `onWsEvent()` : à `WS_EVT_CONNECT` envoie l'état complet au nouveau client ; à `WS_EVT_DATA` répond au ping par un pong immédiat (écho direct au client).

## 🔌 Intégration

- Incluse directement par `main.cpp` ;
- C'est ce module qui inclut `chord_detector.h` et `web_page.h` → les bibliothèques `chord-detector` et `web-page` sont découvertes par le LDF `chain+` sans `lib_deps` ;
- `broadcastState()` est appelé immédiatement par `main.cpp` quand `midi::poll()` signale un changement de notes.