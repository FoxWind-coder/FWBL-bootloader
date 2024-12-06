#include "display.h"
#include <TFT_eSPI.h>
#include <deque>

extern TFT_eSPI tft;
extern uint16_t SCREEN_HEIGHT;
extern uint16_t SCREEN_WIDTH;
extern uint8_t BEEPSTATE;

int currentLine = 0;
std::deque<TextLine> textBuffer;

void updateProgressBar(uint8_t percent, const char* customMessage, uint16_t barColor, uint16_t textColor) {
    #ifdef BOOTDISPLAY
        static uint8_t lastPercent = 255; // Последнее выведенное значение процента (255 как начальное значение, чтобы всегда обновлялось при первом вызове)
        static String lastMessage = "";  // Последнее выведенное сообщение

        // Определение координат и размеров с учетом разрешения экрана
        const int16_t margin = 10;       // Отступ от краев экрана
        const int16_t barHeight = 20;    // Высота полосы загрузки
        const int16_t barWidth = SCREEN_WIDTH - 2 * margin; // Ширина полосы загрузки (весь экран минус отступы)

        // Вычисляем размер и положение полосы загрузки
        const int16_t x = margin;
        const int16_t y = SCREEN_HEIGHT - barHeight - margin;

        // Ограничение процента для заполнения полосы
        uint8_t displayPercent = (percent > 100) ? 100 : percent;

        // Рисование только недостающего фрагмента полосы загрузки
        if (displayPercent > lastPercent) {
            int16_t newFillWidth = (barWidth * displayPercent) / 100;
            int16_t oldFillWidth = (barWidth * lastPercent) / 100;
            tft.fillRect(x + oldFillWidth, y, newFillWidth - oldFillWidth, barHeight, barColor);
        }

        // Очистка только при уменьшении значения
        if (displayPercent < lastPercent) {
            tft.fillRect(x, y, barWidth, barHeight, TFT_BLACK);
            int16_t fillWidth = (barWidth * displayPercent) / 100;
            tft.fillRect(x, y, fillWidth, barHeight, barColor);
        }

        // Подготовка текста
        String message;
        if (customMessage != nullptr) {
            message = String(customMessage) + " " + String(displayPercent) + "%";
        } else {
            if (percent > 100) {
                message = "Flashing firmware... skipped " + String(displayPercent) + "%";
            } else {
                message = "Flashing firmware... " + String(displayPercent) + "%";
            }
        }

        // Очистка и обновление текста только при его изменении
        if (message != lastMessage) {
            const int16_t textWidth = 150; // Ширина текста (фиксированное значение)
            const int16_t textX = (SCREEN_WIDTH - textWidth) / 2;
            const int16_t textY = y - 30; // Отступ от полосы загрузки
            tft.fillRect(textX, textY, textWidth, 20, TFT_BLACK);
            tft.setTextColor(textColor, TFT_BLACK);
            tft.setCursor(textX, textY);
            tft.print(message);
            lastMessage = message; // Сохраняем последнее сообщение
        }

        // Сохраняем последнее значение процента
        lastPercent = displayPercent;
    #endif
}


