#define SPI_DRIVER_SELECT 2
#include <Arduino.h>
#include <SdFat.h>
#include "stm32f4xx_hal.h"
#include <cstdio> // Для snprintf


#ifdef SPIFLASH
    #include <SPI.h>
    #include <SPIFlash.h>
#endif
// #define BOOTDISPLAY MKS_TS35
// // Выбор платформы
// // #define GENERIC_BOARD
// // #define RN3_BOARD
// #define SKIPR_BOARD

#include <boards.h>

#ifdef BOOTDISPLAY
    // #include <User_Setup.h>
    #include <TFT_eSPI.h> 
    TFT_eSPI tft = TFT_eSPI();
    #define PROGRESS_BAR_HEIGHT 20  // Высота полосы загрузки
    #define PROGRESS_BAR_WIDTH 300  // Ширина полосы загрузки
    uint8_t progress = 0;  // Текущий прогресс в процентах
#endif

#define SD_FAT_TYPE 3 // поддерживаемые ФС карты

#ifdef BOOTLOADER
    #define FIRMWARE_FILE "firmware.bin"
    #define PREFLASH "preloader.bin"
    #define LOG_FILE "flashlog.txt"
    #define PREFLASH_CURRENT "preloader.CUR"
    #define FIRMWARE_CURRENT "firmware.CUR"
    #define INTERNAL_FLASH_START_ADDRESS 0x0800C000
    #define VERMARKER 0x0800BFF0
    #define INTERNAL_FLASH_END_ADDRESS   0x080FFFFF // Адрес конца флеш-памяти для STM32F407VET6
    // #define INTERNAL_FLASH_START_ADDRESS 0x08008000  // адрес начала памяти для прошивки после 32 КБ загрузчика
    // в режиме build также выполняется проверка размера загрузчика, по умолчанию ограничение 32кб
    // #define INTERNAL_FLASH_START_ADDRESS 0x08000000 
#else 
    #define VERMARKER 0x0800BFF0
    #define FIRMWARE_FILE "bootloader.bin"
    #define LOG_FILE "bootup.txt"
    #define FIRMWARE_CURRENT "bootloader.cur"
    #define INTERNAL_FLASH_START_ADDRESS 0x08000000
    // #define APPLICATION_ADDRESS 0x0800C000
    #define INTERNAL_FLASH_END_ADDRESS   0x0800C000 // Адрес конца флеш-памяти для STM32F407VET6
#endif


#define FLASH_SECTOR_SIZE 16384
#define FLASH_SECTOR_SIZE_16K         16384       // Размер сектора 16 КБ
#define FLASH_SECTOR_SIZE_64K         65536       // Размер сектора 64 КБ
#define FLASH_SECTOR_SIZE_128K        131072      // Размер сектора 128 КБ
#define BUFFER_SIZE 256         // Размер буфера

typedef void (*pFunction)(void);
SoftSpiDriver<SOFT_MISO_PIN, SOFT_MOSI_PIN, SOFT_SCK_PIN> softSpi;
#define SD_CONFIG SdSpiConfig(SD_CS_PIN, DEDICATED_SPI, SD_SCK_MHZ(0), &softSpi)

#ifdef SPIFLASH
    SPIFlash flash(csPin, 0xEF40); // 0xEF40 для 16-мбитного чипа Windbond
    #define FLASH_SIZE  8 * 1024 * 1024;
    #define SPI_FLASH_START_ADDRESS  0x0
    #define DCFBYTE 0x7FFFFE //display rotation config byte
    #define BEEPBYTE 0x7FFFFD //beeper config byte
    #define VER_BYTE 0x00
    #define FILL_BYTE 0xFF
#endif

#if SD_FAT_TYPE == 0
SdFat SD;
File file;
#elif SD_FAT_TYPE == 1
SdFat32 SD;
File32 file;
#elif SD_FAT_TYPE == 2
SdExFat SD;
ExFile file;
#elif SD_FAT_TYPE == 3
SdFs SD;
FsFile file;
#else
#error Invalid SD_FAT_TYPE
#endif

