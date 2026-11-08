# wifi-manager — WiFi station, IP fixe CIDR et mDNS

Connexion WiFi en mode station avec IP fixe (notation CIDR), annonce mDNS et reconnexion automatique.

## 📄 Fichiers

| Fichier | Rôle |
|---|---|
| `src/wifi_manager.h` | Interface publique (`namespace wifi`) |
| `src/wifi_manager.cpp` | Parsing CIDR + connexion + mDNS + reconnexion |

## 🔗 Dépendances

- Arduino core : `WiFi` + `ESPmDNS` ;
- `include/config.h` :

| Constante | Rôle |
|---|---|
| `WIFI_SSID` / `WIFI_PASSWORD` | Identifiants du réseau (2,4 GHz uniquement) |
| `STATIC_IP_CIDR` | IP fixe en notation CIDR, ex. `"192.168.1.60/24"` (masque + passerelle déduits ; DHCP si invalide) |
| `GATEWAY_OVERRIDE` | Passerelle forcée, ex. `"192.168.1.254"` ; `""` = `x.x.x.1` du réseau |
| `HOSTNAME` | Nom mDNS → `http://<HOSTNAME>.local` |
| `WIFI_CONNECT_TIMEOUT_MS` | Attente maximale de connexion (15 000 ms) |

## 🧩 API (`namespace wifi`)

| Signature | Description |
|---|---|
| `extern bool connected` | État de connexion courant (mis à jour par `setup()` puis `maintain()`). |
| `void setup()` | Configure l'IP fixe depuis `STATIC_IP_CIDR` (repli DHCP), connecte au réseau, démarre mDNS si succès. **Bloquant** jusqu'à `WIFI_CONNECT_TIMEOUT_MS`. |
| `void maintain()` | Met à jour `connected` ; en cas de perte, relance la connexion **au plus toutes les 5 s**. À appeler dans `loop()`. |

## ⚙️ Implémentation

- `parseCidr(cidr, ip, mask, gw)` — découpe `"a.b.c.d/p"`, calcule le masque par décalage, la passerelle = première adresse du réseau (`|1` sur le dernier octet) ;
- `setup()` : `WiFi.config(ip, gw, mask, gw)` (DNS = passerelle ; `GATEWAY_OVERRIDE` prioritaire si non vide), attente bloquant ≤ timeout, puis `MDNS.begin(HOSTNAME)` + service `http/tcp` ;
- `maintain()` : lecture de `WiFi.status()`, `disconnect() + reconnect()` au plus toutes les 5 s.

## 💡 Exemple

```cpp
#include "wifi_manager.h"

wifi::setup();          // puis wifi::maintain() dans loop()
// une fois connecté : http://accords.local
```

## 🔌 Intégration

- Incluse directement par `main.cpp` ; `web::setup()` doit être appelé **après** `wifi::setup()` ;
- `connected` alimente la LED d'état (`status-led`) et le JSON d'état (`web-server`) ;
- Résolution mDNS : native sur Safari iOS/macOS ; certains Android nécessitent une app mDNS, sinon utiliser l'IP fixe.