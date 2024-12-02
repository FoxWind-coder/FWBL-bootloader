#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>
#include <TFT_eSPI.h>  // Добавляем подключение библиотеки TFT_eSPI
#include <deque> // Для работы с буфером строк

// Структура для строки текста и цвета
struct TextLine {
    String text;
    uint16_t color;
};

void updateProgressBar(uint8_t percent, const char* customMessage = nullptr);
void displayText(const char* text, uint16_t color = TFT_GREEN);
void beep(int count, int interval);

#endif
