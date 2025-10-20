// --- Configuración de hardware (ESP32) ---
#define PIN_TACTIL         T0        // Pin táctil
const int PIN_LED          = 22;     // Pin del LED (PWM)
const int CANAL_PWM        = 0;      // Canal PWM para ledc
const int FREQ_PWM         = 5000;   // Frecuencia PWM (Hz)
const int RES_PWM_BITS     = 8;      // Resolución PWM (0..255)

// --- Umbrales y tiempos ---
const int UMBRAL_TACTIL_ON  = 30;    // Menor -> tocado
const int UMBRAL_TACTIL_OFF = 35;    // Histeresis para soltar
const unsigned long RAMP_INTERVAL_MS      = 15;   // Cada cuánto ajusto brillo
const uint8_t       RAMP_STEP             = 5;    // Paso de brillo por ajuste
const unsigned long SAMPLE_INTERVAL_MS    = 50;   // Muestra para grabación/replay
const unsigned long TIEMPO_INACTIVIDAD_MS = 60000; // 1 minuto de apagado para iniciar replay

// --- Archivo de patrones en RAM ---
const int MAX_PATRONES = 8;      // Cantidad máxima de toques guardados
const int MAX_STEPS    = 800;    // Muestras por patrón (50 ms * 800 ≈ 40 s)
uint8_t patrones[MAX_PATRONES][MAX_STEPS];
uint16_t largoPatron[MAX_PATRONES];
int cantidadPatrones = 0;

// --- Estado de brillo (0..255) ---
uint8_t brilloActual   = 0;
uint8_t brilloObjetivo = 0;

// --- Estado de contacto y tiempos ---
bool    tocando                 = false;
bool    tocandoPrevio           = false;
unsigned long tUltimoRamp       = 0;
unsigned long tUltimaMuestra    = 0;
unsigned long tDesdeApagado     = 0;  // Marca desde cuándo está en 0 (apagado estable)

// --- Grabación ---
bool     grabando        = false;
int      idxPatronActual = -1;
uint16_t pasosGrabados   = 0;

// --- Reproducción ---
bool     reproduciendo   = false;
int      idxPatronReplay = 0;    // cuál patrón (0..cantidadPatrones-1)
uint16_t pasoReplay      = 0;    // índice dentro del patrón
bool     faseFadeOut     = false; // tras cada patrón, hacer un fade out a 0
uint8_t  brilloReplay    = 0;

// ------------------ Utilidades ------------------

void aplicarBrillo(uint8_t b) {
  ledcWrite(CANAL_PWM, b);
  brilloActual = b;
}

void setBrilloObjetivo(uint8_t b) {
  brilloObjetivo = b;
}

// Rampa no bloqueante hacia brilloObjetivo
void actualizarRampa() {
  unsigned long ahora = millis();
  if (ahora - tUltimoRamp < RAMP_INTERVAL_MS) return;
  tUltimoRamp = ahora;

  if (brilloActual < brilloObjetivo) {
    uint16_t next = brilloActual + RAMP_STEP;
    aplicarBrillo(next > brilloObjetivo ? brilloObjetivo : next);
  } else if (brilloActual > brilloObjetivo) {
    int next = (brilloActual > RAMP_STEP) ? (brilloActual - RAMP_STEP) : 0;
    aplicarBrillo(next < brilloObjetivo ? brilloObjetivo : next);
  }

  // Mantener marca de "apagado estable"
  if (!tocando && !reproduciendo && brilloActual == 0) {
    // Si recién llegó a 0, inicia contador
    if (tDesdeApagado == 0) tDesdeApagado = ahora;
  } else {
    tDesdeApagado = 0; // no está apagado estable
  }
}

// Agregar una muestra al patrón actual
void grabarMuestra(uint8_t b) {
  if (!grabando || idxPatronActual < 0) return;
  if (pasosGrabados < MAX_STEPS) {
    patrones[idxPatronActual][pasosGrabados++] = b;
  }
}

// Iniciar grabación en un nuevo patrón
void iniciarGrabacion() {
  if (cantidadPatrones < MAX_PATRONES) {
    idxPatronActual = cantidadPatrones;
  } else {
    // Si llegamos al límite, sobreescribimos el más antiguo (rotación)
    // Desplazar todos hacia arriba y usar el último índice
    for (int i = 1; i < MAX_PATRONES; ++i) {
      memcpy(patrones[i - 1], patrones[i], MAX_STEPS);
      largoPatron[i - 1] = largoPatron[i];
    }
    idxPatronActual = MAX_PATRONES - 1;
    cantidadPatrones = MAX_PATRONES - 1; // se recontará más abajo
  }

  pasosGrabados = 0;
  grabando = true;
}

// Finalizar grabación y guardar longitud
void finalizarGrabacion() {
  if (!grabando) return;
  grabando = false;
  largoPatron[idxPatronActual] = pasosGrabados;

  // Solo contar patrones que tengan al menos 1 muestra
  if (pasosGrabados > 0) {
    // Si era un overwrite circular, ya ajustamos arriba
    if (idxPatronActual == cantidadPatrones) {
      cantidadPatrones++;
    }
  }
  idxPatronActual = -1;
  pasosGrabados = 0;
}

