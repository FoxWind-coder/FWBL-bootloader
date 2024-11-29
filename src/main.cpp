#define SPI_DRIVER_SELECT 2
#include <Arduino.h>
#include "stm32f4xx_hal.h"
#include "stm32f4xx.h"
#include <SdFat.h>
#include <usbd_cdc_if.h>
#include <cstdio> // Для snprintf

#ifdef USBSERDBG
#include <USBSerial.h>
#endif

#ifdef SPIFLASH
    #define FILE_NAME "spitest"
    #define BUFFER_SIZE 80
    #include <SPI.h>
    #include <SPIFlash.h>
#endif

#include <boards.h>
#include "flash.h"
#include "display.h"
#include "timerutl.h"

#ifdef WIFI
#include <HardwareSerial.h>
HardwareSerial SerialESP(SERIAL_PORT);
#endif

#ifdef BOOTDISPLAY
    #include <TFT_eSPI.h>
    TFT_eSPI tft = TFT_eSPI();
    uint8_t progress = 0;  // Текущий прогресс в процентах
#endif

#define SD_FAT_TYPE 3

#ifdef SDLOG
int logstate = 0;
#endif

#ifdef POWERHOLD
int pwrhold = 0;
#endif

typedef void (*pFunction)(void);
SoftSpiDriver<SOFT_MISO_PIN, SOFT_MOSI_PIN, SOFT_SCK_PIN> softSpi;
#define SD_CONFIG SdSpiConfig(SD_CS_PIN, DEDICATED_SPI, SD_SCK_MHZ(0), &softSpi)

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

// int currentLine = 0;
unsigned long startTime;
uint16_t SCREEN_HEIGHT = BASE_SCREEN_HEIGHT;
uint16_t SCREEN_WIDTH = BASE_SCREEN_WIDTH;
uint8_t ROTATION = BASE_ROTATION;
uint8_t BEEPSTATE = BASE_BEEPSTATE;

#ifdef SPIFLASH
    SPIFlash flash(csPin, 0xEF40); // 0xEF40 для 16-мбитного чипа Windbond
    #define FLASH_SIZE  8 * 1024 * 1024;
    #define SPI_FLASH_START_ADDRESS  0x0
#endif

#ifdef SPIFLASH
// old header
// struct Header {
//     char header[4];
//     uint8_t signature;
//     union {
//         struct {
//             bool rotate_display: 1;
//             bool is_usb_primary: 1;
//             bool reserved: 6; // Остальные 6 бит зарезервированы
//         };
//         uint8_t v;  // Используем 8-битное поле, чтобы не занимать лишнюю память
//     } flags;
// };

struct Header{
    char        header[4];
    uint8_t   signature;
    union {
        struct {
            bool rotate_display: 1;
            bool is_usb_primary:1;
            bool bootloader_beeper:1;
            bool bootloader_power_lock:1;
            bool bootloader_log:1;
        };
    };
    };

word jedecID;
char jedecIDStr[16];
#endif
unsigned long startMillis;

