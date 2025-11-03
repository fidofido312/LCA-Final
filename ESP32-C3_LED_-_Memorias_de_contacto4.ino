/*
   BOTONES + LEDS en paralelo con efecto FADE independiente + MEMORIA DE PATRONES
   Board: ESP32 / ESP32-C3
   Botones: GPIO 0,1,2,3  (a GND con INPUT_PULLUP, activo en LOW)
   LEDs:    GPIO 8,7,6,5  (PWM via analogWrite)

   Funciones clave:
   - Grabación de eventos por botón: PRESS/RELEASE con dt (delta tiempo) y duración.
   - Inactividad 20 s -> PLAYBACK no bloqueante del patrón.
   - En PLAYBACK, el HOLD del LED se ajusta a la duración grabada del contacto.
*/

#include <Arduino.h>

// ------------ Pines y parámetros básicos ------------
const uint8_t buttonPins[] = {0, 1, 2, 3};
const uint8_t ledPins[]    = {8, 7, 6, 5};
const int numLeds = 4;

// Efecto LED
const int maxBrightness = 255;
const int fadeDelayMs   = 5;     // tiempo entre pasos de brillo
const int defaultHoldMs = 500;   // HOLD por defecto cuando no hay duración

// Reproducción / grabación
const unsigned long inactivityToPlaybackMs = 20000; // 20 s sin pulsar -> PLAYBACK
const unsigned long loopPauseMs            = 800;   // pausa al finalizar patrón

// Ticks (cuantización temporal)
const uint16_t TICK_MS = 5; // 1 tick = 5 ms

// Mapeo de duración grabada -> hold efectivo
const int MIN_HOLD_MS = 120;     // límites para que el efecto se vea bien
const int MAX_HOLD_MS = 2000;

// Anti-rebote
const unsigned long DEBOUNCE_MS = 20;

// Máximo de eventos guardados en RAM
const int MAX_EVENTS = 600;

// ------------ Estados LED ------------
struct LedState {
  enum Phase { OFF, FADE_IN, HOLD, FADE_OUT } phase;
  int brightness;
  unsigned long lastUpdate;
  unsigned long holdStart;
  int customHoldMs; // si >0, usar esto para HOLD; si 0 -> defaultHoldMs
};

LedState leds[numLeds];

// ------------ Botones (debounce + captura flancos) ------------
struct ButtonState {
  bool stableDown = false;     // estado estable actual (LOW = pulsado)
  bool readingDown = false;    // última lectura inmediata
  unsigned long lastChangeMs = 0; // para debounce
  unsigned long pressStartMs = 0; // para medir duración real
};

ButtonState buttons[numLeds];

// ------------ Secuenciador de eventos ------------
enum EventType : uint8_t { PRESS = 0, RELEASE = 1 };

using tick_t = uint16_t; // 0..65535 (con TICK_MS=5 llega a ~327 s)

struct Event {
  uint8_t  track;   // 0..3
  uint8_t  type;    // PRESS/RELEASE
  tick_t   dt;      // ticks desde el evento anterior
  uint16_t extra;   // en RELEASE: duración del contacto (ticks). En PRESS: 0
};

Event eventsBuf[MAX_EVENTS];
uint16_t eventCount = 0;
tick_t lastEventTick = 0;      // para calcular dt
unsigned long lastUserActivity = 0;

// Modo global
enum Mode { LIVE, PLAYBACK };
Mode mode = LIVE;

// Reproducción
uint16_t playbackIndex = 0;
bool playbackArmed = false;
unsigned long nextPlaybackTime = 0;

// Utilidades
static inline tick_t msToTicks(uint32_t ms) { return (tick_t)(ms / TICK_MS); }
static inline uint32_t ticksToMs(tick_t t)  { return (uint32_t)t * TICK_MS; }

void pushEvent(Event e) {
  if (eventCount < MAX_EVENTS) {
    eventsBuf[eventCount++] = e;
  } else {
    // Buffer lleno: podría implementarse rollover o dividir en clips.
    // Por ahora, ignoramos eventos extra.
  }
}