int currentLine = 0;
unsigned long startTime;
uint16_t SCREEN_HEIGHT = BASE_SCREEN_HEIGHT;
uint16_t SCREEN_WIDTH = BASE_SCREEN_WIDTH;
uint8_t ROTATION = BASE_ROTATION;
uint8_t BEEPSTATE = BASE_BEEPSTATE;

void logMessage(const char* message);
void logAddress(uint32_t address, uint8_t byte);
void flashFirmwareToInternalMemory();
void flashFirmwareToSPIFlash();
void startApplication(uint32_t address);
void loadFirmwareFromSPIFlashToRAM();
bool isUserButtonPressed();
void beep(int count, int interval);
void updateProgressBar(uint8_t percent, const char* customMessage = nullptr);
void displayText(const char* text);
void writemarker(const char *data, uint32_t startAddress, uint32_t endAddress);
void readmarker(uint32_t startAddress, uint32_t endAddress);
void delayMillis(unsigned long delayTime) {
    unsigned long startTime = millis(); 
    while (millis() - startTime < delayTime) {
    }
}

struct Header {
    char header[4];
    uint8_t signature;
    union {
        struct {
            bool rotate_display: 1;
            bool is_usb_primary: 1;
            bool reserved: 6; // Остальные 6 бит зарезервированы
        };
        uint8_t v;  // Используем 8-битное поле, чтобы не занимать лишнюю память
    } flags;
};

unsigned long millisSinceStart();
word jedecID;
char jedecIDStr[16];
unsigned long startMillis;

