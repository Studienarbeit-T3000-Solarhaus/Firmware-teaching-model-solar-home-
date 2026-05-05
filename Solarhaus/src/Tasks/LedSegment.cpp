#include "LedSegment.hpp"

// Constructor for contiguous LED segments (sequential pixel indices starting at 'start')
LedSegment::LedSegment(Adafruit_NeoPixel* mainStrip, int start, int len, uint32_t c) {
  strip = mainStrip;
  length = len;
  color = c;
  lastPixelColor = c; 
  position = 0;
  speedMs = 0; 
  reverse = false;
  lastUpdate = 0;
  excludeLast = false;
  pulse = false;
  pulseValue = 0;
  pulseDir = 1;

  pixelIndices = new int[length];
  for (int i = 0; i < length; i++) {
    pixelIndices[i] = start + i;
  }
}

// Constructor for non-contiguous LED segments (arbitrary pixel indices)
LedSegment::LedSegment(Adafruit_NeoPixel* mainStrip, const int* indices, int len, uint32_t c) {
  strip = mainStrip;
  length = len;
  color = c;
  lastPixelColor = c; 
  position = 0;
  speedMs = 0;
  reverse = false;
  lastUpdate = 0;
  excludeLast = false;
  pulse = false;
  pulseValue = 0;
  pulseDir = 1;

  pixelIndices = new int[length];
  for (int i = 0; i < length; i++) {
    pixelIndices[i] = indices[i];
  }
}

LedSegment::~LedSegment() {
  delete[] pixelIndices; 
}

// Configure flow animation speed and direction; speed=0 disables the effect
void LedSegment::setFlow(int newSpeedMs, bool newReverse) {
  speedMs = newSpeedMs;
  reverse = newReverse;
  if (speedMs == 0) {
    clear();
  }
}

void LedSegment::setColor(uint32_t newColor) {
  color = newColor;
}

void LedSegment::setLastPixelColor(uint32_t newColor) {
  lastPixelColor = newColor;
}

void LedSegment::setExcludeLast(bool exclude) {
  excludeLast = exclude;
}

void LedSegment::setPulse(bool p) {
  pulse = p;
}

// Main animation loop: handles pulsing or flowing trail effect depending on mode
void LedSegment::update() {
  if (speedMs <= 0) return;

  unsigned long currentMillis = millis();

  unsigned long updateInterval = pulse ? 20 : speedMs;

  if (currentMillis - lastUpdate >= updateInterval) {
    lastUpdate = currentMillis;

    int effectiveLength = excludeLast ? (length - 1) : length;
    if (effectiveLength < 0) return;

    if (pulse && effectiveLength > 0) {
      int step = map(speedMs, 200, 30, 5, 25);
      if (step < 2) step = 2;

      pulseValue += (pulseDir * step);
      if (pulseValue >= 255) { pulseValue = 255; pulseDir = -1; }
      if (pulseValue <= 20)  { pulseValue = 20;  pulseDir = 1;  }

      uint8_t r = (uint8_t)(color >> 16);
      uint8_t g = (uint8_t)(color >> 8);
      uint8_t b = (uint8_t)color;

      r = (r * pulseValue * 20) >> 16; 
      g = (g * pulseValue * 20) >> 16;
      b = (b * pulseValue * 20) >> 16;
      uint32_t pulsedColor = strip->Color(r, g, b);

      for (int i = 0; i < effectiveLength; i++) {
        strip->setPixelColor(pixelIndices[i], pulsedColor);
      }
    } else if (effectiveLength > 0) {
      // Fade existing pixels to create a trailing effect
      for (int i = 0; i < effectiveLength; i++) {
        uint32_t c = strip->getPixelColor(pixelIndices[i]);
        if (c > 0) {
          uint8_t r = (uint8_t)(c >> 16);
          uint8_t g = (uint8_t)(c >> 8);
          uint8_t b = (uint8_t)c;
          r = (r * 150) >> 8;
          g = (g * 150) >> 8;
          b = (b * 150) >> 8;
          strip->setPixelColor(pixelIndices[i], strip->Color(r, g, b));
        }
      }
      if (!reverse) {
        position++;
        if (position >= effectiveLength) position = 0;
      } else {
        position--;
        if (position < 0) position = effectiveLength - 1;
      }

      uint8_t r = (uint8_t)(color >> 16);
      uint8_t g = (uint8_t)(color >> 8);
      uint8_t b = (uint8_t)color;
      uint32_t dimmedColor = strip->Color((r*20)>>8, (g*20)>>8, (b*20)>>8);
      strip->setPixelColor(pixelIndices[position], dimmedColor);
    }
    
    if (excludeLast && length > 0) {
      strip->setPixelColor(pixelIndices[length - 1], lastPixelColor);
    }
  }
}

void LedSegment::clear() {
  for (int i = 0; i < length; i++) {
    strip->setPixelColor(pixelIndices[i], 0);
  }
}


// Fill the segment proportionally (e.g. to visualize battery level)
void LedSegment::fill(float percentage, uint32_t fillColor) {
  if (percentage < 0.0) percentage = 0.0;
  if (percentage > 1.0) percentage = 1.0;

  float exactLeds = percentage * length;
  int fullLeds = (int)exactLeds;         
  float fraction = exactLeds - fullLeds; 

  uint8_t r = (uint8_t)(fillColor >> 16);
  uint8_t g = (uint8_t)(fillColor >> 8);
  uint8_t b = (uint8_t)fillColor;

  uint8_t dim_r = (r * 20) >> 8;
  uint8_t dim_g = (g * 20) >> 8;
  uint8_t dim_b = (b * 20) >> 8;
  uint32_t dimmedFillColor = strip->Color(dim_r, dim_g, dim_b);

  uint8_t part_r = (uint8_t)(dim_r * fraction);
  uint8_t part_g = (uint8_t)(dim_g * fraction);
  uint8_t part_b = (uint8_t)(dim_b * fraction);
  uint32_t partialColor = strip->Color(part_r, part_g, part_b);

  for (int i = 0; i < length; i++) {
    if (i < fullLeds) {
      strip->setPixelColor(pixelIndices[i], dimmedFillColor);
    } else if (i == fullLeds && fraction > 0.0) {
      strip->setPixelColor(pixelIndices[i], partialColor);
    } else {
      strip->setPixelColor(pixelIndices[i], strip->Color(2, 2, 2)); 
    }
  }
}

int LedSegment::getLength() {
  return length;
}