void clearPattern() {
  eventCount = 0;
  lastEventTick = msToTicks(millis());
}

// ------------ Lógica LED ------------
void triggerLed(int i, unsigned long now) {
  LedState &L = leds[i];
  if (L.phase == LedState::OFF) {
    L.phase = LedState::FADE_IN;
    L.brightness = 0;
    L.lastUpdate = now;
    L.customHoldMs = 0; // hasta que alguien lo configure
    analogWrite(ledPins[i], 0);
  }
}

void setLedCustomHold(int i, int holdMs) {
  holdMs = constrain(holdMs, MIN_HOLD_MS, MAX_HOLD_MS);
  leds[i].customHoldMs = holdMs;
}

void updateLed(int i) {
  unsigned long now = millis();
  LedState &L = leds[i];

  switch (L.phase) {
    case LedState::OFF:
      // En este diseño las pulsaciones reales se manejan en updateButtons()
      break;

    case LedState::FADE_IN:
      if (now - L.lastUpdate >= (unsigned)fadeDelayMs) {
        L.brightness += 5;
        if (L.brightness >= maxBrightness) {
          L.brightness = maxBrightness;
          L.phase = LedState::HOLD;
          L.holdStart = now;
        }
        L.lastUpdate = now;
        analogWrite(ledPins[i], L.brightness);
      }
      break;

    case LedState::HOLD: {
      analogWrite(ledPins[i], maxBrightness);
      int holdMs = (L.customHoldMs > 0) ? L.customHoldMs : defaultHoldMs;
      if (now - L.holdStart >= (unsigned)holdMs) {
        L.phase = LedState::FADE_OUT;
        L.lastUpdate = now;
      }
    } break;

    case LedState::FADE_OUT:
      if (now - L.lastUpdate >= (unsigned)fadeDelayMs) {
        L.brightness -= 5;
        if (L.brightness <= 0) {
          L.brightness = 0;
          L.phase = LedState::OFF;
        }
        L.lastUpdate = now;
        analogWrite(ledPins[i], L.brightness);
      }
      break;
  }
}

// ------------ Captura de botones (debounce + eventos) ------------
void onPress(uint8_t track, unsigned long nowMs) {
  // Evento PRESS
  tick_t nowTick = msToTicks(nowMs);
  tick_t dt = nowTick - lastEventTick;
  lastEventTick = nowTick;
  pushEvent({track, PRESS, dt, 0});

  // Dispara LED y marca actividad
  triggerLed(track, nowMs);
  lastUserActivity = nowMs;
}

void onRelease(uint8_t track, unsigned long nowMs) {
  // Duración del contacto desde el press estable
  unsigned long durMs = (nowMs > buttons[track].pressStartMs)
                        ? (nowMs - buttons[track].pressStartMs)
                        : 0;

  // Evento RELEASE con dt y duración en ticks (extra)
  tick_t nowTick = msToTicks(nowMs);
  tick_t dt = nowTick - lastEventTick;
  lastEventTick = nowTick;
  tick_t durTicks = msToTicks(durMs);
  pushEvent({track, RELEASE, dt, (uint16_t)durTicks});

  lastUserActivity = nowMs;
}

// Lee botones con debounce y emite flancos
void updateButtons() {
  unsigned long now = millis();
  for (int i = 0; i < numLeds; i++) {
    bool rawDown = (digitalRead(buttonPins[i]) == LOW); // activo en LOW

    if (rawDown != buttons[i].readingDown) {
      // cambio inmediato: reinicia temporizador de debounce
      buttons[i].readingDown = rawDown;
      buttons[i].lastChangeMs = now;
    }

    // ¿Cambió establemente?
    if (rawDown != buttons[i].stableDown &&
        (now - buttons[i].lastChangeMs) >= DEBOUNCE_MS) {

      buttons[i].stableDown = rawDown; // confirmamos el nuevo estado

      if (mode == PLAYBACK) {
        // Cualquier actividad real cancela el modo PLAYBACK
        mode = LIVE;
        playbackArmed = false;
        nextPlaybackTime = 0;
        // No retornamos: dejamos seguir para registrar si se desea
      }

      if (buttons[i].stableDown) {
        // Flanco de bajada (PRESS)
        buttons[i].pressStartMs = now;
        onPress(i, now);
      } else {
        // Flanco de subida (RELEASE)
        onRelease(i, now);
      }
    }
  }
}

