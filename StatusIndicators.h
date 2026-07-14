#ifndef STATUS_INDICATORS_H
#define STATUS_INDICATORS_H

#include <Arduino.h>
#include "Config.h"

enum SkunkSystemState {
    STATE_BOOTING = 0,
    STATE_OK = 1,
    STATE_PRINTING = 2,
    STATE_PORTAL_AP = 3,
    STATE_PAPER_OUT = 4,
    STATE_USB_DISCONNECTED = 5
};

class StatusIndicators {
private:
    static SkunkSystemState currentState;
    static unsigned long lastBlinkMillis;
    static bool blinkState;

public:
    static void init() {
        if (RGB_LED_PIN >= 0) {
            pinMode(RGB_LED_PIN, OUTPUT);
            // neopixelWrite es la función nativa en Arduino ESP32 Core v3.x para WS2812 en cualquier GPIO
#if defined(neopixelWrite)
            neopixelWrite(RGB_LED_PIN, 0, 0, 0);
#endif
        }
        if (BUZZER_PIN >= 0) {
            pinMode(BUZZER_PIN, OUTPUT);
            digitalWrite(BUZZER_PIN, LOW);
        }
        currentState = STATE_BOOTING;
        lastBlinkMillis = 0;
        blinkState = false;
    }

    static void setState(SkunkSystemState state) {
        if (currentState != state) {
            currentState = state;
            updateImmediate();
        }
    }

    static SkunkSystemState getState() {
        return currentState;
    }

    static void updateImmediate() {
        if (RGB_LED_PIN < 0) return;

#if defined(neopixelWrite)
        switch (currentState) {
            case STATE_BOOTING:
                neopixelWrite(RGB_LED_PIN, 30, 30, 30); // Blanco tenue
                break;
            case STATE_OK:
                neopixelWrite(RGB_LED_PIN, 0, 40, 0);   // Verde fijo
                break;
            case STATE_PRINTING:
                neopixelWrite(RGB_LED_PIN, 0, 0, 80);   // Azul brillante
                break;
            case STATE_PORTAL_AP:
                neopixelWrite(RGB_LED_PIN, 50, 35, 0);  // Amarillo portal cautivo
                break;
            case STATE_PAPER_OUT:
                neopixelWrite(RGB_LED_PIN, 80, 0, 0);   // Rojo alerta (Falta papel)
                if (BUZZER_PIN >= 0) beep(100);
                break;
            case STATE_USB_DISCONNECTED:
                neopixelWrite(RGB_LED_PIN, 60, 0, 60);  // Morado/Magenta (Desconectada)
                break;
        }
#endif
    }

    static void beep(unsigned long durationMs) {
        if (BUZZER_PIN >= 0) {
            digitalWrite(BUZZER_PIN, HIGH);
            delay(durationMs);
            digitalWrite(BUZZER_PIN, LOW);
        }
    }

    // Método que se llama en el bucle principal o en tarea para parpadear el LED en estados de alerta
    static void loop() {
        if (currentState == STATE_PAPER_OUT || currentState == STATE_USB_DISCONNECTED || currentState == STATE_PORTAL_AP) {
            if (millis() - lastBlinkMillis > 400) {
                lastBlinkMillis = millis();
                blinkState = !blinkState;

#if defined(neopixelWrite)
                if (blinkState) {
                    updateImmediate();
                } else {
                    neopixelWrite(RGB_LED_PIN, 0, 0, 0); // Apagar momentáneamente
                }
#endif
            }
        }
    }
};

SkunkSystemState StatusIndicators::currentState = STATE_BOOTING;
unsigned long StatusIndicators::lastBlinkMillis = 0;
bool StatusIndicators::blinkState = false;

#endif // STATUS_INDICATORS_H