void displayText(const char* text, uint16_t color) {
    #ifdef BOOTDISPLAY
        // Получаем высоту экрана и вычисляем количество строк
        int screenHeight = tft.height();
        int maxLines = (screenHeight - TOP_MARGIN - BOTTOM_MARGIN) / LINE_HEIGHT;

        // Формируем строку с таймкодом и текстом
        unsigned long timeSinceStart = millis();
        char buffer[256];
        snprintf(buffer, sizeof(buffer), "[%lu] %s", timeSinceStart, text);
        String newLine = String(buffer);

        // Если буфер переполнен, смещаем текст вверх
        if (textBuffer.size() >= maxLines) {
            // Очищаем область всего текстового блока
            for (size_t i = 0; i < textBuffer.size(); ++i) {
                int yPos = TOP_MARGIN + i * LINE_HEIGHT;
                tft.fillRect(LEFT_MARGIN, yPos, SCREEN_WIDTH - LEFT_MARGIN, LINE_HEIGHT, TFT_BLACK);
            }

            // Удаляем верхнюю строку из буфера
            textBuffer.pop_front();
        }

        // Добавляем новую строку с цветом в буфер
        textBuffer.push_back({newLine, color});

        // Перерисовываем все строки
        for (size_t i = 0; i < textBuffer.size(); ++i) {
            int yPos = TOP_MARGIN + i * LINE_HEIGHT;

            // Разделяем таймкод и текст
            int separatorIndex = textBuffer[i].text.indexOf(' ');
            String timeCode = textBuffer[i].text.substring(0, separatorIndex);
            String message = textBuffer[i].text.substring(separatorIndex + 1);

            // Отображаем таймкод белым
            tft.setTextColor(TFT_WHITE, TFT_BLACK);
            tft.setCursor(LEFT_MARGIN, yPos);
            tft.print(timeCode);

            // Добавляем отступ в 1 пробел между таймкодом и текстом
            tft.setCursor(LEFT_MARGIN + timeCode.length() * 6 + 6, yPos); // Смещаем начало текста после таймкода на 1 пробел (6 пикселей)

            // Отображаем текст с цветом
            tft.setTextColor(textBuffer[i].color, TFT_BLACK);
            tft.print(message);
        }
    #endif
}


void beep(int count, int interval) {
    #ifdef BEEPER
        if (BEEPSTATE == 1){
            int beepCount = 0;
            unsigned long previousMillis = 0;
            bool isBeeping = false;

            while (beepCount < count) {
                unsigned long currentMillis = millis();
    
                if (isBeeping) {
                    // Проверяем, прошел ли интервал для выключения писка
                    if (currentMillis - previousMillis >= static_cast<unsigned long>(interval)) {
                        digitalWrite(BEEPER, LOW);  // Выключаем пищалку
                        previousMillis = currentMillis;  // Сохраняем текущее время
                        isBeeping = false;  // Готовы к следующему писку
                        beepCount++;  // Увеличиваем счетчик писков
                    }
                } else {
                    // Проверяем, прошел ли интервал для включения следующего писка
                    if (currentMillis - previousMillis >= static_cast<unsigned long>(interval)) {
                        if (beepCount < count) {
                            digitalWrite(BEEPER, HIGH);  // Включаем пищалку
                            previousMillis = currentMillis;  // Сохраняем текущее время
                            isBeeping = true;  // Начинаем новый писк
                        }
                    }
                }
            }
        } else{
            return;
        }
    #endif
}


void displayNote(const char* format, int cellIndex, uint16_t cellColor, uint16_t textColor) {
    // Размеры ячеек
    constexpr int smallCellWidth = 6;  // Ширина маленькой ячейки
    constexpr int numSmallCells = 6;  // Количество маленьких ячеек
    constexpr int cellHeight = TOP_MARGIN;  // Высота строки уведомлений
    const int largeCellWidth = SCREEN_WIDTH - (smallCellWidth * numSmallCells);  // Ширина большой ячейки

    // Определение ширины текущей ячейки
    int xPos, cellWidth;
    if (cellIndex == 0) {
        // Большая ячейка
        xPos = 0;
        cellWidth = largeCellWidth;
    } else {
        // Маленькая ячейка
        xPos = largeCellWidth + (cellIndex - 1) * smallCellWidth;
        cellWidth = smallCellWidth;
    }

    // Обрезка текста, чтобы он поместился в ячейку
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "%s", format);  // Копируем строку формата

    int maxTextWidth = cellWidth / tft.textWidth(" ");
    if (strlen(buffer) > static_cast<size_t>(maxTextWidth)) {
        buffer[maxTextWidth] = '\0';  // Обрезка текста
    }

    // Рисование фона ячейки
    tft.fillRect(xPos, 0, cellWidth, cellHeight, cellColor);

    // Отображение текста
    tft.setTextColor(textColor, cellColor);
    tft.setCursor(xPos + 1, cellHeight / 4);  // Текст с небольшим отступом
    tft.print(buffer);
}