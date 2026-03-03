#include "LedSegment.hpp"

// --- Konstruktor 1 (Zusammenhängend) ---
LedSegment::LedSegment(Adafruit_NeoPixel* mainStrip, int start, int len, uint32_t c) {
  strip = mainStrip;
  length = len;
  color = c;
  position = 0;
  speedMs = 0; 
  reverse = false;
  lastUpdate = 0;

  // Array dynamisch anlegen und mit fortlaufenden Indizes füllen
  pixelIndices = new int[length];
  for (int i = 0; i < length; i++) {
    pixelIndices[i] = start + i;
  }
}

// --- Konstruktor 2 (Unterbrochen / Array) ---
LedSegment::LedSegment(Adafruit_NeoPixel* mainStrip, const int* indices, int len, uint32_t c) {
  strip = mainStrip;
  length = len;
  color = c;
  position = 0;
  speedMs = 0;
  reverse = false;
  lastUpdate = 0;

  // Array dynamisch anlegen und übergebene Indizes kopieren
  pixelIndices = new int[length];
  for (int i = 0; i < length; i++) {
    pixelIndices[i] = indices[i];
  }
}

// --- Destruktor ---
LedSegment::~LedSegment() {
  delete[] pixelIndices; // Speicher wieder freigeben
}

// --- SET FLOW ---
void LedSegment::setFlow(int newSpeedMs, bool newReverse) {
  speedMs = newSpeedMs;
  reverse = newReverse;
  if (speedMs == 0) {
    clear();
  }
}

// --- SET COLOR ---
void LedSegment::setColor(uint32_t newColor) {
  color = newColor;
}

// --- UPDATE ---
void LedSegment::update() {
  if (speedMs <= 0) return;

  unsigned long currentMillis = millis();

  if (currentMillis - lastUpdate >= speedMs) {
    lastUpdate = currentMillis;

    // 1. NEU: Alle Pixel im Segment abdunkeln (Fade-Effekt)
    for (int i = 0; i < length; i++) {
      uint32_t c = strip->getPixelColor(pixelIndices[i]);
      
      if (c > 0) { // Nur rechnen, wenn die LED nicht sowieso schon aus ist
        // RGB-Werte extrahieren
        uint8_t r = (uint8_t)(c >> 16);
        uint8_t g = (uint8_t)(c >> 8);
        uint8_t b = (uint8_t)c;

        // Werte reduzieren (Skalierungsfaktor). 
        // (Wert * 150) >> 8 ist eine schnelle Division: es bleiben ca. 60% der Helligkeit übrig.
        // TIPP: Erhöhe die 150 (z.B. auf 200) für einen längeren Schweif, verringere sie für einen kürzeren!
        r = (r * 150) >> 8;
        g = (g * 150) >> 8;
        b = (b * 150) >> 8;

        strip->setPixelColor(pixelIndices[i], strip->Color(r, g, b));
      }
    }

    // 2. Position berechnen
    if (!reverse) {
      position++;
      if (position >= length) position = 0;
    } else {
      position--;
      if (position < 0) position = length - 1;
    }

    // 3. Das "neue" Pixel voll einschalten (Kopf des Schweifs)
    strip->setPixelColor(pixelIndices[position], color);
  }
}

// --- CLEAR ---
void LedSegment::clear() {
  for (int i = 0; i < length; i++) {
    strip->setPixelColor(pixelIndices[i], 0);
  }
}

// --- NEU: FILL (für Füllstandsanzeigen wie Kondensatoren) ---
void LedSegment::fill(float percentage, uint32_t fillColor) {
  // Zur Sicherheit: Wertebereich begrenzen
  if (percentage < 0.0) percentage = 0.0;
  if (percentage > 1.0) percentage = 1.0;

  // Genaue Anzahl der LEDs als Fließkommazahl (z.B. 3.7)
  float exactLeds = percentage * length;
  int fullLeds = (int)exactLeds;         // Ganzzahliger Anteil (z.B. 3)
  float fraction = exactLeds - fullLeds; // Nachkomma-Anteil (z.B. 0.7)

  // Farbwerte für die teilweise leuchtende LED berechnen
  uint8_t r = (uint8_t)(fillColor >> 16);
  uint8_t g = (uint8_t)(fillColor >> 8);
  uint8_t b = (uint8_t)fillColor;

  uint8_t part_r = (uint8_t)(r * fraction);
  uint8_t part_g = (uint8_t)(g * fraction);
  uint8_t part_b = (uint8_t)(b * fraction);
  uint32_t partialColor = strip->Color(part_r, part_g, part_b);

  for (int i = 0; i < length; i++) {
    if (i < fullLeds) {
      // Diese LEDs sind voll an
      strip->setPixelColor(pixelIndices[i], fillColor);
    } else if (i == fullLeds && fraction > 0.0) {
      // Das ist die "Übergangs-LED"
      strip->setPixelColor(pixelIndices[i], partialColor);
    } else {
      // Restliche LEDs sind aus
      strip->setPixelColor(pixelIndices[i], 0);
    }
  }
}

// --- NEU: GET LENGTH ---
int LedSegment::getLength() {
  return length;
}