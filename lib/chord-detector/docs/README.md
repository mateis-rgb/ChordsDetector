# chord-detector — Moteur de détection d'accords

Reconnaît l'accord joué à partir des classes de hauteur actives et renvoie son nom en notation anglo-saxonne (`C`, `Cm7`, `Cmaj7/E`, `C4`…). Module **purement logique** : aucune dépendance matérielle ni Arduino, donc testable sur PC.

## 📄 Fichiers

| Fichier | Rôle |
|---|---|
| `src/chord_detector.h` | Interface publique (`namespace chords`) |
| `src/chord_detector.cpp` | 28 gabarits d'accords + algorithme de reconnaissance |

## 🔗 Dépendances

Aucune : C++ standard uniquement (`stdint.h`, `stddef.h`). Le module ne lit pas `config.h`.

## 🧩 API (`namespace chords`)

### `struct ChordInfo` — résultat de la détection

| Champ | Type | Description |
|---|---|---|
| `found` | `bool` | Un accord (ou une note) a-t-il été reconnu ? |
| `name` | `char[32]` | `"C"`, `"Cm7"`, `"Cmaj7/E"` (renversement), `"C4"` (note seule) |
| `rootPc` | `int` | Classe de hauteur de la fondamentale (0–11, `-1` si vide) |
| `bassPc` | `int` | Classe de hauteur de la note la plus grave (0–11) |

### Méthodes

| Signature | Description |
|---|---|
| `ChordInfo detect(uint16_t pcMask, int8_t lowestMidi, uint8_t activeCount)` | Détecte l'accord. `pcMask` : bit *n* = classe de hauteur *n* jouée ; `lowestMidi` : note MIDI la plus grave active (`-1` si aucune) ; `activeCount` : nombre de notes actives. |
| `void noteName(uint8_t midi, char* out, size_t len)` | Écrit le nom d'une note avec octave (`"C4"` pour MIDI 60). `out` ≥ 6 octets. |
| `NOTE_NAMES[12]` | `"C"`, `"C#"`, `"D"`… index = classe de hauteur (partagé avec les autres modules). |

## 🎼 Accords reconnus (28 gabarits)

Pour chaque fondamentale : `C Cm C7 Cm7 Cmaj7 CmM7 C6 Cm6 C6/9 C5 C9 Cm9 Cmaj9 C11 Cm11 Cmaj11 C13 Cm13 Cmaj13 Cadd9 C7-5 C7+5 Csus Cdim Cdim7 Cm7b5 Caug Caug7`

### Règles de détection

- **Quinte optionnelle** sur les accords à 4 notes et plus (habitude au clavier) ;
- **Tierce optionnelle** sur les 11e/13e (usage courant) ;
- **Toutes les notes jouées doivent être expliquées** par le gabarit, sinon `found = false` (accord non reconnu, les notes restent affichées) ;
- **Ambiguïtés tranchées par la basse** : `C-E-G-A` = `C6` si la basse est C, `Am7` si elle est A ;
- **Renversements** suffixés `C/E` ; **note seule** → `C4` ; **unisson** → `C`.

## ⚙️ Implémentation

- `struct Iv { uint8_t s; bool opt; }` — un intervalle depuis la fondamentale (demi-tons) + caractère optionnel ;
- `struct Tpl { const char* suffix; const Iv* iv; uint8_t n; }` — un gabarit d'accord ;
- Tables `IV_MAJ, IV_MIN, IV_5, IV_SUS, IV_DIM, IV_DIM7, IV_M7B5, IV_7, IV_M7, IV_MAJ7, IV_MM7, IV_7B5, IV_7S5, IV_AUG7, IV_6, IV_M6, IV_69, IV_9, IV_M9, IV_MAJ9, IV_ADD, IV_11, IV_M11, IV_MAJ11, IV_13, IV_M13, IV_MAJ13` + `TEMPLATES[28]` ;
- `matches()` : teste un couple (gabarit, fondamentale) — tous les intervalles requis présents, aucune note jouée non expliquée ; renvoie les compteurs pour le score ;
- `detect()` : accélérateurs note seule / unisson, sinon balayage 12 fondamentales × 28 gabarits ; score = `10×requis + optionnels présents + 1000 si la fondamentale est la basse` ;
- `noteName()` : octave = `midi / 12 - 1` (MIDI 60 → `C4`).

## 💡 Exemple

```cpp
#include "chord_detector.h"

uint16_t mask = (1 << 0) | (1 << 4) | (1 << 7);   // C, E, G
chords::ChordInfo c = chords::detect(mask, 60, 3);
// c.name == "C", c.rootPc == 0, c.bassPc == 0
```

## 🔌 Intégration

- Consommée par **web-server** : `buildStateJson()` calcule le masque de classes de hauteur puis appelle `chords::detect()`.
- Découverte LDF : `web_server.cpp` inclut `chord_detector.h` → la bibliothèque est liée automatiquement en mode `chain+`, sans `lib_deps`.