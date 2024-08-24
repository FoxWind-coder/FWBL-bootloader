#define SPI_DRIVER_SELECT 2
#include <Arduino.h>
#include <SdFat.h>
#include "stm32f4xx_hal.h"
#include <cstdio> // Для snprintf

#ifdef SPIFLASH
    #include <SPI.h>
    #include <SPIFlash.h>
#endif

#include <boards.h>
#include "flash.h"
#include "display.h"
#include "timerutl.h"

#ifdef BOOTDISPLAY
    #include <TFT_eSPI.h>
    TFT_eSPI tft = TFT_eSPI();
    uint8_t progress = 0;  // Текущий прогресс в процентах
#endif

#define SD_FAT_TYPE 3

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

word jedecID;
char jedecIDStr[16];
#endif
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
            snprintf(jedecIDStr, sizeof(jedecIDStr), "%06X", jedecID);
            // while (1);
            logMessage(jedecIDStr);
            
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

    #ifdef SPIFLASH
        jedecID = flash.readDeviceId();  // Получаем JEDEC ID
        snprintf(jedecIDStr, sizeof(jedecIDStr), "jedecID: %06X", jedecID);
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