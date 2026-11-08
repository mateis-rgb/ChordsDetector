/*
 * chord_detector.cpp — Implémentation du moteur de détection d'accords
 *
 * Méthode : pour chacune des 12 fondamentales possibles, on essaie de faire
 * correspondre l'ensemble joué à des gabarits d'intervalles. Certains
 * intervalles sont optionnels (quinte omise sur les accords à 4 notes et
 * plus, tierce omise sur les 11e/13e). Le meilleur candidat est retenu
 * selon :
 *   1. la fondamentale est la note la plus grave (priorité forte -> évite
 *      l'ambiguïté C6 vs Am7) ;
 *   2. le nombre d'intervalles requis présents (spécificité) ;
 *   3. les intervalles optionnels effectivement joués.
 * Toutes les notes jouées doivent être expliquées par le gabarit,
 * sinon le candidat est rejeté (pas de détection partielle).
 */

#include "chord_detector.h"

#include <stdio.h>
#include <string.h>

namespace chords {

const char* const NOTE_NAMES[12] = {
    "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
};

// ------------------------------------------------------------------ internes
namespace {

// Intervalle depuis la fondamentale (demi-tons) ; opt = optionnel
struct Iv { uint8_t s; bool opt; };
// Gabarit d'accord : suffixe collé à la fondamentale + liste d'intervalles
struct Tpl { const char* suffix; const Iv* iv; uint8_t n; };

// Triades & quintes
constexpr Iv IV_MAJ[]  = {{0,false},{4,false},{7,false}};
constexpr Iv IV_MIN[]  = {{0,false},{3,false},{7,false}};
constexpr Iv IV_5[]    = {{0,false},{7,false}};
constexpr Iv IV_SUS[]  = {{0,false},{5,false},{7,false}};                 // sus4
constexpr Iv IV_DIM[]  = {{0,false},{3,false},{6,false}};
constexpr Iv IV_AUG[]  = {{0,false},{4,false},{8,false}};
constexpr Iv IV_DIM7[] = {{0,false},{3,false},{6,false},{9,false}};
constexpr Iv IV_M7B5[] = {{0,false},{3,false},{6,false},{10,false}};

// Septièmes — la quinte est optionnelle (souvent omise au clavier)
constexpr Iv IV_7[]    = {{0,false},{4,false},{10,false},{7,true}};
constexpr Iv IV_M7[]  = {{0,false},{3,false},{10,false},{7,true}};
constexpr Iv IV_MAJ7[] = {{0,false},{4,false},{11,false},{7,true}};
constexpr Iv IV_MM7[] = {{0,false},{3,false},{11,false},{7,true}};
constexpr Iv IV_7B5[]  = {{0,false},{4,false},{6,false},{10,false}};        // 7-5
constexpr Iv IV_7S5[]  = {{0,false},{4,false},{8,false},{10,false}};        // 7+5 (= aug7)
constexpr Iv IV_AUG7[] = {{0,false},{4,false},{8,false},{10,false}};

// Sixtes
constexpr Iv IV_6[]  = {{0,false},{4,false},{9,false},{7,true}};
constexpr Iv IV_M6[] = {{0,false},{3,false},{9,false},{7,true}};
constexpr Iv IV_69[] = {{0,false},{4,false},{9,false},{2,false},{7,true}}; // 6/9

// Neuvièmes
constexpr Iv IV_9[]    = {{0,false},{4,false},{10,false},{2,false},{7,true}};
constexpr Iv IV_M9[]   = {{0,false},{3,false},{10,false},{2,false},{7,true}};
constexpr Iv IV_MAJ9[] = {{0,false},{4,false},{11,false},{2,false},{7,true}};
constexpr Iv IV_ADD[]  = {{0,false},{4,false},{7,false},{2,false}};        // add9

// Onzièmes — la tierce est souvent omise
constexpr Iv IV_11[]    = {{0,false},{7,false},{10,false},{5,false},{2,true},{4,true}};
constexpr Iv IV_M11[]   = {{0,false},{3,false},{10,false},{5,false},{7,true},{2,true}};
constexpr Iv IV_MAJ11[] = {{0,false},{4,false},{11,false},{5,false},{7,true},{2,true}};

// Treizièmes
constexpr Iv IV_13[]    = {{0,false},{4,false},{10,false},{9,false},{2,true},{7,true}};
constexpr Iv IV_M13[]   = {{0,false},{3,false},{10,false},{9,false},{2,true},{7,true}};
constexpr Iv IV_MAJ13[] = {{0,false},{4,false},{11,false},{9,false},{2,true},{7,true}};

constexpr Tpl TEMPLATES[] = {
    {"",      IV_MAJ,   3}, {"m",     IV_MIN,   3}, {"5",     IV_5,     2},
    {"sus",   IV_SUS,   3}, {"dim",   IV_DIM,   3}, {"dim7",  IV_DIM7,  4},
    {"m7b5",  IV_M7B5,  4}, {"aug",   IV_AUG,   3}, {"aug7",  IV_AUG7,  4},
    {"maj7",  IV_MAJ7,  4}, {"mM7",   IV_MM7,   4}, {"6",     IV_6,     4},
    {"m6",    IV_M6,    4}, {"6/9",   IV_69,    5}, {"7",     IV_7,     4},
    {"m7",    IV_M7,    4}, {"7-5",   IV_7B5,   4}, {"7+5",   IV_7S5,   4},
    {"9",     IV_9,     5}, {"m9",    IV_M9,    5}, {"maj9",  IV_MAJ9,  5},
    {"add",   IV_ADD,   4}, {"11",    IV_11,    6}, {"m11",   IV_M11,   6},
    {"maj11", IV_MAJ11, 6}, {"13",    IV_13,    6}, {"m13",   IV_M13,   6},
    {"maj13", IV_MAJ13, 6},
};
constexpr uint8_t N_TEMPLATES = sizeof(TEMPLATES) / sizeof(TEMPLATES[0]);

// Le gabarit couvre-t-il toutes les notes jouées, et tous ses requis sont-ils présents ?
bool matches(const Tpl& tp, uint8_t root, uint16_t pcMask, int& required, int& presentOpt) {
    uint16_t covered = 0;
    required = 0;
    presentOpt = 0;
    for (uint8_t i = 0; i < tp.n; i++) {
        uint16_t bit = 1u << ((root + tp.iv[i].s) % 12);
        covered |= bit;
        bool played = pcMask & bit;
        if (!tp.iv[i].opt) {
            if (!played) return false;     // intervalle requis absent
            required++;
        } else if (played) {
            presentOpt++;
        }
    }
    return !(pcMask & ~covered);           // note non expliquée -> rejet
}

} // namespace

// ------------------------------------------------------------------ publics
void noteName(uint8_t midi, char* out, size_t len) {
    snprintf(out, len, "%s%d", NOTE_NAMES[midi % 12], midi / 12 - 1);
}

ChordInfo detect(uint16_t pcMask, int8_t lowestMidi, uint8_t activeCount) {
    ChordInfo r;
    r.bassPc = (lowestMidi >= 0) ? (lowestMidi % 12) : -1;

    if (pcMask == 0 || lowestMidi < 0) return r;          // rien de joué

    // --- cas particuliers : note seule ou unisson
    if (__builtin_popcount(pcMask) == 1) {
        r.found  = true;
        r.rootPc = lowestMidi % 12;
        if (activeCount == 1)
            snprintf(r.name, sizeof(r.name), "%s%d",
                     NOTE_NAMES[r.rootPc], lowestMidi / 12 - 1);
        else
            snprintf(r.name, sizeof(r.name), "%s", NOTE_NAMES[r.rootPc]);
        return r;
    }

    // --- essai de tous les couples (fondamentale, gabarit)
    int best = -1;
    for (uint8_t root = 0; root < 12; root++) {
        if (!(pcMask & (1u << root))) continue;          // la fondamentale doit être jouée
        for (uint8_t t = 0; t < N_TEMPLATES; t++) {
            int required, presentOpt;
            if (!matches(TEMPLATES[t], root, pcMask, required, presentOpt)) continue;

            int score = required * 10 + presentOpt + (root == r.bassPc ? 1000 : 0);
            if (score > best) {
                best     = score;
                r.found  = true;
                r.rootPc = root;
                snprintf(r.name, sizeof(r.name), "%s%s",
                         NOTE_NAMES[root], TEMPLATES[t].suffix);
            }
        }
    }

    // --- renversement : la basse n'est pas la fondamentale -> "C/E"
    if (r.found && r.bassPc >= 0 && r.bassPc != r.rootPc) {
        char base[24];
        strlcpy(base, r.name, sizeof(base));
        snprintf(r.name, sizeof(r.name), "%s/%s", base, NOTE_NAMES[r.bassPc]);
    }
    return r;
}

} // namespace chords