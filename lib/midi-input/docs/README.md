# midi-input — USB Host MIDI et suivi des notes actives

Transport USB Host MIDI (bibliothèque `ESP32_Host_MIDI`) et maintien de la liste des notes actives. **Seul module qui touche à l'USB.**

## 📄 Fichiers

| Fichier | Rôle |
|---|---|
| `src/midi_input.h` | Interface publique (`namespace midi`) |
| `src/midi_input.cpp` | Transport hôte + suivi des notes |

## 🔗 Dépendances

- `sauloverissimo/ESP32_Host_MIDI` (déclarée dans `lib_deps` du `platformio.ini`) ;
- Arduino core **3.x** (plateforme pioarduino) — l'USB Host ne fonctionne pas avec le core 2.x de la plateforme officielle.

## 🧩 API (`namespace midi`)

| Signature | Description |
|---|---|
| `void setup()` | Enregistre le transport USB Host (`midiHandler.addTransport(&usbHost)`) et démarre l'énumération. À appeler une fois dans `setup()`. |
| `bool poll()` | Traite les événements MIDI arrivés (`midiHandler.task()` + file). À appeler dans `loop()`. Retourne `true` si l'ensemble des notes actives a changé (→ déclenche un broadcast). |
| `bool keyboardConnected()` | Un clavier MIDI est-il détecté sur le bus USB (énuméré et actif) ? |
| `uint8_t activeNotes(uint8_t* out, uint8_t max)` | Copie les notes MIDI actives **triées croissant** dans `out[]`, retourne le nombre écrit (≤ `max`, ex. 32). |

## ⚙️ Implémentation

- Interne : l'instance `USBConnection usbHost` du transport hôte, le tableau `bool active[128]` des notes maintenues, et `int64_t lastIndex` (anti-doublon de la file, index signés 64 bits) ;
- `applyEvent(const MIDIEventData& ev, bool& changed)` — traduit un événement de la file : `NoteOn vel > 0` → note active ; `NoteOff` ou `NoteOn vel = 0` → note inactive ; clamp `noteNumber ≤ 127` ; statuts `MIDI_NOTE_ON` / `MIDI_NOTE_OFF` ;
- `poll()` : `midiHandler.task()` draine l'USB vers la file, puis parcours de `midiHandler.getQueue()` en ignorant les index déjà traités ;
- `activeNotes()` : balayage croissant 0–127 → sortie naturellement triée (`out[0]` = la plus grave = la basse).

## 🔌 Côté matériel

- Clavier branché sur le port **OTG (« USB »)** de l'ESP32-S3, alimentation par le port **COM** ;
- Un clavier autoalimenté (ex. Yamaha PSR-E363) doit quand même **voir 5 V sur VBUS** pour s'attacher au bus : pads OTG à ponter si nécessaire (voir le README du projet) ;
- Au branchement, le moniteur série doit afficher les lignes `MIDI DRIVER:`.

## 💡 Exemple

```cpp
#include "midi_input.h"

midi::setup();
// … dans loop() :
bool changed = midi::poll();
if (changed) web::broadcastState();
```

## 🔌 Intégration

- Incluse directement par `main.cpp` ;
- `poll()` qui retourne `true` déclenche `web::broadcastState()` immédiat ;
- `keyboardConnected()` alimente la LED d'état (`status-led`) et le JSON d'état (`web-server`).