void setup() {  
    #ifndef BOOTLOADER
        __set_PRIMASK(1); //запрещаем прерывания

        SCB->VTOR = INTERNAL_FLASH_END_ADDRESS;//переносим начало вектора прерываний по указанному адресу

        __set_PRIMASK(0);//разрешаем прерывания
        // Инициализация HAL
        HAL_Init();
    #endif

    #ifdef SPIFLASH
        Header header;
        // pinMode(csPin, OUTPUT);
        // pinMode(clkPin, OUTPUT);
        // pinMode(diPin, OUTPUT);
        // pinMode(doPin, INPUT);
        SPI.setMISO(doPin);
        SPI.setMOSI(diPin);
        SPI.setSCLK(clkPin);
        SPI.begin();
    #endif

    #ifdef SPIFLASH
        flash.wakeup();
        if (flash.initialize()) {
            logMessage("SPI Flash Init FAIL!");
        } else {
            logMessage("SPI Flash Init OK!");
            jedecID = flash.readDeviceId();  // Получаем JEDEC ID
            // Буфер для строки
            char jedecIDStr[9];  // 8 символов для HEX + 1 для null terminator

            // Форматируем JEDEC ID в шестнадцатеричный формат
            snprintf(jedecIDStr, sizeof(jedecIDStr), "%06lX", jedecID);
            // while (1);
            logMessage(jedecIDStr);
            // flash.blockErase4K(SPI_FLASH_START_ADDRESS);
            // flash.blockErase64K(SPI_FLASH_START_ADDRESS);
            // flash.writeByte(DCFBYTE, 0xFD);
            // flash.writeByte(BEEPBYTE, 0xFB);
            // uint8_t configByte = flash.readByte(DCFBYTE);
            // uint8_t beepconf = flash.readByte(BEEPBYTE);
            // Serial.println(flash.readByte(BEEPBYTE));
            #ifdef SHUI
            flash.readBytes(0, &header, sizeof(Header));
            if (strncmp(header.header, "SHUI", 4) == 0) {
                if (header.flags.rotate_display) {
                    ROTATION = 1; // нормальная ориентация
                    SCREEN_HEIGHT = BASE_SCREEN_WIDTH;
                    SCREEN_WIDTH = BASE_SCREEN_HEIGHT;
                } else {
                    ROTATION = 3; // перевернутый
                    SCREEN_HEIGHT = BASE_SCREEN_WIDTH;
                    SCREEN_WIDTH = BASE_SCREEN_HEIGHT;
                }

                if (header.flags.v & (1 << 2)) {
                    BEEPSTATE = 1; // Включение зуммера
                } else {
                    BEEPSTATE = 0; // Отключение зуммера
                }
            } 
            else {
            
                ROTATION = 1;
                BEEPSTATE = 0;
                // SCREEN_HEIGHT = BASE_SCREEN_WIDTH;
                // SCREEN_WIDTH = BASE_SCREEN_HEIGHT;

            }
            #else
                ROTATION = 1;
                BEEPSTATE = 0;
            #endif


        }
    #else
        ROTATION = 1;
        BEEPSTATE = 1;
    #endif

    pinMode(BEEPER, OUTPUT);

    #ifdef BOOTDISPLAY
    tft.init();
    tft.setRotation(ROTATION);  // Поворот экрана (0-3)1.3
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_ORANGE);
    tft.setCursor(0,0);
    tft.print("FoxLoader v0.1");
    tft.setTextColor(TFT_GREEN);
    #ifndef SHUI
        tft.setTextColor(TFT_BLUE);
        displayText("builded as universal bootloader");
        tft.setTextColor(TFT_GREEN);
    #else
        tft.setTextColor(TFT_BLUE);
        displayText("builded as SHUI bootloader");
        tft.setTextColor(TFT_GREEN);
    #endif

    #ifdef BETA
        tft.setTextColor(TFT_RED);
        displayText("this is BETA build! Use at ur own risk!");
        tft.setTextColor(TFT_GREEN);
    #endif
    displayText("builded as universal bootloader");
    readmarker(VERMARKER, INTERNAL_FLASH_START_ADDRESS);
    updateProgressBar(0); 

    #ifndef BOOTLOADER
        tft.setTextColor(TFT_RED);
        displayText("UPDATING BOOTLOADER!!!");
        displayText("DO NOT TURN OFF SYSTEM");
        tft.setTextColor(TFT_GREEN);
    #endif

    #ifdef SPIFLASH
        jedecID = flash.readDeviceId();  // Получаем JEDEC ID
        snprintf(jedecIDStr, sizeof(jedecIDStr), "jedecID: %06lX", jedecID);
        displayText(jedecIDStr);
        // displayText("testlog0");
    #endif

    #endif
        #ifdef SHUI
            flash.readBytes(0, &header, sizeof(Header));
            if (strncmp(header.header, "SHUI", 4) == 0) {
                displayText("SHUI detected!");
                char buffer[256];
                snprintf(buffer, sizeof(buffer), "Signature: %02X", header.signature);
                displayText(buffer);

                snprintf(buffer, sizeof(buffer), "Rotate Display: %s", header.flags.rotate_display ? "Enabled" : "Disabled");
                displayText(buffer);

                snprintf(buffer, sizeof(buffer), "USB Primary: %s", header.flags.is_usb_primary ? "Yes" : "No");
                displayText(buffer);

                snprintf(buffer, sizeof(buffer), "Buzzer: %s", (header.flags.v & (1 << 2)) ? "Enabled" : "Disabled");
                displayText(buffer);
            } else {
                displayText("SHUI not detected!");
            }
        #endif

    #ifdef DEBUG
    Serial.setRx(DEBUG_RX); 
    Serial.setTx(DEBUG_TX);
    Serial.begin(SERIAL_BAUD);
    while (!Serial) { delay(10); } 
    #warning "debug will slow down bootloader functions! remove -DDEBUG for release "
    displayText("Be careful! debug ENABLED");
    #endif

    startMillis = millis();

    #ifdef SD_DET
    pinMode(SD_DET, INPUT);
    if (!digitalRead(SD_DET)){
        logMessage("SD detected. initializing...");
        displayText("SD detected");
    #endif
    
    if (!SD.begin(SD_CONFIG)) {
        logMessage("SD card init failed");
        displayText("SD card init fail");
        beep(1, 30);
        #ifdef BOOTDISPLAY
            updateProgressBar(155); 
        #endif
        softSpi.end();
        startApplication(INTERNAL_FLASH_START_ADDRESS);
    } else {
        logMessage("SD card init OK");
        displayText("SD init OK!");
        #ifdef BOOTLOADER
        if (SD.exists(PREFLASH)) {
            file = SD.open(PREFLASH, FILE_READ);
            if (file) {
                logMessage("updater file opened.");
                displayText("Opened updater file");
                beep(3, 200);
                #ifdef BOOTDISPLAY
                    updateProgressBar(0); 
                #endif
                flashFirmwareToInternalMemory();
                file.close();
                SD.remove(PREFLASH_CURRENT);
                SD.rename(PREFLASH, PREFLASH_CURRENT);
            } else {
                logMessage("Failed to open updater file.");
                displayText("Failed to load updater");
                beep(2, 300);
                #ifdef BOOTDISPLAY
                    updateProgressBar(155); 
                #endif
            }
        } else 
        #endif
        if (SD.exists(FIRMWARE_FILE)) {
            file = SD.open(FIRMWARE_FILE, FILE_READ);
            if (file) {
                logMessage("Firmware file opened.");
                displayText("Opened firmware file");
                beep(3, 200);
                #ifdef BOOTDISPLAY
                    updateProgressBar(0); 
                #endif
                flashFirmwareToInternalMemory();
                writemarker("FWBL v0.1", VERMARKER,INTERNAL_FLASH_START_ADDRESS);
                #ifndef BOOTLOADER
                #endif
                file.close();
                SD.remove(FIRMWARE_CURRENT);
                SD.rename(FIRMWARE_FILE, FIRMWARE_CURRENT);
            } else {
                logMessage("Failed to open firmware file.");
                displayText("Failed to open firmware");
                beep(2, 300);
                #ifdef BOOTDISPLAY
                    updateProgressBar(155); 
                #endif
            }
        
        }
        else {
            logMessage("Firmware file not found.");
            displayText("Firmware not found");
            beep(3, 100);
            #ifdef BOOTDISPLAY
                updateProgressBar(155); 
            #endif
        }
    }

    #ifdef SD_DET
    }
    else {
        logMessage("SD card not found");
        displayText("SD not found");
        beep(1, 700);
        #ifdef BOOTDISPLAY
            updateProgressBar(155); 
        #endif
    }
    #endif
    
    SD.end();
    softSpi.end();
    logMessage("loading internal data");
    displayText("Starting main app");
    #ifdef DEBUG
    Serial.end();
    #endif
    startApplication(INTERNAL_FLASH_START_ADDRESS); 
}


