#ifndef LEDSEGMENT_HPP
#define LEDSEGMENT_HPP

#include <Arduino.h>           // Wichtig für millis(), uint32_t, etc.
#include <Adafruit_NeoPixel.h> // Zugriff auf die NeoPixel-Datentypen

class LedSegment {
  private:
    Adafruit_NeoPixel* strip; // Zeiger auf den Hauptstreifen
    int startPixel;           // Start-Index
    int length;               // Länge des Segments
    int position;             // Aktuelle Lauflicht-Position
    unsigned long lastUpdate; // Zeitstempel
    int speedMs;              // Geschwindigkeit
    bool reverse;             // Richtung
    uint32_t color;           // Farbe

  public:
    // Konstruktor
    LedSegment(Adafruit_NeoPixel* mainStrip, int start, int len, uint32_t c);

    // Methoden
    void setFlow(int newSpeedMs, bool newReverse);
    void setColor(uint32_t newColor);
    void update();
    void clear();
};

#endif