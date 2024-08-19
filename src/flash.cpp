#include "flash.h"
#include <SdFat.h>
#include <SPIFlash.h>
#include <TFT_eSPI.h>
#include "timerutl.h"
#include "display.h"
#include <cinttypes>  // Для PRIX32

extern FsFile file;
extern SPIFlash flash;
extern TFT_eSPI tft;
extern uint8_t progress;
extern SdFs SD;

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

void logAddress(uint32_t address, uint8_t byte) {
    char buffer[50];
    snprintf(buffer, sizeof(buffer), "[%lu] addr: 0x%" PRIX32 " byte: 0x%02X", millisSinceStart(), address, byte);
    logMessage(buffer);
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
                snprintf(logBuffer, sizeof(logBuffer), "Error programming flash memory at address: 0x%" PRIX32, address);
                displayText(logBuffer);
                logMessage(logBuffer);
                snprintf(logBuffer, sizeof(logBuffer), "Data: 0x%" PRIX32, data);
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
        if (static_cast<unsigned int>(len) >= sizeof(buffer) - 2 || address == endAddress) {
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


void backupFlash(uint32_t startAddress, uint32_t endAddress, const char* filename) {
    #ifdef BOOTLOADER
    // Открытие файла на SD карте для записи
    FsFile backupFile = SD.open(filename, FILE_WRITE);
    if (!backupFile) {
        logMessage("Error: Failed to open file on SD card for backup.");
        return;
    }

    // Буфер для чтения данных из флеш-памяти
    uint8_t buffer[256];
    uint32_t currentAddress = startAddress;
    size_t totalBytes = endAddress - startAddress + 1;
    size_t bytesRead = 0;

    logMessage("Starting flash backup to SD card...");
    displayText("Backing up flash");

    while (currentAddress <= endAddress) {
        // Чтение данных из флеш-памяти
        size_t chunkSize = (endAddress - currentAddress + 1 > sizeof(buffer)) ? sizeof(buffer) : endAddress - currentAddress + 1;
        memcpy(buffer, (uint8_t*)currentAddress, chunkSize);

        // Запись данных на SD карту
        backupFile.write(buffer, chunkSize);
        bytesRead += chunkSize;
        currentAddress += chunkSize;

        // Обновление полосы прогресса каждые 5%
        uint8_t newProgress = (bytesRead * 100) / totalBytes;
        if (newProgress >= progress + 5) {
            progress = newProgress;
            updateProgressBar(progress, "backuping...");
            char logBuffer[256];
            snprintf(logBuffer, sizeof(logBuffer), "Backup progress: %d%%", progress);
            logMessage(logBuffer);
        }
    }

    backupFile.close();
    logMessage("Flash backup completed successfully.");
    displayText("Backup complete");
    #endif
}

