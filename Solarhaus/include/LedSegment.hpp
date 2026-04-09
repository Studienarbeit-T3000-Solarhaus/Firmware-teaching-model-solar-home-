#ifndef LEDSEGMENT_HPP
#define LEDSEGMENT_HPP

#include <Arduino.h>           
#include <Adafruit_NeoPixel.h> 

class LedSegment {
  private:
    Adafruit_NeoPixel* strip; 
    int* pixelIndices;        // Speichert die genauen Indizes der LEDs
    int length;               
    int position;             
    unsigned long lastUpdate; 
    int speedMs;              
    bool reverse;             
    uint32_t color;           
    uint32_t lastPixelColor;  // NEU: Eigene Farbe für das statische End-Pixel
    bool excludeLast;         // Letztes Pixel vom Lauflicht ausschließen
    bool pulse;               // Pulsieren statt Lauflicht
    uint8_t pulseValue;       // Aktueller Helligkeitswert für Puls
    int8_t pulseDir;          // Richtung des Pulsierens (1 oder -1)

  public:
    // 1. Alter Konstruktor (für zusammenhängende Segmente)
    LedSegment(Adafruit_NeoPixel* mainStrip, int start, int len, uint32_t c);

    // 2. NEUER Konstruktor (für unterbrochene Segmente anhand eines Arrays)
    LedSegment(Adafruit_NeoPixel* mainStrip, const int* indices, int len, uint32_t c);

    // Destruktor
    ~LedSegment();

    // Methoden
    void setFlow(int newSpeedMs, bool newReverse);
    void setColor(uint32_t newColor);
    void setLastPixelColor(uint32_t newColor); // NEU
    void setExcludeLast(bool exclude); 
    void setPulse(bool p);            
    void update();
    void clear();
    
    // Hilfsfunktionen für statische Füllstände
    void fill(float percentage, uint32_t fillColor);
    int getLength();
};

#endif