#include "midi_input.h"

#include <Arduino.h>
#include <ESP32_Host_MIDI.h>
#include <USBConnection.h>

namespace midi {

	namespace {

		USBConnection usbHost;              // le clavier, branché sur le port OTG
		bool     active[128] = {false};     // notes MIDI actuellement maintenues
		int64_t  lastIndex   = 0;           // anti-doublon de la file d'événements

		void applyEvent(const MIDIEventData& ev, bool& changed) {
			if (ev.noteNumber > 127) return;

			if (ev.statusCode == MIDI_NOTE_ON && ev.velocity7 > 0) {
				if (!active[ev.noteNumber]) changed = true;
				active[ev.noteNumber] = true;
			} else if (ev.statusCode == MIDI_NOTE_OFF ||
					(ev.statusCode == MIDI_NOTE_ON && ev.velocity7 == 0)) {
				if (active[ev.noteNumber]) changed = true;
				active[ev.noteNumber] = false;
			}
		}

	} // namespace

	void setup() {
		midiHandler.addTransport(&usbHost);
		usbHost.begin();                 // démarre l'USB Host (core 0)
		midiHandler.begin();
		Serial.println("[MIDI] USB Host démarré — branchez le clavier…");
	}

	bool poll() {
		midiHandler.task();              // alimente la file d'événements

		bool changed = false;
		for (const auto& ev : midiHandler.getQueue()) {
			if (ev.index <= lastIndex) continue;
			lastIndex = ev.index;
			applyEvent(ev, changed);
		}
		return changed;
	}

	bool keyboardConnected() { return usbHost.isConnected(); }

	uint8_t activeNotes(uint8_t* out, uint8_t max) {
		uint8_t n = 0;
		for (int m = 0; m < 128 && n < max; m++)
			if (active[m]) out[n++] = (uint8_t)m;
		return n;
	}

} // namespace midi