void saveAndCompareFlashData() {
    uint8_t buffer[BUFFER_SIZE];
    uint8_t existingData[BUFFER_SIZE];
    
    // Шаг 1: Считываем первые 80 байт из SPI flash
    flash.readBytes(0, buffer, BUFFER_SIZE);  // Предполагается, что адрес чтения - 0

    // Шаг 2: Открываем файл на SD-карте
    if (file.open(FILE_NAME, FILE_WRITE)) {
        file.write(buffer, BUFFER_SIZE);
        file.close();
        displayText("Data saved to SD card.");
    } else {
        displayText("Failed to open file for writing.");
        return;
    }

    // Шаг 3: Проверяем существование файла для сравнения
    if (file.open(FILE_NAME, FILE_READ)) {
        file.read(existingData, BUFFER_SIZE);
        file.close();

        // Шаг 4: Сравниваем данные
        bool dataModified = false;
        String modifiedBytes = "";

        for (int i = 0; i < BUFFER_SIZE; i++) {
            if (buffer[i] != existingData[i]) {
                dataModified = true;
                modifiedBytes += String(i) + ": " + String(buffer[i], DEC) + " (was: " + String(existingData[i], DEC) + ")\n";
            }
        }

        // Шаг 5: Выводим результат сравнения
        if (dataModified) {
            String outputMessage = "Modified bytes:" + modifiedBytes;
            displayText(outputMessage.c_str());
        } else {
            displayText("No changes detected.");
        }
    } else {
        displayText("Failed to open file for reading.");
    }
}

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

    #ifdef WIFI   
    pinMode(RST_PIN, OUTPUT);
    pinMode(GPIO0_PIN, OUTPUT);
    
    SerialESP.setRx(WIFIRX);
    SerialESP.setTx(WIFITX);
    SerialESP.begin(WIFIBAUD);
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
            snprintf(jedecIDStr, sizeof(jedecIDStr), "%06X", jedecID);
            // while (1);
            logMessage(jedecIDStr);
            
        #ifdef SHUI
            flash.readBytes(setaddr, &header, sizeof(Header)); // Читаем данные в структуру Header
            if (strncmp(header.header, "SHUI", 4) == 0) {
                // Проверяем флаги с помощью битовых операций
                if (header.rotate_display) {
                    ROTATION = 1; // нормальная ориентация
                    SCREEN_HEIGHT = BASE_SCREEN_WIDTH;
                    SCREEN_WIDTH = BASE_SCREEN_HEIGHT;
                } else {
                    ROTATION = 3; // перевернутый
                    SCREEN_HEIGHT = BASE_SCREEN_WIDTH;
                    SCREEN_WIDTH = BASE_SCREEN_HEIGHT;
                }

                // Установка состояния логирования
                #ifdef SDLOG
                    logstate = header.bootloader_log ? 1 : 0; // лог на карту или без лога
                #endif

                #ifdef POWERHOLD
                    pwrhold = header.bootloader_power_lock ? 1 : 0;
                      if (pwrhold == 1) {
                        pinMode(HOLD_PIN, OUTPUT);
                        digitalWrite(HOLD_PIN, HIGH); // Устанавливаем высокий уровень
                    } else {
                        digitalWrite(HOLD_PIN, LOW); // Устанавливаем низкий уровень
                    }
                #endif

                // Установка состояния зуммера
                BEEPSTATE = header.bootloader_beeper ? 1 : 0; // Включение или отключение зуммера
            } else {
                ROTATION = 1;
                BEEPSTATE = 0;

                // SCREEN_HEIGHT = BASE_SCREEN_WIDTH;
                // SCREEN_WIDTH = BASE_SCREEN_HEIGHT;
            }
        #else
            // Значения по умолчанию, если SHUI не определен
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
    tft.print("With love by FoxWind ^_^");
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
    readmarker(VERMARKER, INTERNAL_FLASH_START_ADDRESS);
    updateProgressBar(0); 

    #ifndef BOOTLOADER
        tft.setTextColor(TFT_RED);
        displayText("UPDATING BOOTLOADER!!!");
        displayText("DO NOT TURN OFF SYSTEM");
        tft.setTextColor(TFT_GREEN);
    #endif

    #ifndef SPIFLASH
        displayText("spi flash not defined");
    #else
        // jedecID = flash.readDeviceId();  // Получаем JEDEC ID
        // snprintf(jedecIDStr, sizeof(jedecIDStr), "jedecID: %06X", jedecID);
        displayText(jedecIDStr);
        // displayText("testlog0");
    #endif

    #endif
        #ifdef SHUI
            // flash.readBytes(setaddr, &header, sizeof(Header));
            if (strncmp(header.header, "SHUI", 4) == 0) {
                displayText("SHUI detected!");
                char buffer[256];
                snprintf(buffer, sizeof(buffer), "Signature: %02X", header.signature);
                displayText(buffer);

                snprintf(buffer, sizeof(buffer), "Rotate Display: %s", header.rotate_display ? "true" : "false");
                displayText(buffer);

                snprintf(buffer, sizeof(buffer), "USB Primary: %s", header.is_usb_primary ? "true" : "false");
                displayText(buffer);

                snprintf(buffer, sizeof(buffer), "Buzzer: %s", header.bootloader_beeper ? "true" : "false");
                displayText(buffer);

                snprintf(buffer, sizeof(buffer), "Logging: %s", header.bootloader_log ? "true" : "false");
                displayText(buffer);
            } else {
                displayText("SHUI not detected!");
            }
        #endif

    #ifdef DEBUG
        #warning "debug will slow down bootloader functions! remove -DDEBUG for release "
        #ifndef USBSERDBG
            Serial.setRx(DEBUG_RX); 
            Serial.setTx(DEBUG_TX);
        #endif

        Serial.begin(SERIAL_BAUD);
        // while (true)
        // {
        //    Serial.println("24234");
        //    delay(100);
        // }
        displayText("Be careful! debug ENABLED");
        displayText("Waiting for debugger...");
        unsigned long timeout = millis() + 10000;  // Устанавливаем таймаут на 10 секунд
        while (!Serial && millis() < timeout) {  // Ожидаем подключения или 10 секунд
        }

        if (Serial) {
            logMessage("Debugger connected");  // Если подключение установлено
        } else {
            logMessage("debug timeout");  // Если подключение не установлено за 10 секунд
        }
       
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


        // saveAndCompareFlashData();


        #ifdef WIFI
            if (SD.exists(WIFI_FILE)) { //восстановление прошивки из бекапа
                file = SD.open(WIFI_FILE, FILE_READ);
                if (file) {
                    logMessage("wifi file opened.");
                    displayText("Opened wifi file");
                    beep(2, 200);
                    #ifdef BOOTDISPLAY
                        updateProgres#ifdef USBSERsBar(0); 
                    #endif
                    // backupESP8266(WIFIBACKUP);
                    flashESP8266(WIFI_FILE);
                    file.close();
                    SD.remove(WIFI_CUR);
                    SD.rename(WIFI_FILE, WIFI_CUR);
                } else {
                    logMessage("Failed to open wifi file.");
                    displayText("Failed to load wifi frw");
                    beep(2, 300);
                    #ifdef BOOTDISPLAY
                        updateProgressBar(155); 
                    #endif
                }
            }
        #endif

            if (SD.exists(BACKUPNAME)) { //восстановление прошивки из бекапа
                file = SD.open(BACKUPNAME, FILE_READ);
                if (file) {
                    logMessage("backup file opened.");
                    displayText("Opened backup file");
                    beep(3, 200);
                    #ifdef BOOTDISPLAY
                        updateProgressBar(0); 
                    #endif
                    flashFirmwareToInternalMemory();
                    writemarker(BLVER, VERMARKER,INTERNAL_FLASH_START_ADDRESS);
                    file.close();
                    SD.remove(BACKUP_CURRENT);
                    SD.rename(BACKUPNAME, BACKUP_CURRENT);
                } else {
                    logMessage("Failed to open backup file.");
                    displayText("Failed to load backup");
                    beep(2, 300);
                    #ifdef BOOTDISPLAY
                        updateProgressBar(155); 
                    #endif
                }
            }

        if (SD.exists(PREFLASH)) {
            file = SD.open(PREFLASH, FILE_READ);
            if (file) {
                logMessage("updater file opened.");
                displayText("Opened updater file");
                beep(3, 200);
                #ifdef BOOTDISPLAY
                    updateProgressBar(0); 
                #endif
                displayText("generating backup...");

                if(SD.exists(FIRMWARE_FILE)){
                    displayText("firmware found. backup not generated");
                }else{
                    backupFlash(INTERNAL_FLASH_START_ADDRESS, INTERNAL_FLASH_END_ADDRESS, BACKUPNAME);  
                }
                
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
                writemarker(BLVER, VERMARKER,INTERNAL_FLASH_START_ADDRESS);
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

void loop() {
    
}