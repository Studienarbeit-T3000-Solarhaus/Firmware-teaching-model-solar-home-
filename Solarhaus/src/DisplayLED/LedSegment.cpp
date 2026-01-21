#include "LedSegment.hpp"

LedSegment::LedSegment(Adafruit_NeoPixel* mainStrip, int start, int len, uint32_t c) {
  strip = mainStrip;
  startPixel = start;
  length = len;
  color = c;
  position = 0;
  speedMs = 0; // 0 bedeutet: Animation gestoppt
  reverse = false;
  lastUpdate = 0;
}

// --- SET FLOW ---
void LedSegment::setFlow(int newSpeedMs, bool newReverse) {
  speedMs = newSpeedMs;
  reverse = newReverse;
  
  // Wenn Geschwindigkeit auf 0 gesetzt wird, Segment ausschalten
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
  // Wenn Speed 0 ist, mache nichts
  if (speedMs <= 0) return;

  unsigned long currentMillis = millis();

  if (currentMillis - lastUpdate >= speedMs) {
    lastUpdate = currentMillis;

    // 1. Das "alte" Pixel ausschalten (Schwarz)
    // Wir rechnen: Absoluter Start + relative Position
    strip->setPixelColor(startPixel + position, 0);

    // 2. Position berechnen
    if (!reverse) {
      // Vorwärts
      position++;
      if (position >= length) position = 0;
    } else {
      // Rückwärts
      position--;
      if (position < 0) position = length - 1;
    }

    // 3. Das "neue" Pixel einschalten
    strip->setPixelColor(startPixel + position, color);
    
  }
}

// --- CLEAR ---
void LedSegment::clear() {
  for (int i = 0; i < length; i++) {
    strip->setPixelColor(startPixel + i, 0);
  }
}