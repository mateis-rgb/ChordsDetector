#pragma once
/*
 * web_page.h — Page web servie sur "/" (interface)
 *
 * Page unique « Scène Néon » : thème auto clair/sombre, mode éco,
 * test de réactivité — 100 % CSS/JS embarqué, aucune requête externe.
 *
 * Implémentation (chaîne PROGMEM) : lib/web-page/src/web_page.cpp
 */

namespace web_page {

extern const char INDEX_HTML[];

} // namespace web_page