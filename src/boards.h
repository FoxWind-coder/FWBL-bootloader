#pragma once
// #ifndef BOARDS_H
// #define BOARDS_H
#define BLVER  "FWBL v0.2 BETA"
// Определение платформы и соответствующих пинов
// #define POWERHOLD

#ifdef BOOTLOADER
    // #define WIFI
    // #define DEBUG
    #define WIFI_FILE "esp8266.bin"
    #define WIFIBACKUP "wifibackup.bin"
    #define WIFI_CUR "esp8266.cur"
    #define FIRMWARE_FILE "firmware.bin"
    #define PREFLASH "preloader.bin"
    #define LOG_FILE "flashlog.txt"
    #define PREFLASH_CURRENT "preloader.CUR"
    #define FIRMWARE_CURRENT "firmware.CUR"
    #define BACKUPNAME "backup.bin"
    #define BACKUP_CURRENT "backup.CUR"
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

#ifdef BOOTDISPLAY
#define STM32
#define USER_SETUP_INFO "User_Setup"
#define LOAD_GLCD   // Font 1. Original Adafruit 8 pixel font needs ~1820 bytes in FLASH
// #define LOAD_FONT2  // Font 2. Small 16 pixel high font, needs ~3534 bytes in FLASH, 96 characters
// #define LOAD_FONT4  // Font 4. Medium 26 pixel high font, needs ~5848 bytes in FLASH, 96 characters
// #define LOAD_FONT6  // Font 6. Large 48 pixel font, needs ~2666 bytes in FLASH, only characters 1234567890:-.apm
// #define LOAD_FONT7  // Font 7. 7 segment 48 pixel font, needs ~2438 bytes in FLASH, only characters 1234567890:-.
// #define LOAD_FONT8  // Font 8. Large 75 pixel font needs ~3256 bytes in FLASH, only characters 1234567890:-.
//#define LOAD_FONT8N // Font 8. Alternative to Font 8 above, slightly narrower, so 3 digits fit a 160 pixel TFT
// #define LOAD_GFXFF  // FreeFonts. Include access to the 48 Adafruit_GFX free fonts FF1 to FF48 and custom fonts

// Comment out the #define below to stop the SPIFFS filing system and smooth font code being loaded
// this will save ~20kbytes of FLASH
// #define SMOOTH_FONT
#endif

#ifdef GENERIC_BOARD
    #define SD_CS_PIN PC11
    #define SOFT_MISO_PIN PC8 //пины карты памяти
    #define SOFT_MOSI_PIN PD2
    #define SOFT_SCK_PIN PC12
    // SD_DET не определен для generic
    #ifdef DEBUG  
        #define DEBUG_RX PB11// включение отладки через serial 
        #define DEBUG_TX PB10
        #define SERIAL_BAUD 115200   
    #else
        #define NDEBUG
    #endif
    #ifdef BOOTDISPLAY
        #error "displays not configured. you need do it urself"
    #endif

