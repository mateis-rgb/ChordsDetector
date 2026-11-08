#include "status_led.h"
#include "config.h"

#include <Arduino.h>

namespace statusled {

namespace {
// Écrit la couleur sur la WS2812 intégrée (broche définie dans config.h)
void set(uint8_t r, uint8_t g, uint8_t b) {
    neopixelWrite(RGB_LED_PIN, r, g, b);
}
} // namespace

void begin() { set(0, 0, 0); }

void update(bool wifiOk, bool comOk) {
    const bool blinkOn = (millis() / LED_BLINK_PERIOD_MS) % 2 == 0;

    if (!wifiOk && !comOk) {
        set(blinkOn ? LED_BRIGHTNESS : 0, 0, 0);                                   // rouge
    } else if (wifiOk && !comOk) {
        set(0, blinkOn ? LED_BRIGHTNESS : 0, 0);                                   // vert
    } else if (wifiOk && comOk) {
        set(0, 0, LED_BRIGHTNESS);                                                 // bleu fixe
    } else {
        set(blinkOn ? LED_BRIGHTNESS : 0, blinkOn ? LED_BRIGHTNESS / 2 : 0, 0);    // jaune
    }
}

} // namespace statusled