void logMessage(const char* message) {
    #ifdef DEBUG
        Serial.println(message);
    #endif

    #ifdef SDLOG
        File logFile = SD.open(LOG_FILE, FILE_WRITE);
        if (logFile) {
            logFile.println(message);
            logFile.close();
        }
    #endif
}

  #ifdef BOOTDISPLAY 
void updateProgressBar(uint8_t percent, const char* customMessage) {
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
}
  #endif 

void logAddress(uint32_t address, uint8_t byte) {
    char buffer[50];
    snprintf(buffer, sizeof(buffer), "[%lu] addr: 0x%08X byte: 0x%02X", millisSinceStart(), address, byte);
    logMessage(buffer);
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
                if (currentMillis - previousMillis >= interval) {
                    digitalWrite(BEEPER, LOW);  // Выключаем пищалку
                    previousMillis = currentMillis;  // Сохраняем текущее время
                    isBeeping = false;  // Готовы к следующему писку
                    beepCount++;  // Увеличиваем счетчик писков
                }
            } else {
                // Проверяем, прошел ли интервал для включения следующего писка
                if (currentMillis - previousMillis >= interval) {
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



void flashFirmwareToInternalMemory() {
    uint32_t address = INTERNAL_FLASH_START_ADDRESS;
    uint32_t fileSize = file.size();
    uint32_t endAddress = address + fileSize - 1; // Конечный адрес для прошивки

    // Определяем секторы, которые необходимо стереть
    uint32_t startSector = (address - 0x08000000) / FLASH_SECTOR_SIZE_16K; // Переводим адрес в номер сектора
    uint32_t endSector = (endAddress - 0x08000000) / FLASH_SECTOR_SIZE_16K;

    // Настройка диапазона секторов для стирания
    FLASH_EraseInitTypeDef eraseInitStruct;
    uint32_t sectorError;
    HAL_FLASH_Unlock();

    // Определение сектора и его размера
    if (startSector <= 3) {
        eraseInitStruct.Sector = startSector;
        eraseInitStruct.NbSectors = (endSector <= 3) ? (endSector - startSector + 1) : (4 - startSector);
        eraseInitStruct.VoltageRange = FLASH_VOLTAGE_RANGE_3;
        displayText("Erasing sectors");
        eraseInitStruct.TypeErase = FLASH_TYPEERASE_SECTORS;
        if (HAL_FLASHEx_Erase(&eraseInitStruct, &sectorError) != HAL_OK) {
            char logBuffer[256];
            snprintf(logBuffer, sizeof(logBuffer), "Error erasing flash. ERR %lu", sectorError);
            displayText(logBuffer);
            logMessage(logBuffer);
            HAL_FLASH_Lock();
            return;
        }
    }

    if (endSector > 3) {
        eraseInitStruct.Sector = 4;
        eraseInitStruct.NbSectors = (endSector >= 7) ? 4 : (endSector - 4 + 1);
        eraseInitStruct.VoltageRange = FLASH_VOLTAGE_RANGE_3;

        eraseInitStruct.TypeErase = FLASH_TYPEERASE_SECTORS;
        if (HAL_FLASHEx_Erase(&eraseInitStruct, &sectorError) != HAL_OK) {
            char logBuffer[256];
            snprintf(logBuffer, sizeof(logBuffer), "Error erasing flash. ERR %lu", sectorError);
            displayText(logBuffer);
            logMessage(logBuffer);
            HAL_FLASH_Lock();
            return;
        }
    }

    // Запись данных
    size_t totalBytes = fileSize;
    size_t bytesWritten = 0;
    uint8_t buffer[256]; // Буфер для записи данных

    logMessage("Starting flash write...");
    displayText("Writing");
    while (file.available()) {
        size_t bytesRead = file.read(buffer, sizeof(buffer));
        bytesWritten += bytesRead;

        for (size_t i = 0; i < bytesRead; i += 4) {
            uint32_t data = 0;
            if (i + 4 <= bytesRead) {
                data = *(uint32_t*)(buffer + i);
            } else {
                // Если последняя часть данных меньше 4 байт, завершаем запись
                uint8_t tempBuffer[4] = {0};
                memcpy(tempBuffer, buffer + i, bytesRead - i);
                data = *(uint32_t*)tempBuffer;
            }

            if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, address, data) != HAL_OK) {
                char logBuffer[256];
                snprintf(logBuffer, sizeof(logBuffer), "Error programming flash memory at address: 0x%08X", address);
                displayText(logBuffer);
                logMessage(logBuffer);
                snprintf(logBuffer, sizeof(logBuffer), "Data: 0x%08X", data);
                displayText(logBuffer);
                logMessage(logBuffer);
                HAL_FLASH_Lock();
                return;
            }

            #ifdef DEBUG
            logAddress(address, data & 0xFF);
            #endif

            address += 4;
        }

        #ifdef BOOTDISPLAY
        // Обновляем полосу загрузки каждые 5%
        uint8_t newProgress = (bytesWritten * 100) / totalBytes;
        if (newProgress >= progress + 5) {
            progress = newProgress;
            updateProgressBar(progress);
            char logBuffer[256];
            snprintf(logBuffer, sizeof(logBuffer), "Progress: %d%%", progress);
            logMessage(logBuffer);
        }
        #endif
    }

    HAL_FLASH_Lock();
    logMessage("Firmware flashed to internal memory.");
    displayText("Firmware flashed to internal memory.");
}

