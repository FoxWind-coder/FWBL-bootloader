#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>

void updateProgressBar(uint8_t percent, const char* customMessage = nullptr);
void displayText(const char* text);
void beep(int count, int interval);

#endif