// ------------ Reproducción del patrón (secuenciador) ------------
/*
   Al ejecutar un PRESS en reproducción, buscamos hacia adelante el próximo RELEASE
   del mismo track para leer su duración y la usamos como HOLD (con límites).
*/
int findNextReleaseDurationMs(uint16_t fromIdx, uint8_t track) {
  for (uint16_t k = fromIdx; k < eventCount; ++k) {
    const Event &e = eventsBuf[k];
    if (e.track == track && e.type == RELEASE) {
      return (int)ticksToMs(e.extra);
    }
  }
  // si no hay RELEASE futuro, usar default
  return defaultHoldMs;
}

void updatePlayback() {
  if (eventCount == 0) return;

  unsigned long now = millis();

  if (!playbackArmed) {
    // Programamos el próximo evento según su dt (más pausa de loop si volvemos a inicio)
    tick_t dtTicks = eventsBuf[playbackIndex].dt;
    unsigned long dtMs = ticksToMs(dtTicks);

    if (playbackIndex == 0 && nextPlaybackTime != 0) {
      dtMs += loopPauseMs;
    }
    nextPlaybackTime = now + dtMs;
    playbackArmed = true;
  }

  if (playbackArmed && (long)(now - nextPlaybackTime) >= 0) {
    const Event &e = eventsBuf[playbackIndex];

    if (e.type == PRESS) {
      // Disparar LED
      triggerLed(e.track, now);
      // Intentar fijar un HOLD basado en la próxima duración registrada
      int durMs = findNextReleaseDurationMs(playbackIndex + 1, e.track);
      setLedCustomHold(e.track, durMs);
    } else if (e.type == RELEASE) {
      // Nada obligatorio: ya fijamos el HOLD al PRESS.
      // Si quisieras cambiar el comportamiento, podrías usar e.extra aquí.
    }

    // Avanzar
    playbackIndex++;
    if (playbackIndex >= eventCount) {
      playbackIndex = 0; // repetir patrón
    }
    playbackArmed = false;
  }
}

// ------------ Setup / Loop ------------
void setup() {
  Serial.begin(115200);

  for (int i = 0; i < numLeds; i++) {
    pinMode(buttonPins[i], INPUT_PULLUP);
    pinMode(ledPins[i], OUTPUT);
    analogWrite(ledPins[i], 0);

    leds[i] = {LedState::OFF, 0, millis(), 0, 0};

    // Estado inicial botones
    buttons[i].stableDown  = (digitalRead(buttonPins[i]) == LOW);
    buttons[i].readingDown = buttons[i].stableDown;
    buttons[i].lastChangeMs = millis();
    buttons[i].pressStartMs = millis();
  }

  clearPattern();
  lastUserActivity = millis();

  Serial.println("READY: LIVE mode, recording events.");
}

void loop() {
  unsigned long now = millis();

  // 1) Lectura botones + flancos (y posible cambio de modo a LIVE)
  updateButtons();

  // 2) Actualización de LEDs
  for (int i = 0; i < numLeds; i++) {
    updateLed(i);
  }

  // 3) Paso a PLAYBACK por inactividad (si hay patrón)
  if (mode == LIVE) {
    if ((now - lastUserActivity >= inactivityToPlaybackMs) && (eventCount > 0)) {
      mode = PLAYBACK;
      playbackIndex = 0;
      playbackArmed = false;
      nextPlaybackTime = 0;
      Serial.println("LIVE -> PLAYBACK (inactividad)");
    }
  } else { // PLAYBACK
    updatePlayback();
  }

  // (Opcional) pequeño respiro
  // delay(1);
}