void writemarker(const char *data, uint32_t startAddress, uint32_t endAddress) {
    uint32_t length = strlen(data);
    uint32_t address = startAddress;

    // Проверка на переполнение памяти
    if ((address + length - 1) > endAddress) {
        tft.setTextColor(TFT_RED);
        displayText("Error: Data exceeds flash boundary.");
        tft.setTextColor(TFT_GREEN);
        return;
    }

    HAL_FLASH_Unlock();

    for (uint32_t i = 0; i < length; i += 4) {
        uint32_t word = 0xFFFFFFFF;  // Инициализация значением, чтобы неиспользуемые байты оставались 0xFF

        // Копируем до 4 байт из строки в переменную word
        memcpy(&word, &data[i], length - i >= 4 ? 4 : length - i);

        // Запись 32-битного слова во флеш
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, address, word) != HAL_OK) {
            tft.setTextColor(TFT_RED);
            displayText("Error: Flash programming failed.");
            tft.setTextColor(TFT_GREEN);
            HAL_FLASH_Lock();
            return;
        }

        address += 4; // Переход к следующему 4-байтному блоку
    }

    HAL_FLASH_Lock();
    displayText("Marker successfully written to flash.");
}

void readmarker(uint32_t startAddress, uint32_t endAddress) {
    tft.setTextColor(TFT_ORANGE);
    displayText("reading version marker");
    char buffer[256]; // Буфер для одной строки, которую будем передавать в displayText
    memset(buffer, 0, sizeof(buffer)); // Инициализация буфера нулями

    for (uint32_t address = startAddress; address <= endAddress; address++) {
        uint8_t byte = *(__IO uint8_t*)address;

        // Форматирование считанного байта в строку
        int len = strlen(buffer);
        sprintf(buffer + len, "%c", byte);

        // Если буфер заполнился или достигнут конец диапазона, выводим строку и очищаем буфер
        if (len >= sizeof(buffer) - 2 || address == endAddress) {
            displayText(buffer);
            tft.setTextColor(TFT_GREEN);
            memset(buffer, 0, sizeof(buffer)); // Очистка буфера для следующего вывода
        }
    }
}

void startApplication(uint32_t APPLICATION_ADDRESS) {
    uint32_t app_jump_address;
	
    typedef void(*pFunction)(void);//объявляем пользовательский тип
    pFunction Jump_To_Application;//и создаём переменную этого типа

     __disable_irq();//запрещаем прерывания
		
    app_jump_address = *( uint32_t*) (APPLICATION_ADDRESS + 4);    //извлекаем адрес перехода из вектора Reset
    Jump_To_Application = (pFunction)app_jump_address;            //приводим его к пользовательскому типу
      __set_MSP(*(__IO uint32_t*) APPLICATION_ADDRESS);          //устанавливаем SP приложения   
    __set_PRIMASK(1); //запрещаем прерывания

    SCB->VTOR = APPLICATION_ADDRESS;//переносим начало вектора прерываний по указанному адресу

    __set_PRIMASK(0);//разрешаем прерывания                                      
    Jump_To_Application();		                        //запускаем приложение	
}

unsigned long millisSinceStart() {
    return millis() - startMillis;
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

void loop() {
}
