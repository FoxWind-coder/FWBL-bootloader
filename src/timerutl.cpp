#include "timerutl.h"
#include <Arduino.h>

unsigned long millisSinceStart() {
    return millis();
}

void delayMillis(unsigned long delayTime) {
    unsigned long startTime = millis(); 
    while (millis() - startTime < delayTime) {}
}