#elif defined(RN3_BOARD)

    #ifdef WIFI
    #define SERIAL_PORT 2
    #define RST_PIN PE9
    #define GPIO0_PIN PC13
    #define WIFIRX PA10
    #define WIFITX PA9
    #define WIFIBAUD 115200
    #define FLASH_SIZE 1048576  // Размер флеш-памяти ESP8266 (например, 1 МБ)
    #define READ_BLOCK_SIZE 256 // Размер блока чтения за один раз
    #endif
    
    #define SDLOG
    #define SD_CS_PIN PC9
        #define SOFT_MISO_PIN PC11
        #define SOFT_MOSI_PIN PC12
        #define SOFT_SCK_PIN PC10
        #define SD_DET PD12

        #ifdef BOOTDISPLAY
            #define TFT_BL   PD13            // LED back-light control pin
            #define TFT_BACKLIGHT_ON HIGH  // Level to turn ON back-light (HIGH or LOW)
            #define TFT_MISO  PA6  // Automatically assigned with ESP8266 if not defined
            #define TFT_MOSI  PA7  // Automatically assigned with ESP8266 if not defined
            #define TFT_SCLK  PA5  // Automatically assigned with ESP8266 if not defined
            #define TFT_CS    PD11  // Chip select control pin D8
            #define TFT_DC    PD10  // Data Command control pin
            #define TFT_RST   PC6  // Reset pin (could connect to NodeMCU RST, see next line)
            #define SPI_FREQUENCY  80000000
            #define SPI_READ_FREQUENCY  20000000
            #define SPI_TOUCH_FREQUENCY  2500000
            #define TOUCH_CS PE14     // Chip select pin (T_CS) of touch screen

            #if BOOTDISPLAY == MKS_TS35
                #define ST7796_DRIVER
                #ifdef SHUI
                    #define setaddr 0
                    #define BASE_SCREEN_HEIGHT 480
                    #define BASE_SCREEN_WIDTH 320
                #else
                    #if defined(ROTATION) && (ROTATION == 0 || ROTATION == 2)
                        #define BASE_SCREEN_HEIGHT 480
                        #define BASE_SCREEN_WIDTH 320
                    #else
                        #define BASE_SCREEN_HEIGHT 320
                        #define BASE_SCREEN_WIDTH 480
                    #endif
                #endif
                        #define BASE_ROTATION 1
                        // Определяем отступы
                        #define TOP_MARGIN    10
                        #define BOTTOM_MARGIN 50
                        #define LEFT_MARGIN   10
                        #define LINE_HEIGHT   10 // Определяем высоту строки
            #endif
            
        #endif

        #ifdef SPIFLASH
            #define CUSTOM_CS   PB12
            #define csPin PB12
            #define clkPin PB13
            #define diPin PC3
            #define doPin PC2
        #endif

        #define BEEPER PC5

        #ifdef BEEPER
            #define BASE_BEEPSTATE 1
        #endif

        #ifdef POWERHOLD
        #define HOLD_PIN PB2
        #endif

        #ifdef DEBUG  
            #define DEBUG_RX PB11// включение отладки через serial 
            #define DEBUG_TX PB10
            #define SERIAL_BAUD 115200   
        #else
            #define NDEBUG
        #endif
        
#elif defined(SKIPR_BOARD)
    #define SD_CS_PIN PC9
    #define SOFT_MISO_PIN PC11
    #define SOFT_MOSI_PIN PC12
    #define SOFT_SCK_PIN PC10
    #define SD_DET PC4

    #ifdef BOOTDISPLAY
        #define TFT_BL   PE11            // LED back-light control pin
        #define TFT_BACKLIGHT_ON HIGH  // Level to turn ON back-light (HIGH or LOW)
        #define TFT_MISO  PA6  // Automatically assigned with ESP8266 if not defined
        #define TFT_MOSI  PA7  // Automatically assigned with ESP8266 if not defined
        #define TFT_SCLK  PA5  // Automatically assigned with ESP8266 if not defined

        #define TFT_CS    PE15  // Chip select control pin D8
        #define TFT_DC    PE7  // Data Command control pin
        #define TFT_RST   PD10  // Reset pin (could connect to NodeMCU RST, see next line)
        #define SPI_FREQUENCY  80000000
        #define SPI_READ_FREQUENCY  20000000
        #define SPI_TOUCH_FREQUENCY  2500000
        #define TOUCH_CS PD9     // Chip select pin (T_CS) of touch screen

        #ifdef BOOTDISPLAY == MKS_TS35
        #define ST7796_DRIVER
            #ifdef SHUI
                #define BASE_SCREEN_HEIGHT 480
                #define BASE_SCREEN_WIDTH 320
            #elif
                #if defined(ROTATION) && (ROTATION == 0 || ROTATION == 2)
                    #define BASE_SCREEN_HEIGHT 480
                    #define BASE_SCREEN_WIDTH 320
                #else
                    #define BASE_SCREEN_HEIGHT 320
                    #define BASE_SCREEN_WIDTH 480
                #endif
            #endif
        #endif
    #endif

    #define BEEPER PB2
    
    #ifdef DEBUG  
        #define DEBUG_RX PB11// включение отладки через serial 
        #define DEBUG_TX PB10
        #define SERIAL_BAUD 115200   
    #else
        #define NDEBUG
    #endif
#else
    #error "board not selected"
#endif

#if defined(USBSERDBG) && !defined(USBD_USE_CDC)
    #error "USBSERDBG is defined, but USBD_USE_CDC is missing!"
#endif

#if defined(USBSERDBG) && !defined(USBCON)
    #error "USBSERDBG is defined, but USBCON is missing!"
#endif

#if !defined(USBSERDBG) && (defined(USBD_USE_CDC) || defined(USBCON))
    #error "USBSERDBG is not defined, but USBD_USE_CDC or USBCON is defined!"
#endif

// #endif // BOARDS_H
