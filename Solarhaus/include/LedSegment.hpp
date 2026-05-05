#ifndef LEDSEGMENT_HPP
#define LEDSEGMENT_HPP

#include <Arduino.h>           
#include <Adafruit_NeoPixel.h> 

class LedSegment {
  private:
    Adafruit_NeoPixel* strip; 
    int* pixelIndices;       
    int length;               
    int position;             
    unsigned long lastUpdate; 
    int speedMs;              
    bool reverse;             
    uint32_t color;           
    uint32_t lastPixelColor;  
    bool excludeLast;         
    bool pulse;               
    uint8_t pulseValue;       
    int8_t pulseDir;          

  public:
    LedSegment(Adafruit_NeoPixel* mainStrip, int start, int len, uint32_t c);

    LedSegment(Adafruit_NeoPixel* mainStrip, const int* indices, int len, uint32_t c);

    ~LedSegment();

    // Methods
    void setFlow(int newSpeedMs, bool newReverse);
    void setColor(uint32_t newColor);
    void setLastPixelColor(uint32_t newColor); 
    void setExcludeLast(bool exclude); 
    void setPulse(bool p);            
    void update();
    void clear();
    
    // Helper for Battery level 
    void fill(float percentage, uint32_t fillColor);
    int getLength();
};

#endif