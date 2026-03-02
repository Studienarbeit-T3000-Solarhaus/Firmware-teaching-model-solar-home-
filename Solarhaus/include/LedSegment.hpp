#ifndef LEDSEGMENT_HPP
#define LEDSEGMENT_HPP

#include <Arduino.h>           
#include <Adafruit_NeoPixel.h> 

class LedSegment {
  private:
    Adafruit_NeoPixel* strip; 
    int* pixelIndices;        // NEU: Speichert die genauen Indizes der LEDs
    int length;               
    int position;             
    unsigned long lastUpdate; 
    int speedMs;              
    bool reverse;             
    uint32_t color;           

  public:
    // 1. Alter Konstruktor (für zusammenhängende Segmente, abwärtskompatibel)
    LedSegment(Adafruit_NeoPixel* mainStrip, int start, int len, uint32_t c);

    // 2. NEUER Konstruktor (für unterbrochene Segmente anhand eines Arrays)
    LedSegment(Adafruit_NeoPixel* mainStrip, const int* indices, int len, uint32_t c);

    // Destruktor
    ~LedSegment();

    // Methoden
    void setFlow(int newSpeedMs, bool newReverse);
    void setColor(uint32_t newColor);
    void update();
    void clear();
    
    // NEU: Hilfsfunktionen für statische Füllstände
    void fill(float percentage, uint32_t fillColor);
    int getLength();
};

#endif