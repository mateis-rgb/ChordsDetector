#pragma once
/*
 * midi_input.h — USB Host MIDI et suivi des notes actives (interface)
 *
 * Implémentation : lib/midi-input/src/midi_input.cpp
 */

#include <stdint.h>

namespace midi {

// Démarre l'USB Host (clavier branché sur le port OTG de l'ESP32-S3).
void setup();

// Traite les événements MIDI arrivés. À appeler dans loop().
// Renvoie true si l'ensemble des notes actives a changé.
bool poll();

// Le clavier MIDI est-il détecté sur le bus USB ?
bool keyboardConnected();

// Copie les notes MIDI actives (triées par ordre croissant) dans out[],
// renvoie le nombre de notes. max = capacité de out (ex. 32).
uint8_t activeNotes(uint8_t* out, uint8_t max);

} // namespace midi