// Inicia modo de reproducción autónoma
void iniciarReproduccion() {
  if (cantidadPatrones == 0) return;
  reproduciendo    = true;
  idxPatronReplay  = 0;
  pasoReplay       = 0;
  faseFadeOut      = false;
  brilloReplay     = 0;
  tUltimaMuestra   = millis();
}

// Detiene reproducción
void detenerReproduccion() {
  reproduciendo = false;
  faseFadeOut   = false;
  // Al detener, volvemos a control manual; el brillo objetivo depende de toque
}

// Avanza la reproducción no bloqueante
void actualizarReproduccion() {
  if (!reproduciendo || cantidadPatrones == 0) return;

  unsigned long ahora = millis();
  if (ahora - tUltimaMuestra < SAMPLE_INTERVAL_MS) return;
  tUltimaMuestra = ahora;

  if (!faseFadeOut) {
    // Reproducir la forma grabada
    uint16_t L = largoPatron[idxPatronReplay];
    if (L == 0) {
      // Si está vacío por alguna razón, saltamos al siguiente
      faseFadeOut = true;
      brilloReplay = brilloActual;
    } else {
      // Seteo directo de brillo según la muestra grabada
      brilloReplay = patrones[idxPatronReplay][pasoReplay];
      aplicarBrillo(brilloReplay);
      pasoReplay++;
      if (pasoReplay >= L) {
        // Terminó el patrón: hacemos un fade out suave hacia 0
        faseFadeOut = true;
      }
    }
  } else {
    // Fase de apagado gradual tras reproducir el patrón
    if (brilloReplay > 0) {
      brilloReplay = (brilloReplay > RAMP_STEP) ? (brilloReplay - RAMP_STEP) : 0;
      aplicarBrillo(brilloReplay);
    } else {
      // Pasar al siguiente patrón
      faseFadeOut    = false;
      pasoReplay     = 0;
      idxPatronReplay = (idxPatronReplay + 1) % cantidadPatrones;
    }
  }
}

// ------------------ Arduino ------------------

void setup() {
  delay(1000);
  Serial.begin(115200);

  // PWM para LED
  ledcSetup(CANAL_PWM, FREQ_PWM, RES_PWM_BITS);
  ledcAttachPin(PIN_LED, CANAL_PWM);
  aplicarBrillo(0);
  setBrilloObjetivo(0);

  // Info inicial
  Serial.println("Sistema listo: toque para encender gradualmente, suelte para apagar gradualmente.");
  Serial.println("Se grabarán los patrones de contacto. Tras 1 min apagado, se reproducen en bucle.");
}

void loop() {
  // 1) Lectura tactil con histeresis
  int valorTactil = touchRead(PIN_TACTIL);
  bool tocandoAhora = tocando
                      ? (valorTactil < UMBRAL_TACTIL_OFF)   // seguir tocando hasta OFF
                      : (valorTactil < UMBRAL_TACTIL_ON);   // empezar a tocar en ON

  // 2) Interrupción inmediata de reproducción si alguien toca
  if (reproduciendo && tocandoAhora) {
    detenerReproduccion();
  }

  // 3) Detección de flancos para manejar grabación
  if (tocandoAhora && !tocandoPrevio) {
    // Comienzo de contacto: iniciar grabación y objetivo subir
    iniciarGrabacion();
    setBrilloObjetivo(255);
  }
  if (!tocandoAhora && tocandoPrevio) {
    // Fin de contacto: finalizar grabación y objetivo bajar
    finalizarGrabacion();
    setBrilloObjetivo(0);
  }

  // 4) Control manual de rampa (si NO está reproduciendo)
  if (!reproduciendo) {
    actualizarRampa();
  } else {
    // 5) Control de reproducción (si está activo)
    actualizarReproduccion();
  }

  // 6) Muestreo periódico para grabación
  unsigned long ahora = millis();
  if (ahora - tUltimaMuestra >= SAMPLE_INTERVAL_MS) {
    tUltimaMuestra = ahora;
    if (grabando) {
      // Guardamos cómo evoluciona el brillo durante el toque
      grabarMuestra(brilloActual);
    }
  }

  // 7) Autoinicio de reproducción tras 1 minuto en apagado estable
  if (!reproduciendo && !tocandoAhora && brilloActual == 0 && tDesdeApagado > 0) {
    if (millis() - tDesdeApagado >= TIEMPO_INACTIVIDAD_MS && cantidadPatrones > 0) {
      iniciarReproduccion();
      // Si al iniciar el replay estaba en grabación por algún edge raro, cerramos
      if (grabando) finalizarGrabacion();
    }
  }

  // 8) Debug opcional
  // Serial.printf("touch=%d tocando=%d brillo=%d obj=%d grab=%d rep=%d patrones=%d\n",
  //               valorTactil, tocandoAhora, brilloActual, brilloObjetivo,
  //               grabando, reproduciendo, cantidadPatrones);

  tocandoPrevio = tocando;
  tocando       = tocandoAhora;
}
