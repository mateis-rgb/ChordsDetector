# Détecteur d'accords — ESP32-S3 + clavier MIDI USB

Affiche en temps réel, sur une page web servie par l'ESP32 (téléphone ou PC), la note ou l'accord joué sur un clavier MIDI USB. LED RGB de statut sur la carte.

## 🧵 Câblage

| Depuis | Vers | Câble |
|---|---|---|
| Clavier (port USB-B "imprimante") | Port **USB** (native OTG) de l'ESP32 | USB-B ↔ USB-C **données** (adaptateur/câble OTG) |
| Chargeur USB ou batterie externe | Port **COM** (UART/programmation) de l'ESP32 | USB-C alimentation |

Le port USB-B du clavier est un port *périphérique* : il ne peut pas alimenter l'ESP32, seul un port *hôte* (USB-A rectangulaire) le pourrait. L'alimentation passe donc dans l'autre sens : **une seule source 5 V alimente l'ESP32 (port COM), et l'ESP32 alimente le clavier par le port OTG**. Un seul câble secteur pour l'ensemble — si ton clavier consomme moins de ~500 mA (vérifie son manuel), une batterie externe fonctionne aussi.

> ⚠️ **Si le clavier n'est pas détecté / pas alimenté** : sur certaines cartes clones, le port "USB" ne fournit pas le 5 V par défaut — il y a souvent des **pads OTG à souder/ponter** pour l'activer. Solution de repli : alimenter le clavier avec son propre câble secteur/USB (les données passent quand même par l'OTG).

## 💡 LED RGB (WS2812 intégrée)

| Couleur | Effet | Signification |
|---|---|---|
| 🔴 Rouge | clignote | WiFi HS **et** clavier HS |
| 🟢 Vert | clignote | WiFi OK, clavier HS |
| 🔵 Bleu | fixe | WiFi OK **et** clavier OK |
| 🟡 Jaune | clignote | WiFi HS, clavier OK (cas complémentaire) |

## 🔌 LED introuvable ?

La WS2812 est sur **GPIO48** sur la plupart des cartes N16R8 et DevKitC-1 v1.0, sur **GPIO38** sur les DevKitC-1 v1.1 et certains clones : change `RGB_LED_PIN` dans `config.h`.

## ⚙️ Configuration (`include/config.h`)

- `WIFI_SSID` / `WIFI_PASSWORD` : ton réseau WiFi
- `STATIC_IP_CIDR` : IP fixe en notation CIDR, ex. `192.168.1.60/24` (masque et passerelle déduits ; DHCP si invalide)
- `GATEWAY_OVERRIDE` : passerelle personnalisée si la vôtre n'est pas en `x.x.x.1`
- `HOSTNAME` : nom mDNS → `http://accords.local`

## 🚀 Compilation et flash

```bash
pio run -e detecteur -t upload     # compiler + flasher (port COM)
pio device monitor                # logs série (115200 bauds)
```

**Au premier flash** : maintenir BOOT si nécessaire, brancher via le port COM. Ensuite, redémarrer avec le clavier branché sur le port USB : la LED passe rouge → vert → bleu selon l'état.

> La plateforme utilisée est **pioarduino** (Arduino core 3.x) : l'USB Host MIDI ne fonctionne pas avec la plateforme PlatformIO officielle restée en core 2.x.

## 📱 Page web

Ouvrir `http://accords.local` (Safari iOS/macOS le résout nativement) ou l'IP fixe configurée.

- **Accord courant** en très grand, notes maintenues affichées en pastilles (ex. `C4 E4 G4`)
- **Renversements** gérés : `C/E` ; note seule affichée avec son octave : `C4`
- **Bouton "Test de réactivité"** : 20 pings WebSocket, RTT min/moy/max + aller simple estimé (typiquement 1–15 ms en WiFi local)
- **Design « Scène Néon »** : accord massif à double halo, suffixe coloré, mini-équaliseur animé quand des notes jouent, indicateurs façon LED
- **Thème automatique** : suit le thème clair/sombre du système de l'appareil (`prefers-color-scheme`)
- **Bouton "Mode éco"** : coupe halos néon, grille et animations (économie batterie/CPU). Design 100 % CSS embarqué, aucune requête externe : la page fonctionne même sans Internet.

## 🎼 Accords reconnus

Pour chaque fondamentale : `C Cm C7 Cm7 Cmaj7 CmM7 C6 Cm6 C6/9 C5 C9 Cm9 Cmaj9 C11 Cm11 Cmaj11 C13 Cm13 Cmaj13 Cadd9 C7-5 C7+5 Csus Cdim Cdim7 Cm7b5 Caug Caug7`

Détails d'implémentation (`chord_detector.h`) :

- la **quinte peut être omise** sur les accords à 4 notes et plus (habitude au clavier)
- la **tierce peut être omise** sur les 11e/13e (usage courant)
- ambiguïtés résolues par la **basse** : `C-E-G-A` = `C6` si la basse est C, `Am7` si elle est A
- toutes les notes jouées doivent être expliquées par le gabarit, sinon → "Accord non reconnu" (les notes restent affichées)

## 🔍 Dépannage

| Symptôme | Piste |
|---|---|
| LED ne s'allume jamais | `RGB_LED_PIN` : essayer 48 puis 38 |
| LED rouge persistante | WiFi : vérifier SSID/mot de passe, bande 2,4 GHz uniquement |
| LED verte persistante | Clavier : câble OTG **données** (pas charge seule), pads OTG, alimenter le clavier séparément |
| Clavier autoalimenté (piles/adaptateur, ex. Yamaha PSR-E363) qui n'est jamais détecté | Il ne consomme rien mais **doit voir 5 V sur VBUS** pour s'attacher au bus : vérifier/ponter les pads OTG, mesurer VBUS au multimètre. Au branchement, le moniteur série doit afficher les lignes `MIDI DRIVER:` |
| Clavier Yamaha détecté mais pas de notes | PSR-E = appareil composite (MIDI + audio), descripteur plus complexe : récupérer le log complet du moniteur série |
| `accords.local` ne résout pas | Utiliser l'IP fixe ; certains Android nécessitent une app mDNS |
| Page sobre alors que mode complet | Pas d'accès Internet → Tailwind CDN injoignable → basculer mode éco ou ouvrir le port |
| `IncompatiblePlatform: … Core v6.2.0-post.1 depends on Core >=6.2.0` | Bug connu pioarduino (core pré-version refusé par son propre contrôle). Fix : `~/.platformio/penv/bin/pip install "pioarduino==6.2.0"` puis relancer le build |
| `tool-esptoolpy does not appear to be a Python project` | Avertissement bénin de l'extension VS Code avec esptool 5.x (paquet précompilé). Si l'upload échoue vraiment : `~/.platformio/penv/bin/pip install "esptool==5.*"` |
| Latence élevée | WiFi chargé ; le lien USB lui-même est < 1 ms |

## 🗂️ Structure du projet

Chaque module est une **bibliothèque privée PlatformIO** dans `lib/<nom>/src/` : interfaces et implémentations vivent ensemble, le Library Dependency Finder les découvre et les lie automatiquement (`lib_ldf_mode = chain+` dans `platformio.ini`).

```
detecteur-accords/
├── platformio.ini                 # plateforme pioarduino + dépendances + LDF
├── include/
│   └── config.h                   # constantes : WiFi, IP CIDR, LED — À ÉDITER
├── lib/                           # bibliothèques privées (préco PlatformIO)
│   ├── API.md                     # 📖 interfaces + implémentations combinées
│   ├── chord-detector/            #   src/chord_detector.{h,cpp} + 📖 docs/README.md
│   ├── midi-input/                #   src/midi_input.{h,cpp} + 📖 docs/README.md
│   ├── status-led/                #   src/status_led.{h,cpp} + 📖 docs/README.md
│   ├── web-page/                  #   src/web_page.{h,cpp} + 📖 docs/README.md
│   ├── web-server/                #   src/web_server.{h,cpp} + 📖 docs/README.md
│   └── wifi-manager/              #   src/wifi_manager.{h,cpp} + 📖 docs/README.md
└── src/
    └── main.cpp                   # orchestration (setup + loop)
```

Fait avec ❤️ en France 🇫🇷 avec Mistral AI