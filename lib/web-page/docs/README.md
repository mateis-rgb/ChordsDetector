# web-page — Page web embarquée « Scène Néon »

La page web complète servie sur `/`, stockée en flash (PROGMEM) : une seule chaîne C++ brute, **100 % CSS/JS embarqué, aucune requête externe** — fonctionne même sans Internet.

## 📄 Fichiers

| Fichier | Rôle |
|---|---|
| `src/web_page.h` | Interface publique (`namespace web_page`) |
| `src/web_page.cpp` | `INDEX_HTML[] PROGMEM` — la page entière (HTML + CSS + JS) |

## 🔗 Dépendances

Aucune côté ESP32 : la page vit en flash. Servie par **web-server** (`web::setup()`).

## 🧩 API (`namespace web_page`)

| Symbole | Description |
|---|---|
| `extern const char INDEX_HTML[]` | La page web complète « Scène Néon », stockée en PROGMEM et servie sur `/`. |

## 🖥️ Fonctionnalités côté navigateur

- **Rendu** : accord courant en très grand avec décomposition fondamentale + suffixe néon coloré + basse grise (renversements `C/E`) ; chips de notes re-créées seulement si l'ensemble change ; mini-équaliseur animé quand des notes jouent ; indicateurs façon LED ;
- **WebSocket** : reconnexion automatique avec backoff exponentiel (250 ms → 5 s max) ;
- **Test de réactivité** : bouton qui envoie 20 pings espacés de 70 ms, affiche RTT min/moy/max + aller simple estimé (typiquement 1–15 ms en WiFi local) ;
- **Mode éco** : classe `body.eco` persistée en `localStorage` (clé `accords-mode`), coupe halos néon, grille et animations (économie batterie/CPU) ;
- **Thème automatique** : suit le thème clair/sombre du système (`prefers-color-scheme`, badge indicatif).

## ⚙️ Implémentation

- `web_page.cpp` est une unique chaîne brute C++ (`INDEX_HTML[] PROGMEM`) : pour modifier le design ou le comportement, éditer cette chaîne ;
- La page consomme le protocole WebSocket documenté dans le README de **web-server** (messages `state` / `ping` / `pong`).

## 🔌 Intégration

- Incluse par `web_server.cpp` → bibliothèque découverte par le LDF `chain+` sans `lib_deps` ;
- Ne référence aucun autre module côté ESP32 : tout le comportement réside dans le JS embarqué.