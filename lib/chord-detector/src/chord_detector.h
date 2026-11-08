#pragma once
/*
 * chord_detector.h — Moteur de détection d'accords (interface)
 *
 * Reçoit les classes de hauteur jouées et renvoie le nom de l'accord
 * (notation anglo-saxonne : C, Cm, C7, Cmaj7, C6/9, C11, C13, C7-5, C7+5,
 * Csus, Cdim, Cdim7, Cm7b5, Caug, Caug7, …), gère les renversements
 * ("C/E"), la note seule ("C4") et l'unisson ("C").
 *
 * Implémentation : lib/chord-detector/src/chord_detector.cpp
 */

#include <stddef.h>
#include <stdint.h>

namespace chords {

// Noms des notes, index = classe de hauteur (0 = C)
extern const char* const NOTE_NAMES[12];

// Résultat de la détection
struct ChordInfo {
    bool found    = false;
    char name[32] = {0};   // "C", "Cm7", "Cmaj7/E", "C4" (note seule)…
    int  rootPc   = -1;    // classe de hauteur de la fondamentale
    int  bassPc   = -1;    // classe de hauteur de la note la plus grave
};

// pcMask      : bit n = classe de hauteur n jouée
// lowestMidi  : note MIDI la plus grave active (-1 si aucune)
// activeCount : nombre de notes actives
ChordInfo detect(uint16_t pcMask, int8_t lowestMidi, uint8_t activeCount);

// Nom d'une note avec son octave ("C4" pour MIDI 60). out >= 6 octets.
void noteName(uint8_t midi, char* out, size_t len);

} // namespace chords