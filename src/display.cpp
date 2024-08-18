#include "display.h"
#include <TFT_eSPI.h>

extern TFT_eSPI tft;
extern uint16_t SCREEN_HEIGHT;
extern uint16_t SCREEN_WIDTH;
extern uint8_t BEEPSTATE;
int currentLine = 0;

void updateProgressBar(uint8_t percent, const char* customMessage) {
    #ifdef BOOTDISPLAY 
        // Определение координат и размеров с учетом разрешения экрана
        const int16_t margin = 10; // Отступ от краев экрана
        const int16_t barHeight = 20; // Высота полосы загрузки
        const int16_t barWidth = SCREEN_WIDTH - 2 * margin; // Ширина полосы загрузки (весь экран минус отступы)

        // Вычисляем размер и положение полосы загрузки
        const int16_t x = margin;
        const int16_t y = SCREEN_HEIGHT - barHeight - margin;

        // Ограничение процента для заполнения полосы
        uint8_t displayPercent = (percent > 100) ? 100 : percent;

        // Очистка области полосы загрузки
        tft.fillRect(x, y, barWidth, barHeight, TFT_BLACK);

        // Рисование полосы загрузки
        int16_t fillWidth = (barWidth * displayPercent) / 100;
        tft.fillRect(x, y, fillWidth, barHeight, TFT_GREEN);

        // Настройка текста
        tft.setTextColor(TFT_GREEN, TFT_BLACK);
        
        // Подготовка текста с учетом процента и статуса
        String message;
        if (customMessage != nullptr) {
            message = String(customMessage) + " %d%%";  // Форматируем кастомное сообщение с добавлением процентов
        } else {
            if (percent > 100) {
                tft.setTextColor(TFT_RED, TFT_BLACK);
                message = "Flashing firmware... skipped %d%%";
            } else {
                tft.setTextColor(TFT_GREEN, TFT_BLACK);
                message = "Flashing firmware... %d%%";
            }
        }
        
        // Ширина текста
        int16_t textWidth = 150; // Ширина текста (фиксированное значение)
        
        // Вычисление координат для текста
        int16_t textX = (SCREEN_WIDTH - textWidth) / 2;
        int16_t textY = y - 30; // Отступ от полосы загрузки
        
        // Очистка области для текста
        tft.fillRect(textX, textY, textWidth, 20, TFT_BLACK);  // Заполняем прямоугольник черным цветом (высота 20 - примерная высота текста)

        // Отображение текста
        tft.setCursor(textX, textY);
        tft.printf(message.c_str(), displayPercent);
    #endif 
}

void displayText(const char* text) {
#ifdef BOOTDISPLAY
    // Получаем высоту экрана
    int screenHeight = tft.height();
    
    // Вычисляем количество строк, которые можно вывести на экран
    int maxLines = (screenHeight - TOP_MARGIN - BOTTOM_MARGIN) / LINE_HEIGHT;

    // Проверяем, не вышли ли за пределы экрана
    if (currentLine >= maxLines) {
        currentLine = 0; // Возвращаемся к первой строке
    }

    // Вычисляем позицию по Y для вывода текста
    int yPos = TOP_MARGIN + currentLine * LINE_HEIGHT;

    // Получаем время с момента запуска в миллисекундах
    unsigned long timeSinceStart = millis();

    // Формируем строку с временем
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "[%lu] %s", timeSinceStart, text);

    // Выводим текст на дисплей
    tft.setCursor(LEFT_MARGIN, yPos);
    tft.print(buffer);

    // Переходим к следующей строке
    currentLine++;
#endif
}

void beep(int count, int interval) {
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
}
// Реализация остальных функций...
